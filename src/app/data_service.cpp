#include "data_service.h"

#include <cstdio>
#include <cstring>
#include <new>

#include "generated/places_coords.h"
#include "nvp/calc/calibrate.h"
#include "nvp/net/coords.h"
#include "nvp/providers/http_source.h"
#include "nvp/providers/imsakiyem.h"
#include "nvp/providers/planner.h"
#include "nvp/providers/runner.h"
#include "nvp/store/refresh_state.h"
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
using nvp::providers::ProviderId;
using nvp::store::RefreshOutcome;
using nvp::store::Store;

// --- Shared between the cores, under g_lock ---
platform::tasks::Lock* g_lock = nullptr;
Store* g_pub = nullptr;  // the published copy (PSRAM)
uint32_t g_version = 0;
bool g_today_known = false;
Date g_today;
int32_t g_utc_offset = 3 * 3600;
uint8_t g_pub_outcome = 0;
int64_t g_pub_next = 0;
bool g_req_refresh = false, g_req_show = false, g_req_corrupt = false, g_req_erase = false;
bool g_req_plan = false, g_req_calib = false;
int g_req_block = -1;  // provider; -1 none
bool g_req_block_on = false;

// --- Worker only ---
Store* g_store = nullptr;  // the worker's store (PSRAM)
Store* g_fill = nullptr;   // scratch for the published copy with the device's own days (PSRAM)
nvp::providers::RunScratch* g_scratch = nullptr;
nvp::store::RefreshState g_state;
nvp::providers::Planner g_planner(g_state);
bool g_ready = false;
uint32_t g_old_district = 0;  // a previous district's file, removed after the first save for the new one
bool g_had_location = false;
bool g_prev_up = false;
uint8_t g_blocked = 0;  // bit (1 << ProviderId), development (R10), RAM only
uint32_t g_mismatches = 0;
char g_district_name[48] = {};
nvp::net::Coords g_coords;
const char* g_coords_origin = "none";
bool g_detail_tried = false;
Date g_published_day{0, 0, 0};
void (*g_before_write)() = nullptr;
platform::littlefs_fs::Fs g_fs;

uint32_t now_ms() { return platform::sysinfo::uptime_ms(); }
int64_t now_utc() { return platform::systime::utc_us() / 1000000; }

void before_write() {
    if (g_before_write != nullptr) g_before_write();
}

void log_line(const char* text) {
    platform::console::printf("[%8lu] data: %s\n", static_cast<unsigned long>(now_ms()), text);
}

void date_text(Date d, char* out, size_t cap) {
    std::snprintf(out, cap, "%04d-%02u-%02u", static_cast<int>(d.year), static_cast<unsigned>(d.month),
                  static_cast<unsigned>(d.day));
}

void range_text(const Store& s, char* out, size_t cap) {
    if (s.count == 0) {
        std::snprintf(out, cap, "none");
        return;
    }
    char a[16], b[16];
    date_text(s.records[0].date, a, sizeof a);
    date_text(s.records[s.count - 1].date, b, sizeof b);
    std::snprintf(out, cap, "%s..%s", a, b);
}

// "26.11.2026 03:41" in local time.
void local_text(int64_t utc, int32_t offset, char* out, size_t cap) {
    if (utc <= 0) {
        std::snprintf(out, cap, "none");
        return;
    }
    const int64_t local = utc + offset;
    int64_t day = local / 86400;
    if (local < 0 && local % 86400 != 0) --day;
    const int64_t sod = local - day * 86400;
    const Date d = nvp::engine::to_date(static_cast<nvp::engine::DayNumber>(day));
    std::snprintf(out, cap, "%02u.%02u.%04d %02u:%02u", static_cast<unsigned>(d.day), static_cast<unsigned>(d.month),
                  static_cast<int>(d.year), static_cast<unsigned>(sod / 3600), static_cast<unsigned>((sod / 60) % 60));
}

bool has_coords() { return g_coords.valid && !g_state.coords_suspect; }

