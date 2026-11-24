#include "net_worker.h"

#include <cstdio>
#include <cstring>

#include "nvp/diag/backoff.h"
#include "nvp/diag/portal_json.h"
#include "platform/console_io.h"
#include "platform/https_probe.h"
#include "platform/setup_portal.h"
#include "platform/storage.h"
#include "platform/sysinfo.h"
#include "platform/tasks.h"
#include "platform/wifi_link.h"

namespace app::net {
namespace {

using nvp::diag::WifiEvent;
using nvp::diag::WifiState;
namespace wifi = platform::wifi;

constexpr int kCore = 0;                 // network work on core 0, display on core 1 (research R8)
constexpr uint32_t kStackBytes = 12288;  // TLS needs a deep stack
constexpr uint32_t kTickMs = 20;
constexpr uint32_t kJoinTimeoutMs = 20000;  // MODE-04
constexpr uint32_t kGraceMs = 5000;         // contracts/setup-portal.md
constexpr uint32_t kPortalScanMs = 15000;

// --- Requests and the published snapshot: shared between the cores, under g_lock ---
platform::tasks::Lock* g_lock = nullptr;
bool g_req_setup = false;
bool g_req_cancel = false;
bool g_req_forget = false;
bool g_stress_on = false;  // desired; the worker starts and stops the run
bool g_req_test_write = false;
bool g_req_test_scan = false;
bool g_req_test_https = false;
bool g_req_test_mix = false;
Snapshot g_pub;

// --- Worker-only state (core 0) ---
bool g_silence_ok = false;
WifiState g_state = WifiState::Idle;
WifiState g_previous = WifiState::Idle;
nvp::diag::Backoff g_backoff;
bool g_has_stored = false;
char g_ssid[33] = {};
char g_pass[64] = {};  // never logged, printed or served (FR-024)
bool g_submitted = false;
char g_sub_ssid[33] = {};
char g_sub_pass[64] = {};
bool g_last_try_failed = false;
uint32_t g_join_start = 0, g_backoff_until = 0, g_try_start = 0, g_grace_until = 0;
char g_ap_ssid[33] = {};
char g_ap_pass[9] = {};
char g_message[64] = {};
bool g_scan_done_once = false;
// Scan cache for the setup page.
nvp::diag::ScanEntry g_scan[32];
size_t g_scan_count = 0;
uint32_t g_last_portal_scan = 0;
enum class ScanOwner { None, Portal, Stress, Test } g_scan_owner = ScanOwner::None;
// Stress.
bool g_stress_running = false, g_stress_paused = false, g_stress_has_run = false;
nvp::diag::StressSchedule g_schedule;
nvp::diag::StressCounters g_counters;
uint32_t g_scratch_counter = 0;
// A flash write started while a Wi-Fi scan runs garbles the panel; a write alone only flickers
// once (stack decision log, 2026-11-23, `test mix`). Stress writes therefore wait for the scan.
bool g_write_pending = false;

uint32_t now() { return platform::sysinfo::uptime_ms(); }

void log(const char* what) {
    platform::console::printf("[%8lu] %s\n", static_cast<unsigned long>(now()), what);
}

void set_message(const char* fmt, const char* arg = "") {
    std::snprintf(g_message, sizeof g_message, fmt, arg);
}

void clear_submitted() {
    g_submitted = false;
    std::memset(g_sub_ssid, 0, sizeof g_sub_ssid);
    std::memset(g_sub_pass, 0, sizeof g_sub_pass);
}

void apply(WifiEvent event) {
    const WifiState before = g_state;
    nvp::diag::WifiContext ctx;
    ctx.silence_ok = g_silence_ok;
    ctx.has_stored = g_has_stored;
    const nvp::diag::WifiStep step = nvp::diag::wifi_step(g_state, g_previous, event, ctx);
    const nvp::diag::WifiAction a = step.action;
    char line[96];

    if (a & nvp::diag::kActRefused) {
        set_message("Refused: silence step not acknowledged");
        log("wifi: refused, silence step not acknowledged");
    }
    if (a & nvp::diag::kActResetBackoff) g_backoff.reset();
    if (a & nvp::diag::kActStopAp) {
        platform::portal::stop();
        wifi::stop_hotspot();
        g_grace_until = 0;
        log("wifi: hotspot closed");
    }
    if (a & nvp::diag::kActDiscardSubmitted) {
        if (event == WifiEvent::TryFail) {
            g_last_try_failed = true;
            set_message("Connection failed");
            log("wifi: connection failed; details discarded");
            wifi::disconnect();
        }
        clear_submitted();
    }
    if (a & nvp::diag::kActEraseStored) {
        wifi::erase_stored();
        g_has_stored = false;
        std::memset(g_ssid, 0, sizeof g_ssid);
        std::memset(g_pass, 0, sizeof g_pass);
        set_message("Wi-Fi details forgotten");
        log("wifi: stored details erased");
    }
    if (a & nvp::diag::kActDisconnect) wifi::disconnect();
    if (a & nvp::diag::kActStartAp) {
        g_previous = before;
        wifi::disconnect();
        wifi::hotspot_ssid(g_ap_ssid);
        wifi::hotspot_password(g_ap_pass);
        if (wifi::start_hotspot(g_ap_ssid, g_ap_pass)) {
            platform::portal::Callbacks cb;
            cb.submit = [](const char* ssid, const char* pass) {
                if (g_state != WifiState::SetupAP || !nvp::diag::valid_credentials(ssid, pass)) return false;
                std::snprintf(g_sub_ssid, sizeof g_sub_ssid, "%s", ssid);
                std::snprintf(g_sub_pass, sizeof g_sub_pass, "%s", pass);
                g_submitted = true;
                return true;
            };
            cb.status_json = [](char* out, size_t cap) {
                nvp::diag::PortalState s = nvp::diag::PortalState::Setup;
                if (g_state == WifiState::Trying) s = nvp::diag::PortalState::Trying;
                else if (g_state == WifiState::Connected) s = nvp::diag::PortalState::Connected;
                else if (g_last_try_failed) s = nvp::diag::PortalState::Failed;
                const char* ssid = g_state == WifiState::Connected ? g_ssid : g_sub_ssid;
                nvp::diag::build_status_json(s, ssid, out, cap);
            };
            cb.scan_json = [](char* out, size_t cap) {
                nvp::diag::ScanEntry copy[32];
                std::memcpy(copy, g_scan, sizeof copy);
                nvp::diag::build_scan_json(copy, g_scan_count, out, cap);
            };
            platform::portal::start(cb);
            g_last_portal_scan = 0;
            g_last_try_failed = false;
            set_message("Waiting for details from the phone");
            std::snprintf(line, sizeof line, "wifi: hotspot %s open at %s", g_ap_ssid, wifi::hotspot_address());
            log(line);  // name and address only, never the password (FR-024)
        } else {
            set_message("Hotspot failed to start");
            log("wifi: hotspot failed to start");
        }
    }
    if (a & nvp::diag::kActTrySubmitted) {
        g_last_try_failed = false;
        wifi::disconnect();
        wifi::join(g_sub_ssid, g_sub_pass);
        g_try_start = now();
        set_message("Connecting to %s...", g_sub_ssid);
        std::snprintf(line, sizeof line, "wifi: trying %s", g_sub_ssid);
        log(line);
    }
    if (a & nvp::diag::kActStoreSubmitted) {
        if (wifi::store(g_sub_ssid, g_sub_pass)) {
            std::snprintf(g_ssid, sizeof g_ssid, "%s", g_sub_ssid);
            std::snprintf(g_pass, sizeof g_pass, "%s", g_sub_pass);
            g_has_stored = true;
            set_message("Connected to %s", g_ssid);
            std::snprintf(line, sizeof line, "wifi: connected to %s; details stored", g_ssid);
        } else {
            set_message("Connected, but the details could not be stored");
            std::snprintf(line, sizeof line, "wifi: connected to %s; storing failed", g_sub_ssid);
        }
        log(line);
        clear_submitted();
    }
    if (a & nvp::diag::kActStopApAfterGrace) g_grace_until = now() + kGraceMs;
    if (a & nvp::diag::kActStartJoin) {
        wifi::join(g_ssid, g_pass);
        g_join_start = now();
        std::snprintf(line, sizeof line, "wifi: joining %s", g_ssid);
        log(line);
    }
    if (a & nvp::diag::kActStartBackoff) {
        const uint32_t d = g_backoff.next_delay_ms();
        g_backoff_until = now() + d;
        wifi::disconnect();
        std::snprintf(line, sizeof line, "wifi: not connected; retry in %lu s", static_cast<unsigned long>(d / 1000));
        log(line);
    }
    if (event == WifiEvent::JoinOk) {
        std::snprintf(line, sizeof line, "wifi: connected to %s", g_ssid);
        log(line);
        g_message[0] = '\0';
    }
    g_state = step.next;
}

void drive_wifi() {
    const uint32_t t = now();
    switch (g_state) {
        case WifiState::Joining: {
            const wifi::Link l = wifi::link();
            if (l == wifi::Link::Up) apply(WifiEvent::JoinOk);
            else if (l == wifi::Link::Failed || t - g_join_start > kJoinTimeoutMs) apply(WifiEvent::JoinFail);
            break;
        }
        case WifiState::Connected:
            if (g_grace_until == 0 && wifi::link() != wifi::Link::Up) apply(WifiEvent::LinkLost);
            break;
        case WifiState::Backoff:
            if (static_cast<int32_t>(t - g_backoff_until) >= 0) apply(WifiEvent::BackoffElapsed);
            break;
        case WifiState::SetupAP:
            if (g_submitted) apply(WifiEvent::Submit);
            break;
        case WifiState::Trying: {
            const wifi::Link l = wifi::link();
            if (l == wifi::Link::Up) apply(WifiEvent::TryOk);
            else if (l == wifi::Link::Failed || t - g_try_start > kJoinTimeoutMs) apply(WifiEvent::TryFail);
            break;
        }
        case WifiState::Idle: break;
    }
    if (g_grace_until != 0 && static_cast<int32_t>(t - g_grace_until) >= 0) {
        platform::portal::stop();
        wifi::stop_hotspot();
        g_grace_until = 0;
        log("wifi: hotspot closed after the grace period");
    }
}

void drive_scans() {
    if (g_scan_owner != ScanOwner::None) {
        const int r = wifi::scan_poll();
        if (r == -1) return;
        const bool ok = r >= 0;
        size_t n = 0;
        if (ok) {
            nvp::diag::ScanEntry fresh[32];
            n = wifi::scan_take(fresh, 32);
            if (g_scan_owner == ScanOwner::Portal || g_scan_owner == ScanOwner::Stress) {
                std::memcpy(g_scan, fresh, sizeof g_scan);
                g_scan_count = n;
            }
            g_scan_done_once = true;
        }
        if (g_scan_owner == ScanOwner::Test) {
            char line[64];
            if (ok) {
                std::snprintf(line, sizeof line, "test: scan done %u networks", static_cast<unsigned>(n));
            } else {
                std::snprintf(line, sizeof line, "test: scan failed");
            }
            log(line);
        }
        if (g_scan_owner == ScanOwner::Stress) {
            char line[64];
            if (ok) {
                std::snprintf(line, sizeof line, "stress: scan done %u networks", static_cast<unsigned>(n));
            } else {
                std::snprintf(line, sizeof line, "stress: scan failed");
            }
            log(line);
            g_counters.record_scan(ok, ok ? nullptr : "scan: failed");
            g_counters.cycle_done(now());
        }
        g_scan_owner = ScanOwner::None;
        return;
    }
    // While the hotspot is open, refresh the list for the setup page (not while trying).
    if (wifi::hotspot_up() && g_state == WifiState::SetupAP &&
        (g_last_portal_scan == 0 || now() - g_last_portal_scan >= kPortalScanMs)) {
        g_last_portal_scan = now();
        if (wifi::scan_start()) g_scan_owner = ScanOwner::Portal;
    }
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
    const bool connected = g_state == WifiState::Connected && wifi::link() == wifi::Link::Up;
    const nvp::diag::StressPlan p = g_schedule.next(now(), connected, wifi::hotspot_up());
    if (p.paused != g_stress_paused) {
        g_stress_paused = p.paused;
        log(p.paused ? "stress: paused, hotspot open" : "stress: resumed");
    }
    char line[96];
    if (p.scan) {
        if (g_scan_owner == ScanOwner::None && wifi::scan_start()) {
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
        log(g_has_stored ? "stress: https skipped (not connected)" : "stress: https skipped (no Wi-Fi set up)");
    }
}

void publish() {
    Snapshot s;
    s.state = g_state;
    switch (g_state) {
        case WifiState::Idle:
            s.line.kind = g_has_stored ? nvp::diag::WifiKind::Off : nvp::diag::WifiKind::NotSetUp;
            break;
        case WifiState::Joining:
        case WifiState::Backoff:
            s.line.kind = nvp::diag::WifiKind::Joining;
            std::snprintf(s.line.ssid, sizeof s.line.ssid, "%s", g_ssid);
            break;
        case WifiState::Connected:
            s.line.kind = nvp::diag::WifiKind::Connected;
            std::snprintf(s.line.ssid, sizeof s.line.ssid, "%s", g_ssid);
            s.line.rssi_dbm = wifi::rssi();
            break;
        case WifiState::SetupAP:
        case WifiState::Trying:
            s.line.kind = nvp::diag::WifiKind::Hotspot;
            break;
    }
    std::snprintf(s.line.ap_ssid, sizeof s.line.ap_ssid, "%s", g_ap_ssid);
    std::snprintf(s.line.ap_address, sizeof s.line.ap_address, "%s", wifi::hotspot_address());
    s.hotspot_open = wifi::hotspot_up();
    if (s.hotspot_open) std::snprintf(s.hotspot_password, sizeof s.hotspot_password, "%s", g_ap_pass);
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
    g_has_stored = wifi::load_stored(g_ssid, g_pass);
    apply(WifiEvent::Boot);  // joins in the background if details are stored and silence passed
    for (;;) {
        bool setup = false, cancel = false, forget = false, stress = false, t_write = false, t_scan = false;
        bool t_https = false, t_mix = false;
        {
            platform::tasks::Guard guard(*g_lock);
            setup = g_req_setup;
            cancel = g_req_cancel;
            forget = g_req_forget;
            stress = g_stress_on;
            t_write = g_req_test_write;
            t_scan = g_req_test_scan;
            t_https = g_req_test_https;
            t_mix = g_req_test_mix;
            g_req_setup = g_req_cancel = g_req_forget = g_req_test_write = g_req_test_scan = false;
            g_req_test_https = g_req_test_mix = false;
        }
        if (setup) apply(WifiEvent::SetupConfirmed);
        if (cancel) apply(WifiEvent::Cancel);
        if (forget) apply(WifiEvent::Forget);
        if (t_mix) {  // the order of one stress cycle: scan start, then the write
            t_scan = true;
            t_write = true;
        }
        if (t_scan) {
            if (g_scan_owner == ScanOwner::None && wifi::scan_start()) {
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

void request_forget() {
    if (g_lock == nullptr) return;
    platform::tasks::Guard guard(*g_lock);
    g_req_forget = true;
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
