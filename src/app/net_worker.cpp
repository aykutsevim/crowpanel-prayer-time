#include "net_worker.h"

#include <cstdio>
#include <cstring>

#include "generated/web_assets.h"
#include "nvp/diag/backoff.h"
#include "nvp/net/portal_json.h"
#include "nvp/net/scan.h"
#include "nvp/net/submission.h"
#include "platform/console_io.h"
#include "platform/dns_responder.h"
#include "platform/https_probe.h"
#include "platform/settings_store.h"
#include "platform/setup_portal.h"
#include "platform/storage.h"
#include "platform/sysinfo.h"
#include "platform/tasks.h"
#include "platform/wifi_link.h"

namespace app::net {
namespace {

using nvp::net::Event;
using nvp::net::FailureClass;
using nvp::net::Mode;
namespace wifi = platform::wifi;

constexpr int kCore = 0;                 // network work on core 0, display on core 1 (spec 001 R8)
constexpr uint32_t kStackBytes = 12288;  // TLS needs a deep stack
constexpr uint32_t kTickMs = 20;
constexpr uint32_t kJoinTimeoutMs = 20000;  // MODE-04
constexpr uint32_t kTryDelayMs = 1000;      // after the 202, before the channel may change (R2)
constexpr uint32_t kRetryGapMs = 1000;      // between connection tries inside the 20 s window
constexpr uint32_t kGraceMs = 5000;         // MODE-04

// --- Requests and the published snapshot: shared between the cores, under g_lock ---
platform::tasks::Lock* g_lock = nullptr;
bool g_req_setup = false;
bool g_req_cancel = false;
bool g_req_start_over = false;
bool g_stress_on = false;  // desired; the worker starts and stops the run
bool g_req_test_write = false;
bool g_req_test_scan = false;
bool g_req_test_https = false;
bool g_req_test_mix = false;
Snapshot g_pub;

// --- Worker-only state (core 0) ---
bool g_silence_ok = false;
Mode g_mode = Mode::Start;
nvp::diag::Backoff g_backoff;
bool g_has_wifi = false;
bool g_has_location = false;
char g_ssid[33] = {};
char g_pass[64] = {};  // never logged, printed or served (FR-023)
nvp::net::Location g_location;
nvp::net::Submission g_sub;  // the attempt's details; wiped when it ends
char g_attempt_ssid[33] = {};
FailureClass g_failure = FailureClass::None;
uint16_t g_reasons[16] = {};
size_t g_reason_count = 0;
uint32_t g_join_start = 0, g_backoff_until = 0, g_try_at = 0, g_try_start = 0, g_retry_at = 0, g_grace_until = 0;
bool g_trying = false;
bool g_pending_ap = false;  // start the device's network when the first scan finishes (R3)
uint8_t g_phones = 0;
char g_ap_ssid[33] = {};
char g_ap_pass[9] = {};
char g_message[64] = {};
bool g_scan_done_once = false;
nvp::net::ScanEntry g_scan[nvp::net::kMaxScanEntries + 20];
size_t g_scan_count = 0;
enum class ScanOwner { None, Setup, Stress, Test } g_scan_owner = ScanOwner::None;
// Stress.
bool g_stress_running = false, g_stress_paused = false, g_stress_has_run = false;
nvp::diag::StressSchedule g_schedule;
nvp::diag::StressCounters g_counters;
uint32_t g_scratch_counter = 0;
// A flash write started while a Wi-Fi scan runs garbled the panel on the Arduino build; the rule is
// kept (spec 004 FR-018). Stress writes therefore wait for the scan.
bool g_write_pending = false;

uint32_t now() { return platform::sysinfo::uptime_ms(); }

void log(const char* what) {
    platform::console::printf("[%8lu] %s\n", static_cast<unsigned long>(now()), what);
}

void set_message(const char* fmt, const char* arg = "") {
    std::snprintf(g_message, sizeof g_message, fmt, arg);
}

nvp::net::Context context() {
    nvp::net::Context c;
    c.silence_ok = g_silence_ok;
    c.has_wifi = g_has_wifi;
    c.has_location = g_has_location;
    return c;
}

bool seen_in_scan(const char* ssid) {
    for (size_t i = 0; i < g_scan_count; ++i) {
        if (std::strcmp(g_scan[i].ssid, ssid) == 0) return true;
    }
    return false;
}

bool wait_for_scan() {  // FR-018: no storage write while a scan runs
    if (!wifi::scan_running()) return true;
    wifi::scan_stop();
    g_scan_owner = ScanOwner::None;
    return !wifi::scan_running();
}

void start_ap();
void stop_ap();

// Applies one event through the mode machine and performs its actions in flag order.
nvp::net::Action apply(Event event) {
    const Mode before = g_mode;
    const nvp::net::Step step = nvp::net::mode_step(g_mode, event, context());
    const nvp::net::Action a = step.action;
    char line[112];

    if (a & nvp::net::kActRefused) {
        log(g_silence_ok ? "wifi: refused" : "wifi: refused, silence step not acknowledged");
        set_message("Refused");
    }
    if (a & nvp::net::kActEraseWifi) {
        wait_for_scan();
        wifi::erase_stored();
        g_has_wifi = false;
        std::memset(g_ssid, 0, sizeof g_ssid);
        std::memset(g_pass, 0, sizeof g_pass);
        log("wifi: stored Wi-Fi details erased; location kept (MODE-06)");
    }
    if (a & nvp::net::kActDisconnect) {
        g_trying = false;
        wifi::disconnect();
    }
    if (a & nvp::net::kActDiscardSubmitted) {
        nvp::net::wipe(g_sub);
        g_trying = false;
    }
    if (a & nvp::net::kActScan) {
        if (!wifi::scan_running() && wifi::scan_start(false)) {
            g_scan_owner = ScanOwner::Setup;
            if (a & nvp::net::kActStartAp) g_pending_ap = true;  // the network starts after the scan
        }
    }
    if ((a & nvp::net::kActStartAp) && !g_pending_ap) start_ap();
    if (a & nvp::net::kActStopAp) stop_ap();
    if (a & nvp::net::kActScheduleTry) {
        g_failure = FailureClass::None;  // the footer's failure lasts until the next attempt (FR-005)
        g_try_at = now() + kTryDelayMs;
        std::snprintf(g_attempt_ssid, sizeof g_attempt_ssid, "%s", g_sub.ssid);
        std::snprintf(line, sizeof line, "mode: connecting \"%s\"", g_sub.ssid);
        log(line);
    }
    if (a & nvp::net::kActStartTry) {
        wait_for_scan();
        uint16_t drop[16];
        wifi::take_reasons(drop, 16);  // reasons from before this attempt do not count
        g_reason_count = 0;
        wifi::join(g_sub.ssid, g_sub.pass);
        g_try_start = now();
        g_trying = true;
        g_retry_at = 0;
    }
    if (a & nvp::net::kActStoreSubmitted) {
        wait_for_scan();
        const bool loc_ok = platform::settings::store_location(g_sub.location);  // location first (R10)
        const bool wifi_ok = loc_ok && wifi::store(g_sub.ssid, g_sub.pass);
        char ip[16];
        wifi::station_ip(ip);
        if (wifi_ok) {
            g_location = g_sub.location;
            g_has_location = true;
            std::snprintf(g_ssid, sizeof g_ssid, "%s", g_sub.ssid);
            std::snprintf(g_pass, sizeof g_pass, "%s", g_sub.pass);
            g_has_wifi = true;
            std::snprintf(line, sizeof line, "attempt: ok %s; stored wifi + location %lu", ip,
                          static_cast<unsigned long>(g_location.district_id));
        } else {
            std::snprintf(line, sizeof line, "attempt: ok %s; storing failed", ip);
        }
        log(line);
        nvp::net::wipe(g_sub);
        g_trying = false;
    }
    if (a & nvp::net::kActStartGrace) g_grace_until = now() + kGraceMs;
    if (a & nvp::net::kActResetBackoff) g_backoff.reset();
    if (a & nvp::net::kActStartJoin) {
        wifi::join(g_ssid, g_pass);
        g_join_start = now();
        std::snprintf(line, sizeof line, "wifi: joining %s", g_ssid);
        log(line);
    }
    if (a & nvp::net::kActStartBackoff) {
        const uint32_t d = g_backoff.next_delay_ms();
        g_backoff_until = now() + d;
        wifi::disconnect();
        std::snprintf(line, sizeof line, "wifi: not connected, retry in %lu s", static_cast<unsigned long>(d / 1000));
        log(line);
    }
    if (event == Event::JoinOk) {
        std::snprintf(line, sizeof line, "wifi: connected %s", g_ssid);
        log(line);
    }
    g_mode = step.next;
    if (g_mode != before && nvp::net::mode_name(g_mode) != nvp::net::mode_name(before)) {
        const char* why = "";
        if (g_mode == Mode::Setup && before == Mode::Start) why = !g_has_location ? " (no location)" : " (no Wi-Fi)";
        std::snprintf(line, sizeof line, "mode: %s%s", nvp::net::mode_name(g_mode), why);
        log(line);
    }
    return a;
}

// --- Portal callbacks: they run only inside platform::portal::handle(), on this task ---

int on_submit(const char* body, size_t len, char* out, size_t cap) {
    nvp::net::Submission s;
    nvp::net::SubmitError e = nvp::net::parse_submission(body, len, s);
    if (e == nvp::net::SubmitError::Ok &&
        !nvp::net::place_lookup(generated::places_json, generated::places_json_len, s.location.district_id,
                                s.location.province, s.location.district)) {
        e = nvp::net::SubmitError::Location;
    }
    nvp::net::Action a;
    if (e == nvp::net::SubmitError::Ok) {
        if (g_mode == Mode::Setup) g_sub = s;  // only a new attempt takes the details
        a = apply(Event::SubmitValid);
    } else {
        a = apply(Event::SubmitInvalid);
    }
    nvp::net::wipe(s);
    if (a & nvp::net::kActAnswer202) {
        std::snprintf(out, cap, "{\"accepted\":true}");
        return 202;
    }
    if (a & nvp::net::kActAnswer400) {
        std::snprintf(out, cap, "{\"error\":\"%s\"}", nvp::net::submit_error_name(e));
        return 400;
    }
    std::snprintf(out, cap, "{\"error\":\"busy\"}");
    return 409;
}

void on_status(char* out, size_t cap) {
    const char* mode = g_mode == Mode::Connecting ? "connecting" : g_mode == Mode::Grace ? "connected" : "setup";
    nvp::net::status_json(mode, nvp::net::failure_name(g_failure), g_attempt_ssid,
                          g_has_location ? &g_location : nullptr, out, cap);
}

void on_scan(bool refresh, char* out, size_t cap) {
    if (refresh && g_mode == Mode::Setup && !wifi::scan_running() && wifi::scan_start(true)) {
        g_scan_owner = ScanOwner::Setup;
        log("portal: rescan (grouped)");
    }
    nvp::net::scan_json(g_scan, g_scan_count, wifi::scan_running(), out, cap);
}

void start_ap() {
    g_pending_ap = false;
    wifi::hotspot_ssid(g_ap_ssid);
    wifi::hotspot_password(g_ap_pass);
    if (!wifi::start_hotspot(g_ap_ssid, g_ap_pass)) {
        log("wifi: device network failed to start");
        return;
    }
    platform::portal::Callbacks cb;
    cb.submit = on_submit;
    cb.status_json = on_status;
    cb.scan_json = on_scan;
    platform::portal::start(cb);
    const bool dns = platform::dns::start();
    g_phones = 0;
    char line[96];
    std::snprintf(line, sizeof line, "wifi: device network %s open at %s%s", g_ap_ssid, wifi::hotspot_address(),
                  dns ? "" : " (DNS failed)");
    log(line);  // name and address only, never the password (FR-023)
}

void stop_ap() {
    platform::portal::stop();
    platform::dns::stop();
    wifi::stop_hotspot();
    g_pending_ap = false;
    g_grace_until = 0;
    g_phones = 0;
    log("wifi: device network closed");
}

void drive_attempt() {
    if (!g_trying) return;
    const uint32_t t = now();
    g_reason_count += wifi::take_reasons(g_reasons + g_reason_count, 16 - g_reason_count);
    const wifi::Link l = wifi::link();
    if (l == wifi::Link::Up) {
        apply(Event::TryOk);
        return;
    }
    const bool timeout = t - g_try_start >= kJoinTimeoutMs;
    if (l == wifi::Link::Failed || timeout) {
        const FailureClass c = nvp::net::classify_failure(g_reasons, g_reason_count, seen_in_scan(g_sub.ssid));
        // A definite answer (twice "wrong password", or "not found") ends the attempt early.
        const bool definite = g_reason_count >= 2 && c != FailureClass::Other;
        if (!timeout && !definite) {
            if (g_retry_at == 0) g_retry_at = t + kRetryGapMs;
            if (static_cast<int32_t>(t - g_retry_at) >= 0) {
                g_retry_at = 0;
                wifi::join(g_sub.ssid, g_sub.pass);
            }
            return;
        }
        g_failure = c;
        char line[96];
        std::snprintf(line, sizeof line, "attempt: failed %s (%u reasons, last %u)", nvp::net::failure_name(c),
                      static_cast<unsigned>(g_reason_count),
                      static_cast<unsigned>(g_reason_count ? g_reasons[g_reason_count - 1] : 0));
        log(line);
        apply(timeout ? Event::TryTimeout : Event::TryFail);
    }
}

void drive_wifi() {
    const uint32_t t = now();
    switch (g_mode) {
        case Mode::Setup: {
            const uint8_t p = wifi::hotspot_clients();
            if (p != g_phones) {
                const bool joined = p > g_phones;
                g_phones = p;
                char line[48];
                std::snprintf(line, sizeof line, "portal: phone %s (%u)", joined ? "joined" : "left", static_cast<unsigned>(p));
                log(line);
                apply(joined ? Event::PhoneJoined : Event::PhoneLeft);
            }
            break;
        }
        case Mode::Connecting:
            if (g_try_at != 0 && static_cast<int32_t>(t - g_try_at) >= 0) {
                g_try_at = 0;
                apply(Event::TryStart);
            }
            drive_attempt();
            break;
        case Mode::Grace:
            if (static_cast<int32_t>(t - g_grace_until) >= 0) apply(Event::GraceElapsed);
            break;
        case Mode::RunJoining: {
            const wifi::Link l = wifi::link();
            if (l == wifi::Link::Up) apply(Event::JoinOk);
            else if (l == wifi::Link::Failed || t - g_join_start > kJoinTimeoutMs) apply(Event::JoinFail);
            break;
        }
        case Mode::RunUp:
            if (wifi::link() != wifi::Link::Up) apply(Event::LinkLost);
            break;
        case Mode::RunBackoff:
            if (static_cast<int32_t>(t - g_backoff_until) >= 0) apply(Event::BackoffElapsed);
            break;
        case Mode::Start:
        case Mode::RunNoRadio:
            break;
    }
    if (platform::dns::running()) platform::dns::poll();
}

void drive_scans() {
    if (g_scan_owner == ScanOwner::None) return;
    const int r = wifi::scan_poll();
    if (r == -1) return;
    const bool ok = r >= 0;
    size_t n = 0;
    nvp::net::ScanEntry fresh[sizeof g_scan / sizeof g_scan[0]];
    if (ok) {
        n = wifi::scan_take(fresh, sizeof fresh / sizeof fresh[0]);
        g_scan_done_once = true;
    }
    char line[64];
    switch (g_scan_owner) {
        case ScanOwner::Setup:
        case ScanOwner::Stress:
            if (ok) {
                std::memcpy(g_scan, fresh, sizeof g_scan);
                g_scan_count = nvp::net::merge_scan(g_scan, n);
            }
            if (g_scan_owner == ScanOwner::Setup) {
                std::snprintf(line, sizeof line, "scan: %s, %u networks listed", ok ? "done" : "failed",
                              static_cast<unsigned>(g_scan_count));
                log(line);
            } else {
                std::snprintf(line, sizeof line, ok ? "stress: scan done %u networks" : "stress: scan failed",
                              static_cast<unsigned>(n));
                log(line);
                g_counters.record_scan(ok, ok ? nullptr : "scan: failed");
                g_counters.cycle_done(now());
            }
            break;
        case ScanOwner::Test:
            std::snprintf(line, sizeof line, ok ? "test: scan done %u networks" : "test: scan failed",
                          static_cast<unsigned>(n));
            log(line);
            break;
        case ScanOwner::None:
            break;
    }
    g_scan_owner = ScanOwner::None;
    if (g_pending_ap) start_ap();  // the first scan is done: now the device's network (R3)
}

void drive_stress(bool want) {
    if (want && !g_stress_running) {
        g_stress_running = true;
        g_stress_has_run = true;
        g_schedule.start(now());
        g_counters.start(now());
        log("stress: on");
    } else if (!want && g_stress_running) {
        g_counters.update_elapsed(now());
        g_stress_running = false;
        g_stress_paused = false;
        g_write_pending = false;
        log("stress: off");
    }
    if (!g_stress_running) return;
    const bool connected = g_mode == Mode::RunUp && wifi::link() == wifi::Link::Up;
    const nvp::diag::StressPlan p = g_schedule.next(now(), connected, wifi::hotspot_up());
    if (p.paused != g_stress_paused) {
        g_stress_paused = p.paused;
        log(p.paused ? "stress: paused, device network open" : "stress: resumed");
    }
    char line[96];
    if (p.scan) {
        if (g_scan_owner == ScanOwner::None && wifi::scan_start(false)) {
            g_scan_owner = ScanOwner::Stress;
            log("stress: scan start");
        } else {
            g_counters.record_scan(false, "scan: could not start");
            g_counters.cycle_done(now());
            log("stress: scan could not start");
        }
    }
    if (p.write) g_write_pending = true;
    if (g_write_pending && g_scan_owner == ScanOwner::None) {
        g_write_pending = false;
        const platform::storage::Result w = platform::storage::scratch_write(++g_scratch_counter);
        g_counters.record_write(w.ok, w.reason);
        if (w.ok) {
            log("stress: write ok 4096 B");
        } else {
            std::snprintf(line, sizeof line, "stress: write failed (%s)", w.reason);
            log(line);
        }
    }
    if (p.https) {
        const platform::https::Result h = platform::https::probe();
        g_counters.record_https(h.ok ? nvp::diag::HttpsOutcome::Ok : nvp::diag::HttpsOutcome::Error, h.reason);
        if (h.ok) {
            std::snprintf(line, sizeof line, "stress: https ok %d %lu B %lu.%lu s", h.status,
                          static_cast<unsigned long>(h.bytes), static_cast<unsigned long>(h.duration_ms / 1000),
                          static_cast<unsigned long>((h.duration_ms % 1000) / 100));
        } else {
            std::snprintf(line, sizeof line, "stress: %s", h.reason);
        }
        log(line);
    }
    if (p.https_skip) {
        g_counters.record_https(nvp::diag::HttpsOutcome::Skipped);
        log(g_has_wifi ? "stress: https skipped (not connected)" : "stress: https skipped (no Wi-Fi set up)");
    }
}

void publish() {
    Snapshot s;
    s.mode = g_mode;
    s.phones = g_phones;
    s.failure = g_failure;
    std::snprintf(s.ap_ssid, sizeof s.ap_ssid, "%s", g_ap_ssid);
    std::snprintf(s.attempt_ssid, sizeof s.attempt_ssid, "%s", g_attempt_ssid);
    s.has_location = g_has_location;
    s.location = g_location;
    s.wifi_up = g_mode == Mode::RunUp && wifi::link() == wifi::Link::Up;
    std::snprintf(s.ssid, sizeof s.ssid, "%s", g_ssid);
    if (s.wifi_up) {
        s.rssi = wifi::rssi();
        wifi::station_ip(s.ip);
    }
    switch (g_mode) {
        case Mode::Start:
        case Mode::RunNoRadio:
            s.line.kind = g_has_wifi ? nvp::diag::WifiKind::Off : nvp::diag::WifiKind::NotSetUp;
            break;
        case Mode::RunJoining:
        case Mode::RunBackoff:
            s.line.kind = nvp::diag::WifiKind::Joining;
            std::snprintf(s.line.ssid, sizeof s.line.ssid, "%s", g_ssid);
            break;
        case Mode::RunUp:
            s.line.kind = nvp::diag::WifiKind::Connected;
            std::snprintf(s.line.ssid, sizeof s.line.ssid, "%s", g_ssid);
            s.line.rssi_dbm = s.rssi;
            break;
        case Mode::Setup:
        case Mode::Connecting:
        case Mode::Grace:
            s.line.kind = nvp::diag::WifiKind::Hotspot;
            break;
    }
    std::snprintf(s.line.ap_ssid, sizeof s.line.ap_ssid, "%s", g_ap_ssid);
    std::snprintf(s.line.ap_address, sizeof s.line.ap_address, "%s", wifi::hotspot_address());
    s.hotspot_open = wifi::hotspot_up();
    if (s.hotspot_open || g_pending_ap) std::snprintf(s.hotspot_password, sizeof s.hotspot_password, "%s", g_ap_pass);
    std::snprintf(s.setup_message, sizeof s.setup_message, "%s", g_message);
    s.scan_completed = g_scan_done_once;
    if (g_stress_running) g_counters.update_elapsed(now());
    s.stress_running = g_stress_running;
    s.stress_paused = g_stress_paused;
    s.stress_has_run = g_stress_has_run;
    s.stress = g_counters;
    platform::tasks::Guard guard(*g_lock);
    g_pub = s;
}

void worker(void*) {
    // Secrets first, before any scan can run (analysis U2, FR-018): the access-point password and
    // the settings PIN are generated once and kept.
    g_has_wifi = wifi::load_stored(g_ssid, g_pass);  // also opens NVS
    platform::settings::ensure_pin();
    wifi::hotspot_ssid(g_ap_ssid);
    wifi::hotspot_password(g_ap_pass);
    g_has_location = platform::settings::load_location(g_location);
    apply(Event::Boot);
    for (;;) {
        bool setup = false, cancel = false, start_over = false, stress = false, t_write = false;
        bool t_scan = false, t_https = false, t_mix = false;
        {
            platform::tasks::Guard guard(*g_lock);
            setup = g_req_setup;
            cancel = g_req_cancel;
            start_over = g_req_start_over;
            stress = g_stress_on;
            t_write = g_req_test_write;
            t_scan = g_req_test_scan;
            t_https = g_req_test_https;
            t_mix = g_req_test_mix;
            g_req_setup = g_req_cancel = g_req_start_over = g_req_test_write = g_req_test_scan = false;
            g_req_test_https = g_req_test_mix = false;
        }
        if (setup) apply(Event::DevSetup);
        if (cancel) apply(Event::DevCancel);
        if (start_over) apply(Event::StartOver);
        if (t_mix) {  // the order of one stress cycle: scan start, then the write
            t_scan = true;
            t_write = true;
        }
        if (t_scan) {
            if (g_scan_owner == ScanOwner::None && wifi::scan_start(false)) {
                g_scan_owner = ScanOwner::Test;
                log("test: scan start");
            } else {
                log("test: scan could not start (another scan running)");
            }
        }
        if (t_write) {
            const uint32_t t0 = now();
            log("test: write start");
            const platform::storage::Result r = platform::storage::scratch_write(++g_scratch_counter);
            char line[64];
            std::snprintf(line, sizeof line, "test: write %s 4096 B in %lu ms", r.ok ? "ok" : "failed",
                          static_cast<unsigned long>(now() - t0));
            log(line);
        }
        if (t_https) {
            log("test: https start");
            const platform::https::Result h = platform::https::probe();
            char line[80];
            if (h.ok) {
                std::snprintf(line, sizeof line, "test: https ok %d %lu B in %lu ms", h.status,
                              static_cast<unsigned long>(h.bytes), static_cast<unsigned long>(h.duration_ms));
            } else {
                std::snprintf(line, sizeof line, "test: https failed (%s)", h.reason);
            }
            log(line);
        }
        drive_wifi();
        platform::portal::handle();
        drive_scans();
        drive_stress(stress);
        publish();
        platform::tasks::sleep_ms(kTickMs);
    }
}

}  // namespace

void start(bool silence_ok) {
    if (g_lock != nullptr) return;
    g_lock = new platform::tasks::Lock();
    g_silence_ok = silence_ok;
    if (!platform::tasks::start_worker("net", kCore, kStackBytes, worker, nullptr)) {
        platform::console::write("net: worker could not start\n");
    }
}

bool request_setup() {
    if (!g_silence_ok || g_lock == nullptr) return false;
    platform::tasks::Guard guard(*g_lock);
    g_req_setup = true;
    return true;
}

void request_cancel() {
    if (g_lock == nullptr) return;
    platform::tasks::Guard guard(*g_lock);
    g_req_cancel = true;
}

void request_start_over() {
    if (g_lock == nullptr) return;
    platform::tasks::Guard guard(*g_lock);
    g_req_start_over = true;
}

bool request_stress(bool on) {
    if (on && (!g_silence_ok || g_lock == nullptr)) return false;
    if (g_lock == nullptr) return true;
    platform::tasks::Guard guard(*g_lock);
    g_stress_on = on;
    if (!on) {
        g_pub.stress_running = false;  // the report printed right after `stress off` says so
        g_pub.stress_paused = false;
    }
    return true;
}

void request_test_write() {
    if (g_lock == nullptr) return;
    platform::tasks::Guard guard(*g_lock);
    g_req_test_write = true;
}

bool request_test_scan() {
    if (!g_silence_ok || g_lock == nullptr) return false;
    platform::tasks::Guard guard(*g_lock);
    g_req_test_scan = true;
    return true;
}

bool request_test_https() {
    if (!g_silence_ok || g_lock == nullptr) return false;
    platform::tasks::Guard guard(*g_lock);
    g_req_test_https = true;
    return true;
}

bool request_test_mix() {
    if (!g_silence_ok || g_lock == nullptr) return false;
    platform::tasks::Guard guard(*g_lock);
    g_req_test_mix = true;
    return true;
}

Snapshot snapshot() {
    if (g_lock == nullptr) return Snapshot{};
    platform::tasks::Guard guard(*g_lock);
    return g_pub;
}

}  // namespace app::net