nvp::calc::Coordinates coords() {
    return nvp::calc::Coordinates{g_coords.lat_e5 / 100000.0, g_coords.lon_e5 / 100000.0};
}

const nvp::store::Calibration* calibration() {
    const nvp::store::Calibration& c = g_state.calibration;
    return c.valid && c.district == g_store->district_id ? &c : nullptr;
}

// The published copy: the store, plus the device's own days when nothing covers today (R8), plus a
// computed Hijri date on calculated days (shown only in the calculated state, 02 §8).
void publish() {
    *g_fill = *g_store;
    Date today{0, 0, 0};
    int32_t offset = 0;
    bool known = false;
    {
        platform::tasks::Guard guard(*g_lock);
        known = g_today_known;
        today = g_today;
        offset = g_utc_offset;
    }
    if (known && has_coords() && g_fill->district_id != 0) {
        const size_t added = nvp::calc::fill_local(*g_fill, today, coords(), calibration(), 8, offset);
        if (added > 0 && nvp::store::find(*g_store, today) == nullptr && !(g_published_day == today)) {
            char a[16], line[96];
            date_text(today, a, sizeof a);
            std::snprintf(line, sizeof line, "local times for %s on (%u days, no record for today)", a,
                          static_cast<unsigned>(added));
            log_line(line);
        }
    }
    const int8_t hijri_offset = g_state.calibration.district == g_fill->district_id ? g_state.calibration.hijri_offset : 0;
    for (uint16_t i = 0; i < g_fill->count; ++i) {
        DayRecord& r = g_fill->records[i];
        if (!r.hijri.present && nvp::engine::rank(r.source) < 40) r.hijri = nvp::calc::computed_hijri(r.date, hijri_offset);
    }
    if (known) g_published_day = today;
    platform::tasks::Guard guard(*g_lock);
    *g_pub = *g_fill;
    g_pub_outcome = static_cast<uint8_t>(g_state.last_outcome);
    ++g_version;
}

void file_name(uint32_t district, char* out, size_t cap) { nvp::store::store_file_name(district, false, out, cap); }

// Loads the store of `district` into g_store; a bad file is logged once, deleted and never shown
// (spec 006 FR-009). A leftover .new is removed by load().
void load_store(uint32_t district) {
    char name[32];
    file_name(district, name, sizeof name);
    before_write();  // load() may delete a leftover .new, and a bad file is deleted below
    const nvp::store::LoadResult r = nvp::store::load(g_fs, district, *g_store);
    char line[112];
    if (r == nvp::store::LoadResult::Ok) {
        char range[40];
        range_text(*g_store, range, sizeof range);
        std::snprintf(line, sizeof line, "%s loaded, %u records %s", name, static_cast<unsigned>(g_store->count), range);
    } else if (r == nvp::store::LoadResult::Missing) {
        std::snprintf(line, sizeof line, "no store for district %lu", static_cast<unsigned long>(district));
    } else {
        g_fs.remove(name);
        std::snprintf(line, sizeof line, "%s %s, ignored", name, nvp::store::load_result_text(r));
    }
    log_line(line);
}

// Coordinates from the bundled table (research R4); the district detail is asked later, online.
void resolve_coords(uint32_t district, const char* name) {
    g_coords = nvp::net::lookup_coords(generated::places_coords, generated::places_coords_len, district, name);
    g_coords_origin = g_coords.valid ? "table" : "none";
    g_detail_tried = false;
    char line[96];
    if (g_coords.valid) {
        std::snprintf(line, sizeof line, "coordinates %ld.%05ld %ld.%05ld (table)",
                      static_cast<long>(g_coords.lat_e5 / 100000), static_cast<long>(g_coords.lat_e5 % 100000),
                      static_cast<long>(g_coords.lon_e5 / 100000), static_cast<long>(g_coords.lon_e5 % 100000));
    } else {
        std::snprintf(line, sizeof line, "coordinates not in the table for district %lu", static_cast<unsigned long>(district));
    }
    log_line(line);
}

