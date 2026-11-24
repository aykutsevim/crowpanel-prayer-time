#include "bringup_app.h"

#include <cstdio>
#include <cstring>

#include "app/net_worker.h"
#include "app/serial_console.h"
#include "diag/boot_screen.h"
#include "diag/diag_screen.h"
#include "diag/settings_screen.h"
#include "diag/view_model.h"
#include "nvp/diag/report.h"
#include "nvp/diag/selftest.h"
#include "nvp/diag/touch_judge.h"
#include "nvp/version.h"
#include "platform/audio_off.h"
#include "platform/backlight.h"
#include "platform/board.h"
#include "platform/console_io.h"
#include "platform/display.h"
#include "platform/storage.h"
#include "platform/sysinfo.h"
#include "platform/touch_gt911.h"

namespace app {
namespace {

using nvp::diag::index;
using nvp::diag::Outcome;
using nvp::diag::SelfTestResult;
using nvp::diag::TestName;
namespace ui = nvp::ui::diag;

constexpr const char* kSilenceRefusal = "refused: silence step not acknowledged\n";
constexpr uint32_t kViewRefreshMs = 100;
constexpr uint32_t kConfirmTimeoutMs = 15000;  // FR-019a

SelfTestResult g_tests[nvp::diag::kTestCount];
char g_report[2048];
nvp::diag::TouchJudge g_judge;
uint32_t g_boot_to_diag_ms = 0;
bool g_display_ok = false;

lv_obj_t* g_diag_screen = nullptr;
lv_obj_t* g_settings_screen = nullptr;
ui::ActiveScreen g_screen = ui::ActiveScreen::Boot;
ui::Confirm g_confirm = ui::Confirm::None;
uint32_t g_confirm_since = 0;

struct TouchState {
    bool pressed = false;
    int16_t x = 0;
    int16_t y = 0;
};
TouchState g_touch;
char g_status[48] = "";

nvp::diag::SilenceLine silence_line() {
    const platform::silence::SilenceState& s = platform::silence::silence_state();
    nvp::diag::SilenceLine line;
    line.ran = s.ran;
    line.acknowledged = s.acknowledged;
    line.ran_first = s.ran_first;
    line.attempts = s.attempts;
    std::snprintf(line.reason, sizeof line.reason, "%s", s.reason);
    return line;
}

// The Wi-Fi self-test follows the worker: passed once the radio joined a network or finished a
// scan (data-model.md SelfTestResult).
void update_wifi_test(const net::Snapshot& n) {
    SelfTestResult& t = g_tests[index(TestName::WiFi)];
    if (!can_use_radio_and_load()) {
        nvp::diag::set_result(t, Outcome::Skipped, "silence step not acknowledged");
    } else if (n.scan_completed || n.line.kind == nvp::diag::WifiKind::Connected) {
        nvp::diag::set_result(t, Outcome::Passed, "");
    } else if (n.line.kind == nvp::diag::WifiKind::NotSetUp) {
        nvp::diag::set_result(t, Outcome::Skipped, "no Wi-Fi set up");
    } else {
        nvp::diag::set_result(t, Outcome::Skipped, "no scan or connection yet");
    }
}

void print_report() {
    const net::Snapshot n = net::snapshot();
    update_wifi_test(n);
    nvp::diag::ReportInput in;
    in.firmware = nvp::kFirmwareVersion;
    in.model = board::kModel;
    in.revision = board::kRevision;
    in.revision_detected = false;  // the firmware cannot read the board revision
    const platform::sysinfo::Snapshot snap = platform::sysinfo::read();
    in.flash_bytes = snap.flash_bytes;
    in.psram_bytes = snap.psram_bytes;
    in.silence = silence_line();
    for (size_t i = 0; i < nvp::diag::kTestCount; ++i) in.tests[i] = g_tests[i];
    in.wifi = n.line;  // names and address only: no password field exists (FR-024)
    for (size_t i = 0; i < nvp::diag::kCornerCount; ++i) in.corners[i] = g_judge.results()[i];
    in.heap_internal_free = snap.internal_free;
    in.heap_internal_largest = snap.internal_largest;
    in.heap_psram_free = snap.psram_free;
    in.stress_running = n.stress_running;
    in.stress_paused = n.stress_paused;
    in.stress_has_run = n.stress_has_run;
    in.stress = n.stress;
    in.boot_to_diag_ms = g_boot_to_diag_ms;
    in.uptime_s = platform::sysinfo::uptime_ms() / 1000;
    in.reset_reason = snap.reset_reason;
    in.coredump_bytes = snap.coredump_bytes;
    nvp::diag::report::format(in, g_report, sizeof g_report);
    platform::console::write(g_report);
}

void set_status(const char* text) { std::snprintf(g_status, sizeof g_status, "%s", text); }

// --- Actions shared by the screen and the serial console (serial acts without confirmation) ---

void stress(bool on) {
    if (!net::request_stress(on)) {
        platform::console::write(kSilenceRefusal);
        set_status("Refused: silence not acknowledged");
        return;
    }
    set_status(on ? "Stress on" : "Stress off");
    if (!on) print_report();  // final counters (contracts/serial-console.md)
}

void wifi_setup() {
    if (!net::request_setup()) {
        platform::console::write(kSilenceRefusal);
        set_status("Refused: silence not acknowledged");
        return;
    }
    set_status("Starting the hotspot");
}

void cmd_stress_on() { stress(true); }
void cmd_stress_off() { stress(false); }
void cmd_wifi_setup() { wifi_setup(); }
void cmd_wifi_cancel() { net::request_cancel(); }
void cmd_wifi_forget() { net::request_forget(); }
void cmd_test_write() {
    net::request_test_write();
    set_status("Test: write");  // shown on screen so the owner can match what they see
}
void cmd_test_scan() {
    if (net::request_test_scan()) {
        set_status("Test: scan");
    } else {
        platform::console::write(kSilenceRefusal);
    }
}

void cmd_test_https() {
    if (net::request_test_https()) {
        set_status("Test: https");
    } else {
        platform::console::write(kSilenceRefusal);
    }
}
void cmd_test_mix() {
    if (net::request_test_mix()) {
        set_status("Test: mix");
    } else {
        platform::console::write(kSilenceRefusal);
    }
}

const ConsoleHandlers kHandlers = [] {
    ConsoleHandlers h;
    h.report = print_report;
    h.stress_on = cmd_stress_on;
    h.stress_off = cmd_stress_off;
    h.wifi_setup = cmd_wifi_setup;
    h.wifi_cancel = cmd_wifi_cancel;
    h.wifi_forget = cmd_wifi_forget;
    h.test_write = cmd_test_write;
    h.test_scan = cmd_test_scan;
    h.test_https = cmd_test_https;
    h.test_mix = cmd_test_mix;
    return h;
}();

void show(ui::ActiveScreen screen) {
    g_screen = screen;
    g_confirm = ui::Confirm::None;
    lv_screen_load(screen == ui::ActiveScreen::Settings ? g_settings_screen : g_diag_screen);
}

void open_confirm(ui::Confirm c) {
    g_confirm = c;
    g_confirm_since = platform::sysinfo::uptime_ms();
}

// One tap can be reported twice; two toggles in a row would cancel each other before the worker
// sees them (owner observation 2026-11-23). Ignore a second toggle within one second.
void on_stress_button(void*) {
    static uint32_t last = 0;
    const uint32_t now = platform::sysinfo::uptime_ms();
    if (last != 0 && now - last < 1000) return;
    last = now;
    stress(!net::snapshot().stress_running);  // the worker publishes within 20 ms
}
void on_settings_button(void*) { show(ui::ActiveScreen::Settings); }
void on_request_setup(void*) { open_confirm(ui::Confirm::StartSetup); }
void on_request_forget(void*) { open_confirm(ui::Confirm::Forget); }
void on_confirm_no(void*) { g_confirm = ui::Confirm::None; }
void on_cancel_setup(void*) { net::request_cancel(); }
void on_back(void*) { show(ui::ActiveScreen::Diagnostic); }

void on_confirm_yes(void*) {
    const ui::Confirm c = g_confirm;
    g_confirm = ui::Confirm::None;
    if (c == ui::Confirm::StartSetup) wifi_setup();
    if (c == ui::Confirm::Forget) net::request_forget();
}

// Raw touch for LVGL and for the corner judge (FR-014, FR-014a). Corners are judged on the
// touch-down point, on the diagnostic screen only. No sound or vibration exists (SIL-06).
bool read_touch(int16_t* x, int16_t* y) {
    const bool pressed = platform::touch::read_point(x, y);
    if (pressed && !g_touch.pressed && g_screen == ui::ActiveScreen::Diagnostic) g_judge.judge(*x, *y);
    g_touch.pressed = pressed;
    g_touch.x = *x;
    g_touch.y = *y;
    return pressed;
}

ui::Edge usb_edge() {
    switch (board::kUsbCEdge) {
        case board::PanelEdge::Left: return ui::Edge::Left;
        case board::PanelEdge::Right: return ui::Edge::Right;
        case board::PanelEdge::Top: return ui::Edge::Top;
        case board::PanelEdge::Bottom: return ui::Edge::Bottom;
    }
    return ui::Edge::Left;
}

void update_view() {
    const net::Snapshot n = net::snapshot();
    update_wifi_test(n);
    const uint32_t now = platform::sysinfo::uptime_ms();
    if (g_confirm != ui::Confirm::None && now - g_confirm_since >= kConfirmTimeoutMs) {
        g_confirm = ui::Confirm::None;  // unanswered: nothing changes (FR-019a)
    }

    ui::DiagViewModel vm;
    vm.firmware = nvp::kFirmwareVersion;
    vm.model = board::kModel;
    vm.revision = board::kRevision;
    vm.silence = silence_line();
    for (size_t i = 0; i < nvp::diag::kTestCount; ++i) vm.tests[i] = g_tests[i];
    vm.wifi = n.line;
    const platform::sysinfo::Snapshot snap = platform::sysinfo::read();
    vm.heap_internal_free = snap.internal_free;
    vm.heap_psram_free = snap.psram_free;
    vm.stress.running = n.stress_running;
    vm.stress.paused = n.stress_paused;
    vm.stress.elapsed_s = n.stress.elapsed_s;
    vm.stress.cycles = n.stress.cycles;
    vm.stress.scan_ok = n.stress.scan_ok;
    vm.stress.scan_err = n.stress.scan_err;
    vm.stress.write_ok = n.stress.write_ok;
    vm.stress.write_err = n.stress.write_err;
    vm.stress.https_ok = n.stress.https_ok;
    vm.stress.https_err = n.stress.https_err;
    vm.stress.https_skip = n.stress.https_skip;
    std::snprintf(vm.stress.last_error, sizeof vm.stress.last_error, "%s", n.stress.last_error);
    vm.touch_active = g_touch.pressed && g_screen == ui::ActiveScreen::Diagnostic;
    vm.touch_x = g_touch.x;
    vm.touch_y = g_touch.y;
    for (size_t i = 0; i < nvp::diag::kCornerCount; ++i) vm.corners[i] = g_judge.results()[i];
    vm.heartbeat = (now / 1000) % 2 == 0;
    vm.screen = g_screen;
    vm.confirm = g_confirm;
    std::snprintf(vm.status, sizeof vm.status, "%s", g_status);
    vm.hotspot_open = n.hotspot_open;
    std::snprintf(vm.hotspot_password, sizeof vm.hotspot_password, "%s", n.hotspot_password);  // screen only
    std::snprintf(vm.setup_message, sizeof vm.setup_message, "%s", n.setup_message);
    if (g_screen == ui::ActiveScreen::Settings) {
        ui::settings_screen_update(vm);
    } else {
        ui::diag_screen_update(vm);
    }
}

}  // namespace

bool can_use_radio_and_load() { return platform::silence::silence_state().acknowledged; }

void print_silence_line() {
    char line[96];
    nvp::diag::report::format_silence_line(silence_line(), line, sizeof line);
    platform::console::write(line);
}

void start() {
    // Fixed system facts, read once before the display starts (no flash access later, research R8).
    platform::sysinfo::init();

    // Silence self-test: the result of the step that already ran first (FR-001a).
    const platform::silence::SilenceState& s = platform::silence::silence_state();
    if (s.acknowledged) {
        nvp::diag::set_result(g_tests[index(TestName::Silence)], Outcome::Passed, "");
    } else {
        nvp::diag::set_result(g_tests[index(TestName::Silence)], Outcome::Failed, s.reason);
    }

    // Step 3: backlight fully on, the only level (OD1, FR-017).
    if (platform::backlight::full_on()) {
        nvp::diag::set_result(g_tests[index(TestName::Backlight)], Outcome::Passed, "");
    } else {
        nvp::diag::set_result(g_tests[index(TestName::Backlight)], Outcome::Failed, "byte 0 not acknowledged");
    }

    // Step 4: display and boot screen. A failure leaves everything else running (spec edge case).
    const platform::display::Result d = platform::display::begin();
    g_display_ok = d.ok;
    if (d.ok) {
        nvp::diag::set_result(g_tests[index(TestName::Display)], Outcome::Passed, "");
        lv_screen_load(ui::boot_screen_create(nvp::kFirmwareVersion, board::kModel, board::kRevision));
        platform::display::refresh_now();
    } else {
        nvp::diag::set_result(g_tests[index(TestName::Display)], Outcome::Failed, d.reason);
    }

    // Step 5: touch start-up (bounded, research R5), while the boot screen shows.
    const platform::touch::Result t = platform::touch::begin();
    nvp::diag::set_result(g_tests[index(TestName::Touch)], t.ok ? Outcome::Passed : Outcome::Failed, t.reason);
    if (t.ok) platform::display::attach_touch(read_touch);

    // Step 6: the diagnostic screen (SC-006); the settings screen is created with it.
    nvp::diag::set_result(g_tests[index(TestName::WiFi)], Outcome::Skipped, "no scan or connection yet");
    if (d.ok) {
        ui::DiagActions actions;
        actions.stress_toggle = on_stress_button;
        actions.open_settings = on_settings_button;
        actions.request_setup = on_request_setup;
        actions.request_forget = on_request_forget;
        actions.confirm_yes = on_confirm_yes;
        actions.confirm_no = on_confirm_no;
        actions.cancel_setup = on_cancel_setup;
        actions.back = on_back;
        g_diag_screen = ui::diag_screen_create(usb_edge(), actions);
        g_settings_screen = ui::settings_screen_create(actions);
        g_screen = ui::ActiveScreen::Diagnostic;
        update_view();
        lv_screen_load(g_diag_screen);
        platform::display::refresh_now();
        g_boot_to_diag_ms = platform::sysinfo::uptime_ms();
        platform::console::printf("[%8lu] diag screen shown\n", static_cast<unsigned long>(g_boot_to_diag_ms));
    }

    // Step 7: storage mount and self-test (after the screen, a first-boot format can take seconds).
    platform::storage::Result r = platform::storage::mount();
    if (r.ok) r = platform::storage::self_test();
    nvp::diag::set_result(g_tests[index(TestName::Storage)], r.ok ? Outcome::Passed : Outcome::Failed, r.reason);

    // Step 9: the report (FR-026).
    print_report();

    // Steps 8 and 10: the network worker on core 0 reads the stored details and joins in the
    // background, only if the silence step passed (FR-001a, FR-023).
    net::start(can_use_radio_and_load());
}

void loop() {
    serial_console_poll(kHandlers);
    if (!g_display_ok) return;
    static uint32_t last = 0;
    const uint32_t now = platform::sysinfo::uptime_ms();
    if (now - last >= kViewRefreshMs) {
        last = now;
        update_view();
    }
    platform::display::handle();
}

}  // namespace app
