#include "bringup_app.h"

#include <cstdio>
#include <cstring>

#include "app/serial_console.h"
#include "diag/boot_screen.h"
#include "diag/diag_screen.h"
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

constexpr const char* kNotInThisStage = "not in this stage";
constexpr const char* kSilenceRefusal = "refused: silence step not acknowledged\n";
constexpr uint32_t kViewRefreshMs = 100;

SelfTestResult g_tests[nvp::diag::kTestCount];
char g_report[2048];
nvp::diag::TouchJudge g_judge;
uint32_t g_boot_to_diag_ms = 0;
bool g_diag_shown = false;

// Latest raw touch, written by the touch callback (LVGL, same core) and read by the view model.
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

void print_report() {
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
    in.wifi.kind = nvp::diag::WifiKind::Off;  // Stage 3
    for (size_t i = 0; i < nvp::diag::kCornerCount; ++i) in.corners[i] = g_judge.results()[i];
    in.heap_internal_free = snap.internal_free;
    in.heap_internal_largest = snap.internal_largest;
    in.heap_psram_free = snap.psram_free;
    in.stress_running = false;  // Stage 3
    in.boot_to_diag_ms = g_boot_to_diag_ms;
    in.uptime_s = platform::sysinfo::uptime_ms() / 1000;
    nvp::diag::report::format(in, g_report, sizeof g_report);
    platform::console::write(g_report);
}

void set_status(const char* text) { std::snprintf(g_status, sizeof g_status, "%s", text); }

// FR-001a: stress and Wi-Fi refuse while silence is not acknowledged. Neither exists before
// Stage 3, so an acknowledged silence step still gets "not available in this stage".
void start_needing_silence() {
    if (!can_use_radio_and_load()) {
        platform::console::write(kSilenceRefusal);
        set_status("Refused: silence not acknowledged");
        return;
    }
    platform::console::write("not available in this stage\n");
    set_status("Not available in this stage");
}

void not_in_this_stage() { platform::console::write("not available in this stage\n"); }

const ConsoleHandlers kHandlers = [] {
    ConsoleHandlers h;
    h.report = print_report;
    h.stress_on = start_needing_silence;
    h.stress_off = not_in_this_stage;
    h.wifi_setup = start_needing_silence;
    h.wifi_cancel = not_in_this_stage;
    h.wifi_forget = not_in_this_stage;
    return h;
}();

void on_stress_button(void*) { start_needing_silence(); }
void on_settings_button(void*) { set_status("Settings: not in this stage"); }

// Raw touch for LVGL and for the corner judge (FR-014, FR-014a). A corner is judged on the
// touch-down point. No sound or vibration exists to give (SIL-06).
bool read_touch(int16_t* x, int16_t* y) {
    const bool pressed = platform::touch::read_point(x, y);
    if (pressed && !g_touch.pressed) g_judge.judge(*x, *y);
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
    ui::DiagViewModel vm;
    vm.firmware = nvp::kFirmwareVersion;
    vm.model = board::kModel;
    vm.revision = board::kRevision;
    vm.silence = silence_line();
    for (size_t i = 0; i < nvp::diag::kTestCount; ++i) vm.tests[i] = g_tests[i];
    vm.wifi.kind = nvp::diag::WifiKind::Off;
    const platform::sysinfo::Snapshot snap = platform::sysinfo::read();
    vm.heap_internal_free = snap.internal_free;
    vm.heap_psram_free = snap.psram_free;
    vm.touch_active = g_touch.pressed;
    vm.touch_x = g_touch.x;
    vm.touch_y = g_touch.y;
    for (size_t i = 0; i < nvp::diag::kCornerCount; ++i) vm.corners[i] = g_judge.results()[i];
    vm.heartbeat = (platform::sysinfo::uptime_ms() / 1000) % 2 == 0;
    vm.screen = ui::ActiveScreen::Diagnostic;
    std::snprintf(vm.status, sizeof vm.status, "%s", g_status);
    ui::diag_screen_update(vm);
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

    // Step 6: the diagnostic screen (SC-006).
    if (d.ok) {
        ui::DiagActions actions;
        actions.stress_toggle = on_stress_button;
        actions.open_settings = on_settings_button;
        lv_obj_t* diag = ui::diag_screen_create(usb_edge(), actions);
        update_view();
        lv_screen_load(diag);
        platform::display::refresh_now();
        g_boot_to_diag_ms = platform::sysinfo::uptime_ms();
        g_diag_shown = true;
        platform::console::printf("[%8lu] diag screen shown\n", static_cast<unsigned long>(g_boot_to_diag_ms));
    }

    // Step 7: storage mount and self-test (after the screen, a first-boot format can take seconds).
    platform::storage::Result r = platform::storage::mount();
    if (r.ok) r = platform::storage::self_test();
    nvp::diag::set_result(g_tests[index(TestName::Storage)], r.ok ? Outcome::Passed : Outcome::Failed, r.reason);

    // Steps 8 and 10 arrive in Stage 3 (stored Wi-Fi details, background worker).
    nvp::diag::set_result(g_tests[index(TestName::WiFi)], Outcome::Skipped, kNotInThisStage);

    // Step 9: the report (FR-026).
    print_report();
}

void loop() {
    serial_console_poll(kHandlers);
    if (!g_diag_shown) return;
    static uint32_t last = 0;
    const uint32_t now = platform::sysinfo::uptime_ms();
    if (now - last >= kViewRefreshMs) {
        last = now;
        update_view();
    }
    platform::display::handle();
}

}  // namespace app