// The district detail, once per location, when the table had no row (FR-015).
class DetailBody : public nvp::providers::HttpBody {
public:
    nvp::providers::DistrictDetailParser parser;
    void on_status(int) override {}
    bool on_chunk(const char* data, size_t n) override { return parser.feed(data, n); }
};

void fetch_detail(uint32_t district, bool time_valid) {
    g_detail_tried = true;
    char url[96];
    nvp::providers::district_detail_url(district, url, sizeof url);
    platform::https_fetch::Client http(time_valid);
    DetailBody body;
    const nvp::providers::HttpResult h = http.get(url, body);
    body.parser.finish();
    const nvp::providers::DistrictDetail& d = body.parser.detail();
    char line[112];
    if (h.outcome == nvp::providers::HttpOutcome::Done && h.status == 200 && d.accepted()) {
        g_coords.valid = true;
        g_coords.lat_e5 = d.lat_e5;
        g_coords.lon_e5 = d.lon_e5;
        g_coords_origin = "detail";
        std::snprintf(line, sizeof line, "coordinates from the district detail (%s)", d.confidence);
        publish();
    } else {
        std::snprintf(line, sizeof line, "coordinates not resolved (http %d, %s)", h.status,
                      d.has_coords ? d.confidence : "none");
    }
    log_line(line);
}

void save_state() {
    before_write();
    const nvp::store::SaveResult r = nvp::store::save_refresh_state(g_fs, g_state);
    if (r != nvp::store::SaveResult::Ok) {
        char line[64];
        std::snprintf(line, sizeof line, "refresh state not saved (%s)", nvp::store::save_result_text(r));
        log_line(line);
    }
}

// Calibration from the stored Diyanet-derived days (FR-018), persisted when it changes.
void recalibrate() {
    if (!has_coords()) return;
    const nvp::calc::CalibrationResult r = nvp::calc::calibrate(g_store->records, g_store->count, coords(), g_utc_offset);
    nvp::store::Calibration c = g_state.calibration;
    char line[112];
    if (r.status == nvp::calc::CalibrationResult::Status::Ok) {
        c.valid = true;
        for (int k = 0; k < 6; ++k) c.offsets[k] = r.offsets[k];
        c.days = r.days;
        c.district = g_store->district_id;
        std::snprintf(line, sizeof line, "calibration %d %d %d %d %d %d from %u days, hijri %+d", c.offsets[0],
                      c.offsets[1], c.offsets[2], c.offsets[3], c.offsets[4], c.offsets[5], static_cast<unsigned>(c.days),
                      r.hijri_ok ? r.hijri_offset : c.hijri_offset);
    } else if (r.status == nvp::calc::CalibrationResult::Status::OffsetTooLarge) {
        const char* names[6] = {"imsak", "gunes", "ogle", "ikindi", "aksam", "yatsi"};
        std::snprintf(line, sizeof line, "calibration rejected (%s %+d > %d min)", names[r.bad_prayer], r.bad_offset,
                      nvp::calc::kCalibrationLimit);
        log_line(line);
        return;
    } else {
        return;  // too few Diyanet days: keep what there is
    }
    if (r.hijri_ok) {
        c.hijri_offset = r.hijri_offset;
        c.district = g_store->district_id;
    }
    const bool changed = !(c.valid == g_state.calibration.valid && c.days == g_state.calibration.days &&
                           c.district == g_state.calibration.district &&
                           c.hijri_offset == g_state.calibration.hijri_offset &&
                           std::memcmp(c.offsets, g_state.calibration.offsets, sizeof c.offsets) == 0);
    if (!changed) return;
    g_state.calibration = c;
    log_line(line);
}

