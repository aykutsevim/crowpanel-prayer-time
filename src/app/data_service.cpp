#include "data_service.h"

#include <cstdio>
#include <cstring>
#include <new>

#include "nvp/providers/refresh.h"
#include "nvp/store/store.h"
#include "platform/console_io.h"
#include "platform/https_fetch.h"
#include "platform/littlefs_fs.h"
#include "platform/sysinfo.h"
#include "platform/systime.h"
#include "platform/tasks.h"

namespace app::data {
namespace {

using nvp::engine::Date;
using nvp::engine::DayRecord;
using nvp::store::Store;

constexpr uint32_t kRetryMs = 60000;  // one retry a minute after a failure (FR-005, R8)

// --- Shared between the cores, under g_lock ---
platform::tasks::Lock* g_lock = nullptr;
Store* g_pub = nullptr;  // the published copy (PSRAM)
uint32_t g_version = 0;
bool g_today_known = false;
Date g_today;
bool g_req_refresh = false, g_req_show = false, g_req_corrupt = false, g_req_erase = false;

// --- Worker only ---
Store* g_store = nullptr;  // the worker's store (PSRAM)
nvp::providers::RefreshScratch* g_scratch = nullptr;
bool g_ready = false;
uint32_t g_old_district = 0;  // a previous district's file, removed after the first save for the new one
uint8_t g_failures = 0;       // failed attempts since the last success, new day, connection or request
uint32_t g_retry_at = 0;
Date g_attempt_day;
platform::littlefs_fs::Fs g_fs;

uint32_t now_ms() { return platform::sysinfo::uptime_ms(); }

void log_line(const char* text) {
    platform::console::printf("[%8lu] data: %s\n", static_cast<unsigned long>(now_ms()), text);
}

void date_text(Date d, char* out, size_t cap) {
    std::snprintf(out, cap, "%04d-%02u-%02u", static_cast<int>(d.year), static_cast<unsigned>(d.month),
                  static_cast<unsigned>(d.day));
}

void range_text(const Store& s, char* out, size_t cap) {
    char a[16], b[16];
    date_text(s.records[0].date, a, sizeof a);
    date_text(s.records[s.count - 1].date, b, sizeof b);
    std::snprintf(out, cap, "%s..%s", a, b);
}

void publish() {
    platform::tasks::Guard guard(*g_lock);
    *g_pub = *g_store;
    ++g_version;
}

// Waits for a running Wi-Fi scan before any flash write (FR-011, T035); set by the worker.
void (*g_before_write)() = nullptr;
void before_write() {
    if (g_before_write != nullptr) g_before_write();
}

void file_name(uint32_t district, char* out, size_t cap) { nvp::store::store_file_name(district, false, out, cap); }

// Loads the store of `district` into g_store; a bad file is logged once, deleted and never shown
// (FR-009, T024). A leftover .new is removed by load().
void load_store(uint32_t district) {
    char name[32];
    file_name(district, name, sizeof name);
    before_write();  // load() may delete a leftover .new, and a bad file is deleted below
    const nvp::store::LoadResult r = nvp::store::load(g_fs, district, *g_store);
    char line[112];
    if (r == nvp::store::LoadResult::Ok) {
        char range[40];
        range_text(*g_store, range, sizeof range);
        std::snprintf(line, sizeof line, "%s loaded, %u records %s", name, static_cast<unsigned>(g_store->count),
                      range);
    } else if (r == nvp::store::LoadResult::Missing) {
        std::snprintf(line, sizeof line, "no store for district %lu", static_cast<unsigned long>(district));
    } else {
        g_fs.remove(name);
        std::snprintf(line, sizeof line, "%s %s, ignored", name, nvp::store::load_result_text(r));
    }
    log_line(line);
}

void show() {
    char line[160];
    if (g_store->count == 0) {
        std::snprintf(line, sizeof line, "none (%s)", g_store->district_id == 0 ? "no location" : "store empty");
        log_line(line);
        return;
    }
    // The checksum of the file as it is now (it may have been corrupted on purpose).
    before_write();  // load() may delete a leftover .new (FR-011, T036)
    const nvp::store::LoadResult r = nvp::store::load(g_fs, g_store->district_id, g_scratch->merged);
    char range[40], refresh[40];
    range_text(*g_store, range, sizeof range);
    const int64_t t = g_store->last_refresh_utc;
    const Date d = nvp::engine::to_date(static_cast<nvp::engine::DayNumber>(t / 86400));
    const int32_t sod = static_cast<int32_t>(t % 86400);
    std::snprintf(refresh, sizeof refresh, "%04d-%02u-%02u %02ld:%02ld UTC", static_cast<int>(d.year),
                  static_cast<unsigned>(d.month), static_cast<unsigned>(d.day), static_cast<long>(sod / 3600),
                  static_cast<long>((sod / 60) % 60));
    std::snprintf(line, sizeof line, "district %lu, %u records %s, source %s, crc %s, last refresh %s",
                  static_cast<unsigned long>(g_store->district_id), static_cast<unsigned>(g_store->count), range,
                  nvp::store::source_key(g_store->records[g_store->count - 1].source),
                  r == nvp::store::LoadResult::Ok ? "ok" : nvp::store::load_result_text(r), refresh);
    log_line(line);
}

void corrupt() {
    char name[32];
    file_name(g_store->district_id, name, sizeof name);
    before_write();
    if (g_store->district_id != 0 && platform::littlefs_fs::flip_middle_byte(name)) {
        log_line("store file corrupted; restart to see the recovery");
    } else {
        log_line("corrupt refused (no store file)");
    }
}

void erase() {
    char name[32];
    file_name(g_store->district_id, name, sizeof name);
    before_write();
    g_fs.remove(name);
    // Cleared in place: a Store is 9.6 kB, too large for a temporary on the worker's stack.
    g_store->count = 0;
    g_store->last_refresh_utc = 0;
    publish();
    log_line("store erased");
}

class Gate : public nvp::providers::SaveGate {
public:
    explicit Gate(void (*fn)()) : fn_(fn) {}
    void before_save() override {
        if (fn_ != nullptr) fn_();
    }

private:
    void (*fn_)();
};

// One line per attempt (FR-017, contracts/serial-console.md).
const char* g_http_error = "";  // esp_err name of the last client failure, for the log

void log_attempt(const char* api, const nvp::providers::Attempt& a, bool last_and_failed) {
    if (!a.tried || a.reason == nvp::providers::Reason::Ok) return;
    char line[128];
    if (a.reason == nvp::providers::Reason::Network) {
        std::snprintf(line, sizeof line, "imsakiyem %s failed (%s %s)%s", api,
                      nvp::providers::http_outcome_text(a.http.outcome), g_http_error,
                      last_and_failed ? (g_failures == 1 ? ", retry in 60 s" : ", no retry until reconnection") : "");
    } else if (a.reason == nvp::providers::Reason::Http) {
        std::snprintf(line, sizeof line, "imsakiyem %s failed (http %d)%s", api, a.http.status,
                      last_and_failed ? (g_failures == 1 ? ", retry in 60 s" : ", no retry until reconnection") : "");
    } else {
        std::snprintf(line, sizeof line, "imsakiyem %s rejected (%s), %lu records, store unchanged%s", api,
                      nvp::providers::reason_text(a.reason), static_cast<unsigned long>(a.records),
                      last_and_failed ? (g_failures == 1 ? ", retry in 60 s" : "") : "");
    }
    log_line(line);
}

void refresh(const Tick& t, Date today) {
    nvp::providers::RefreshRequest req;
    req.district = t.district;
    req.today = today;
    req.now_utc = platform::systime::utc_us() / 1000000;
    platform::https_fetch::Client http(t.time_valid);
    Gate gate(t.before_save);
    const uint32_t t0 = now_ms();
    const nvp::providers::RefreshResult r =
        nvp::providers::refresh_with(http, g_fs, req, *g_store, *g_scratch, &gate);
    const uint32_t ms = now_ms() - t0;
    g_http_error = http.last_error();
    char line[160];
    if (r.stored) {
        g_failures = 0;
        log_attempt("v2", r.v2, false);
        char range[40];
        range_text(*g_store, range, sizeof range);
        std::snprintf(line, sizeof line, "%s ok, %lu records stored (%s), %lu.%lu s, heap min %lu kB",
                      nvp::providers::provider_text(r.provider),
                      static_cast<unsigned long>(r.provider == nvp::providers::Provider::ImsakiyemV1 ? r.v1.records
                                                                                                 : r.v2.records),
                      range, static_cast<unsigned long>(ms / 1000), static_cast<unsigned long>((ms % 1000) / 100),
                      static_cast<unsigned long>(http.heap_min_internal() / 1024));
        log_line(line);
        if (r.merge.mismatches > 0) {  // R6: both time sets
            const DayRecord& a = r.merge.mismatch_stored;
            const DayRecord& b = r.merge.mismatch_incoming;
            char d[16];
            date_text(a.date, d, sizeof d);
            std::snprintf(line, sizeof line, "MISMATCH %u days, first %s: %s %u %u %u %u %u %u / %s %u %u %u %u %u %u",
                          static_cast<unsigned>(r.merge.mismatches), d, nvp::store::source_key(a.source), a.minutes[0],
                          a.minutes[1], a.minutes[2], a.minutes[3], a.minutes[4], a.minutes[5],
                          nvp::store::source_key(b.source), b.minutes[0], b.minutes[1], b.minutes[2], b.minutes[3],
                          b.minutes[4], b.minutes[5]);
            log_line(line);
        }
        publish();
        // DATA-08 (T031): the old district's file goes only after the first save for the new one.
        if (g_old_district != 0 && g_old_district != t.district) {
            char old[32];
            file_name(g_old_district, old, sizeof old);
            before_write();
            g_fs.remove(old);
            std::snprintf(line, sizeof line, "store for district %lu removed (location changed to %lu)",
                          static_cast<unsigned long>(g_old_district), static_cast<unsigned long>(t.district));
            log_line(line);
            g_old_district = 0;
        }
        return;
    }
    ++g_failures;
    g_retry_at = now_ms() + kRetryMs;
    if (r.reason == nvp::providers::Reason::Ok) {  // a valid batch whose save failed
        std::snprintf(line, sizeof line, "%s save failed (%s), store unchanged, %lu ms",
                      nvp::providers::provider_text(r.provider), nvp::store::save_result_text(r.save),
                      static_cast<unsigned long>(ms));
        log_line(line);
        return;
    }
    // A rejection leaves the store and DataView untouched (T027).
    log_attempt("v2", r.v2, !r.v1.tried);
    log_attempt("v1", r.v1, r.v1.tried);
    if (r.v2.reason == nvp::providers::Reason::ErrorCode && !r.v1.tried) {
        std::snprintf(line, sizeof line, "district %lu not found at the provider", static_cast<unsigned long>(t.district));
        log_line(line);
    }
}

}  // namespace

// --- UI core ---

void note_today(Date today) {
    if (g_lock == nullptr) return;
    platform::tasks::Guard guard(*g_lock);
    g_today = today;
    g_today_known = true;
}

Around around(Date today) {
    Around a;
    if (g_lock == nullptr || !nvp::engine::valid(today)) return a;
    const nvp::engine::DayNumber n = nvp::engine::to_day_number(today);
    platform::tasks::Guard guard(*g_lock);
    for (int i = 0; i < 3; ++i) {
        const DayRecord* r = nvp::store::find(*g_pub, nvp::engine::to_date(n - 1 + i));
        if (r != nullptr) {
            a.present[i] = true;
            a.rec[i] = *r;
        }
    }
    return a;
}

Summary summary() {
    Summary s;
    if (g_lock == nullptr) return s;
    platform::tasks::Guard guard(*g_lock);
    s.version = g_version;
    s.district = g_pub->district_id;
    s.count = g_pub->count;
    s.present = g_pub->count > 0;
    if (s.present) {
        s.first = g_pub->records[0].date;
        s.last = g_pub->records[g_pub->count - 1].date;
        s.last_refresh_utc = g_pub->last_refresh_utc;
    }
    return s;
}

void request_refresh() {
    if (g_lock == nullptr) return;
    platform::tasks::Guard guard(*g_lock);
    g_req_refresh = true;
}
void request_show() {
    if (g_lock == nullptr) return;
    platform::tasks::Guard guard(*g_lock);
    g_req_show = true;
}
void request_corrupt() {
    if (g_lock == nullptr) return;
    platform::tasks::Guard guard(*g_lock);
    g_req_corrupt = true;
}
void request_erase() {
    if (g_lock == nullptr) return;
    platform::tasks::Guard guard(*g_lock);
    g_req_erase = true;
}

// --- Network worker ---

void worker_start(bool has_location, uint32_t district, void (*before_write_fn)()) {
    g_before_write = before_write_fn;
    void* a = platform::tasks::alloc_psram(sizeof(Store));
    void* b = platform::tasks::alloc_psram(sizeof(Store));
    void* c = platform::tasks::alloc_psram(sizeof(nvp::providers::RefreshScratch));
    if (a == nullptr || b == nullptr || c == nullptr) {
        log_line("no memory for the store; prayer times off");
        return;
    }
    g_store = new (a) Store();
    Store* pub = new (b) Store();
    g_scratch = new (c) nvp::providers::RefreshScratch();
    if (has_location) {
        load_store(district);  // FR-009: a bad file is ignored and deleted
    }
    g_lock = new platform::tasks::Lock();
    g_pub = pub;
    publish();
    g_ready = true;
}

void worker_tick(const Tick& t) {
    if (!g_ready) return;
    if (t.before_save != nullptr) g_before_write = t.before_save;
    bool want = false, want_show = false, want_corrupt = false, want_erase = false, today_known = false;
    Date today;
    {
        platform::tasks::Guard guard(*g_lock);
        want = g_req_refresh;
        want_show = g_req_show;
        want_corrupt = g_req_corrupt;
        want_erase = g_req_erase;
        g_req_refresh = g_req_show = g_req_corrupt = g_req_erase = false;
        today_known = g_today_known;
        today = g_today;
    }
    // A new location (T031): nothing from the old district is shown for the new one (FR-010).
    if (t.has_location && t.district != g_store->district_id) {
        if (g_store->district_id != 0) g_old_district = g_store->district_id;
        load_store(t.district);
        publish();
        g_failures = 0;
    }
    if (want_show) show();
    if (want_corrupt) corrupt();
    if (want_erase) erase();

    if (!t.has_location || !today_known) {
        if (want) log_line("refused (no location or no date yet)");
        return;
    }
    if (t.link_came_up || !(today == g_attempt_day)) g_failures = 0;  // a new connection or a new day
    if (want) {
        if (!t.running_up || !t.time_valid) {
            log_line(!t.running_up ? "refused (not connected)" : "refused (time not valid)");
            return;
        }
        g_failures = 0;
    } else {
        // R8: when the store does not cover today; after a failure, one retry a minute later.
        if (!t.running_up || !t.time_valid || nvp::store::covers(*g_store, today)) return;
        if (g_failures >= 2) return;
        if (g_failures == 1 && static_cast<int32_t>(now_ms() - g_retry_at) < 0) return;
    }
    g_attempt_day = today;
    refresh(t, today);
}

}  // namespace app::data