void show() {
    char line[200];
    if (g_store->count == 0) {
        std::snprintf(line, sizeof line, "none (%s)", g_store->district_id == 0 ? "no location" : "store empty");
        log_line(line);
        return;
    }
    // The checksum of the file as it is now (it may have been corrupted on purpose).
    before_write();  // load() may delete a leftover .new (spec 006 FR-011, T036)
    const nvp::store::LoadResult r = nvp::store::load(g_fs, g_store->district_id, g_scratch->merged);
    char range[40];
    range_text(*g_store, range, sizeof range);
    unsigned per[5] = {};
    for (uint16_t i = 0; i < g_store->count; ++i) ++per[static_cast<size_t>(g_store->records[i].source)];
    char parts[96] = "";
    size_t used = 0;
    for (size_t s = 0; s < 5; ++s) {
        if (per[s] == 0) continue;
        const int n = std::snprintf(parts + used, sizeof parts - used, "%s%s %u", used == 0 ? "" : ", ",
                                    nvp::store::source_key(static_cast<nvp::engine::SourceId>(s)), per[s]);
        if (n > 0 && used + static_cast<size_t>(n) < sizeof parts) used += static_cast<size_t>(n);
    }
    char refresh[24];
    local_text(g_store->last_refresh_utc, 0, refresh, sizeof refresh);
    std::snprintf(line, sizeof line, "district %lu, %u records %s (%s), crc %s, last refresh %s UTC, mismatches %lu",
                  static_cast<unsigned long>(g_store->district_id), static_cast<unsigned>(g_store->count), range, parts,
                  r == nvp::store::LoadResult::Ok ? "ok" : nvp::store::load_result_text(r), refresh,
                  static_cast<unsigned long>(g_mismatches));
    log_line(line);
}

void plan(const nvp::providers::PlannerView& v) {
    char a[24], b[24], line[176];
    local_text(g_state.last_success_utc, v.utc_offset_s, a, sizeof a);
    local_text(g_planner.next_weekly_utc(v), v.utc_offset_s, b, sizeof b);
    std::snprintf(line, sizeof line, "plan last success %s (%s), next weekly %s, night minute %02u:%02u", a,
                  nvp::store::refresh_outcome_text(g_state.last_outcome), b,
                  static_cast<unsigned>(g_planner.night_minute() / 60), static_cast<unsigned>(g_planner.night_minute() % 60));
    log_line(line);
    local_text(g_state.next_retry_utc, v.utc_offset_s, a, sizeof a);
    std::snprintf(line, sizeof line, "plan retry %s (step %u), today %s, %ld days ahead",
                  g_state.next_retry_utc == 0 ? "none" : a, static_cast<unsigned>(g_state.retry_step),
                  v.today_diyanet ? "diyanet" : (v.today_calculated ? "calculated" : "none"),
                  static_cast<long>(v.days_ahead < 0 ? 0 : v.days_ahead));
    log_line(line);
    for (size_t i = 1; i < nvp::providers::kFetchedProviders; ++i) {
        const nvp::store::ProviderState& ps = g_state.providers[i];
        const ProviderId p = static_cast<ProviderId>(i);
        local_text(ps.cool_down_until, v.utc_offset_s, a, sizeof a);
        local_text(ps.skip_until, v.utc_offset_s, b, sizeof b);
        std::snprintf(line, sizeof line, "plan %s failures %u%s%s%s%s%s", nvp::providers::provider_key(p),
                      static_cast<unsigned>(ps.failures), ps.cool_down_until > v.now_utc ? ", cool-down until " : "",
                      ps.cool_down_until > v.now_utc ? a : "", ps.skip_until > v.now_utc ? ", skipped until " : "",
                      ps.skip_until > v.now_utc ? b : "", (g_blocked & (1u << i)) != 0 ? ", blocked" : "");
        log_line(line);
    }
}

void calib() {
    char line[128];
    if (g_coords.valid) {
        std::snprintf(line, sizeof line, "calib coords %ld.%05ld %ld.%05ld (%s)%s", static_cast<long>(g_coords.lat_e5 / 100000),
                      static_cast<long>(g_coords.lat_e5 % 100000), static_cast<long>(g_coords.lon_e5 / 100000),
                      static_cast<long>(g_coords.lon_e5 % 100000), g_coords_origin,
                      g_state.coords_suspect ? ", suspect (two mirrors agreed, both implausible)" : "");
    } else {
        std::snprintf(line, sizeof line, "calib coords none");
    }
    log_line(line);
    const nvp::store::Calibration* c = calibration();
    if (c != nullptr) {
        std::snprintf(line, sizeof line, "calib offsets %d %d %d %d %d %d from %u days, hijri %+d", c->offsets[0],
                      c->offsets[1], c->offsets[2], c->offsets[3], c->offsets[4], c->offsets[5],
                      static_cast<unsigned>(c->days), c->hijri_offset);
    } else {
        unsigned diyanet = 0;
        for (uint16_t i = 0; i < g_store->count; ++i) diyanet += nvp::engine::rank(g_store->records[i].source) >= 40 ? 1 : 0;
        std::snprintf(line, sizeof line, "calib none (%u diyanet days, %u needed)", diyanet,
                      static_cast<unsigned>(nvp::calc::kCalibrationMinDays));
    }
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

class Control : public nvp::providers::RunControl {
public:
    explicit Control(void (*gate)()) : gate_(gate) {}
    // The location is changed on this same task, between ticks, so a run is never overtaken by it
    // here; the planner abandons nothing on the device (host test T023 covers the mechanism).
    bool should_continue() override { return true; }
    void before_save() override {
        if (gate_ != nullptr) gate_();
    }

private:
    void (*gate_)();
};

// One line per provider call (FR-023).
void log_provider(ProviderId p, const nvp::providers::ProviderReport& rep, uint32_t ms) {
    char line[160];
    const char* key = nvp::providers::provider_key(p);
    if (!rep.asked) {
        if (rep.skip == nvp::providers::Skip::Blocked || rep.skip == nvp::providers::Skip::CoolingDown ||
            rep.skip == nvp::providers::Skip::Skipped24h || rep.skip == nvp::providers::Skip::Cap ||
            rep.skip == nvp::providers::Skip::NoCoords) {
            std::snprintf(line, sizeof line, "%s %s", key, nvp::providers::skip_text(rep.skip));
            log_line(line);
        }
        return;
    }
    const nvp::providers::FetchResult& r = rep.result;
    if (r.outcome == nvp::providers::FetchOutcome::Ok) {
        std::snprintf(line, sizeof line, "%s ok, %u records, %u requests, %lu.%lu s", key, static_cast<unsigned>(rep.records),
                      static_cast<unsigned>(r.requests), static_cast<unsigned long>(ms / 1000),
                      static_cast<unsigned long>((ms % 1000) / 100));
    } else if (r.reason == nvp::providers::Reason::Implausible) {
        const char* names[6] = {"imsak", "gunes", "ogle", "ikindi", "aksam", "yatsi"};
        char d[16];
        date_text(rep.plausibility.date, d, sizeof d);
        std::snprintf(line, sizeof line, "%s rejected (implausible: %s %s %+d min from calculation), store unchanged", key,
                      d, rep.plausibility.prayer >= 0 ? names[rep.plausibility.prayer] : "?", rep.plausibility.diff);
    } else if (r.outcome == nvp::providers::FetchOutcome::RateLimited) {
        std::snprintf(line, sizeof line, "%s cooling down 30 min (http %d)", key, r.http_status);
    } else if (r.reason == nvp::providers::Reason::Network || r.reason == nvp::providers::Reason::Http) {
        std::snprintf(line, sizeof line, "%s failed (%s %d)", key, nvp::providers::reason_text(r.reason), r.http_status);
    } else {
        std::snprintf(line, sizeof line, "%s rejected (%s), store unchanged", key, nvp::providers::reason_text(r.reason));
    }
    log_line(line);
}

nvp::providers::PlannerView make_view(const Tick& t, Date today, int32_t offset) {
    nvp::providers::PlannerView v;
    v.now_utc = now_utc();
    v.utc_offset_s = offset;
    v.clock_valid = t.time_valid;
    v.link_up = t.running_up;
    v.has_location = t.has_location;
    v.days_ahead = nvp::providers::diyanet_days_ahead(*g_store, today);
    v.today_diyanet = v.days_ahead >= 0;
    const DayRecord* r = nvp::store::find(*g_store, today);
    v.today_calculated = r != nullptr ? nvp::engine::rank(r->source) < 40 : has_coords();
    return v;
}

void refresh(const Tick& t, Date today, int32_t offset, const nvp::providers::Decision& d) {
    char line[160];
    std::snprintf(line, sizeof line, "refresh started (%s: %s)%s", nvp::providers::trigger_text(d.trigger), d.why,
                  d.lite ? ", lite" : "");
    log_line(line);
    nvp::providers::RunInput in;
    in.district = t.district;
    in.today = today;
    in.now_utc = now_utc();
    in.has_coords = has_coords();
    in.lat_e5 = g_coords.lat_e5;
    in.lon_e5 = g_coords.lon_e5;
    in.today_calculated = make_view(t, today, offset).today_calculated;
    in.lite = d.lite;
    in.blocked = g_blocked;
    in.utc_offset_s = offset;
    platform::https_fetch::Client http(t.time_valid);
    nvp::providers::HttpSource src(http);
    Control control(t.before_save);
    const uint32_t t0 = now_ms();
    const nvp::providers::RunResult r = nvp::providers::refresh_run(src, g_fs, in, *g_store, *g_scratch, g_state, &control);
    const uint32_t ms = now_ms() - t0;
    for (size_t i = 1; i < nvp::providers::kFetchedProviders; ++i) log_provider(static_cast<ProviderId>(i), r.providers[i], ms);
    if (r.has_mismatch) {
        char dd[16];
        date_text(r.mismatch_stored.date, dd, sizeof dd);
        const DayRecord& a = r.mismatch_stored;
        const DayRecord& b = r.mismatch_incoming;
        std::snprintf(line, sizeof line, "MISMATCH %u days, first %s: %s %u %u %u %u %u %u / %s %u %u %u %u %u %u",
                      static_cast<unsigned>(r.mismatches), dd, nvp::store::source_key(a.source), a.minutes[0], a.minutes[1],
                      a.minutes[2], a.minutes[3], a.minutes[4], a.minutes[5], nvp::store::source_key(b.source), b.minutes[0],
                      b.minutes[1], b.minutes[2], b.minutes[3], b.minutes[4], b.minutes[5]);
        log_line(line);
    }
    g_mismatches += r.mismatches;
    if (r.coords_suspect_set) log_line("coordinates suspect (two mirrors agreed, both implausible)");
    const bool calculated_after = make_view(t, today, offset).today_calculated;
    g_planner.on_finished(r.outcome, r.diyanet_stored, now_utc(), calculated_after);
    if (r.saved) recalibrate();
    save_state();
    char range[40];
    range_text(*g_store, range, sizeof range);
    if (r.outcome == RefreshOutcome::Complete) {
        std::snprintf(line, sizeof line, "refresh complete, %u records (%s), %u requests, %lu.%lu s, heap min %lu kB",
                      static_cast<unsigned>(g_store->count), range, static_cast<unsigned>(r.requests),
                      static_cast<unsigned long>(ms / 1000), static_cast<unsigned long>((ms % 1000) / 100),
                      static_cast<unsigned long>(http.heap_min_internal() / 1024));
    } else {
        char next[24];
        local_text(g_state.next_retry_utc, offset, next, sizeof next);
        std::snprintf(line, sizeof line, "refresh %s, %u records (%s), %u requests, retry %s%s",
                      nvp::store::refresh_outcome_text(r.outcome), static_cast<unsigned>(g_store->count), range,
                      static_cast<unsigned>(r.requests), next, calculated_after ? " (today calculated, hourly)" : "");
    }
    log_line(line);
    // DATA-08 (spec 006 T031): the old district's file goes only after the first save for the new one.
    if (r.saved && r.diyanet_stored && g_old_district != 0 && g_old_district != t.district) {
        char old[32];
        file_name(g_old_district, old, sizeof old);
        before_write();
        g_fs.remove(old);
        std::snprintf(line, sizeof line, "store for district %lu removed (location changed to %lu)",
                      static_cast<unsigned long>(g_old_district), static_cast<unsigned long>(t.district));
        log_line(line);
        g_old_district = 0;
    }
    publish();
}

}  // namespace

// --- UI core ---

void note_today(Date today, int32_t utc_offset_s) {
    if (g_lock == nullptr) return;
    platform::tasks::Guard guard(*g_lock);
    g_today = today;
    g_utc_offset = utc_offset_s;
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
    s.last_outcome = g_pub_outcome;
    s.next_utc = g_pub_next;
    if (s.present) {
        s.first = g_pub->records[0].date;
        s.last = g_pub->records[g_pub->count - 1].date;
        s.last_refresh_utc = g_pub->last_refresh_utc;
    }
    return s;
}

uint16_t stored_days_after(Date today) {
    if (g_lock == nullptr) return 0;
    platform::tasks::Guard guard(*g_lock);
    return nvp::store::consecutive_days_after(*g_pub, today);
}

namespace {
void set_flag(bool& flag) {
    if (g_lock == nullptr) return;
    platform::tasks::Guard guard(*g_lock);
    flag = true;
}
}  // namespace

void request_refresh() { set_flag(g_req_refresh); }
void request_show() { set_flag(g_req_show); }
void request_corrupt() { set_flag(g_req_corrupt); }
void request_erase() { set_flag(g_req_erase); }
void request_plan() { set_flag(g_req_plan); }
void request_calib() { set_flag(g_req_calib); }
void request_block(int provider, bool block) {
    if (g_lock == nullptr) return;
    platform::tasks::Guard guard(*g_lock);
    g_req_block = provider;
    g_req_block_on = block;
}

// --- Network worker ---

void worker_start(bool has_location, uint32_t district, const char* district_name, void (*before_write_fn)()) {
    g_before_write = before_write_fn;
    void* a = platform::tasks::alloc_psram(sizeof(Store));
    void* b = platform::tasks::alloc_psram(sizeof(Store));
    void* f = platform::tasks::alloc_psram(sizeof(Store));
    void* c = platform::tasks::alloc_psram(sizeof(nvp::providers::RunScratch));
    if (a == nullptr || b == nullptr || c == nullptr || f == nullptr) {
        log_line("no memory for the store; prayer times off");
        return;
    }
    g_store = new (a) Store();
    Store* pub = new (b) Store();
    g_fill = new (f) Store();
    g_scratch = new (c) nvp::providers::RunScratch();
    // The refresh state (research R7): a bad or missing file means "never refreshed".
    before_write();
    const nvp::store::LoadResult rs = nvp::store::load_refresh_state(g_fs, g_state);
    char line[96];
    std::snprintf(line, sizeof line, "refresh state %s", rs == nvp::store::LoadResult::Ok
                                                              ? nvp::store::refresh_outcome_text(g_state.last_outcome)
                                                              : nvp::store::load_result_text(rs));
    log_line(line);
    const platform::sysinfo::Snapshot si = platform::sysinfo::read();
    g_planner.set_night_minute(nvp::providers::night_minute_of(si.mac));
    std::snprintf(line, sizeof line, "night minute %02u:%02u", static_cast<unsigned>(g_planner.night_minute() / 60),
                  static_cast<unsigned>(g_planner.night_minute() % 60));
    log_line(line);
    g_planner.on_boot();
    if (has_location) {
        std::snprintf(g_district_name, sizeof g_district_name, "%s", district_name != nullptr ? district_name : "");
        load_store(district);  // a bad file is ignored and deleted
        resolve_coords(district, g_district_name);
    }
    g_had_location = has_location;
    g_lock = new platform::tasks::Lock();
    g_pub = pub;
    publish();
    g_ready = true;
}

void worker_tick(const Tick& t) {
    if (!g_ready) return;
    g_before_write = t.before_save != nullptr ? t.before_save : g_before_write;
    bool want = false, want_show = false, want_corrupt = false, want_erase = false, want_plan = false, want_calib = false;
    bool today_known = false;
    int block = -1;
    bool block_on = false;
    Date today;
    int32_t offset = 0;
    {
        platform::tasks::Guard guard(*g_lock);
        want = g_req_refresh;
        want_show = g_req_show;
        want_corrupt = g_req_corrupt;
        want_erase = g_req_erase;
        want_plan = g_req_plan;
        want_calib = g_req_calib;
        block = g_req_block;
        block_on = g_req_block_on;
        g_req_refresh = g_req_show = g_req_corrupt = g_req_erase = g_req_plan = g_req_calib = false;
        g_req_block = -1;
        today_known = g_today_known;
        today = g_today;
        offset = g_utc_offset;
    }
    // A new location (spec 006 T031, spec 007 FR-004 a): nothing from the old district is shown.
    if (t.has_location && (t.district != g_store->district_id ||
                           std::strcmp(t.district_name != nullptr ? t.district_name : "", g_district_name) != 0)) {
        if (g_store->district_id != 0 && g_store->district_id != t.district) g_old_district = g_store->district_id;
        std::snprintf(g_district_name, sizeof g_district_name, "%s", t.district_name != nullptr ? t.district_name : "");
        if (t.district != g_store->district_id) load_store(t.district);
        resolve_coords(t.district, g_district_name);
        if (g_had_location) {
            g_planner.on_location_changed();
        } else {
            g_planner.on_setup_done();
        }
        publish();
    }
    g_had_location = t.has_location;
    if (t.running_up != g_prev_up) {
        if (t.running_up) g_planner.on_link_up(); else g_planner.on_link_down();
        g_prev_up = t.running_up;
    }

    const nvp::providers::PlannerView view = make_view(t, today, offset);
    if (want_show) show();
    if (want_corrupt) corrupt();
    if (want_erase) erase();
    if (want_plan) plan(view);
    if (want_calib) calib();
    if (block >= 0) {
        char line[64];
        if (block == 0) {
            g_blocked = 0;
            std::snprintf(line, sizeof line, "all unblocked");
        } else {
            const uint8_t bit = static_cast<uint8_t>(1u << block);
            g_blocked = block_on ? static_cast<uint8_t>(g_blocked | bit) : static_cast<uint8_t>(g_blocked & ~bit);
            std::snprintf(line, sizeof line, "%s %s", nvp::providers::provider_key(static_cast<ProviderId>(block)),
                          block_on ? "blocked (development)" : "unblocked");
        }
        log_line(line);
    }
    if (want && !g_planner.on_manual(view.now_utc)) log_line("refused (once per minute)");

    // A new day, or a store change from a command: republish (the device's own days follow today).
    if (today_known && !(g_published_day == today)) publish();

    // Coordinates missing from the table: the district detail, once per location (FR-015).
    if (t.has_location && !g_coords.valid && !g_detail_tried && t.running_up && t.time_valid) {
        fetch_detail(t.district, t.time_valid);
    }

    if (!t.has_location || !today_known) {
        if (want) log_line("refused (no location or no date yet)");
        return;
    }
    const nvp::providers::Decision d = g_planner.decide(view);
    if (d.start) refresh(t, today, offset, d);
    {
        const int64_t next = g_planner.next_scheduled_utc(make_view(t, today, offset));
        platform::tasks::Guard guard(*g_lock);
        g_pub_next = next;
        g_pub_outcome = static_cast<uint8_t>(g_state.last_outcome);
    }
}

}  // namespace app::data
