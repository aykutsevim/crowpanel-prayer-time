#include "bringup_app.h"

#include <cstdio>
#include <cstring>

#include "app/clock.h"
#include "app/net_worker.h"
#include "app/serial_console.h"
#include "diag/boot_screen.h"
#include "diag/diag_screen.h"
#include "diag/settings_screen.h"
#include "diag/view_model.h"
#include "main/fixture_view.h"
#include "main/main_screen.h"
#include "main/main_view.h"
#include "nvp/net/views.h"
#include "setup/connecting_screen.h"
#include "setup/setup_screen.h"
#include "setup/status_overlay.h"
#include "nvp/diag/report.h"
#include "nvp/diag/selftest.h"
#include "nvp/diag/touch_judge.h"
#include "nvp/engine/calendar.h"
#include "nvp/engine/engine.h"
#include "nvp/screen/fixtures.h"
#include "nvp/screen/gesture.h"
#include "nvp/screen/system_state.h"
#include "nvp/time/rtc_codec.h"
#include "nvp/time/zone.h"
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

// --- Spec 003: the main screen is the start screen; diagnostics are a hidden mode (FR-022). ---
// Spec 004: in the product mode the screen follows the network worker's mode: setup, connecting,
// or the main screen with its status overlay (03 §1, 02 §7, FR-001, FR-004..FR-008).
enum class Mode : uint8_t { Main, Diagnostic };
Mode g_mode = Mode::Main;
enum class Product : uint8_t { None, Main, Setup, Connecting };
Product g_product = Product::None;
lv_obj_t* g_main_screen = nullptr;
lv_obj_t* g_setup_screen = nullptr;
lv_obj_t* g_connecting_screen = nullptr;
// Touch (02 §8, research R12): classified inside the touch callback, acted on in loop().
nvp::screen::TouchClassifier g_classifier;
nvp::screen::Touch g_touch_event = nvp::screen::Touch::None;
int16_t g_tap_x = 0, g_tap_y = 0;
uint32_t g_last_touch = 0;
uint32_t g_confirm_shown_at = 0;
bool g_log_lvgl_memory = true;  // one line after start-up and after each fixture (spec 004 T068)

void log_lvgl_memory() {
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    platform::console::printf("[%8lu] lvgl memory: %lu of %lu B used (%u %%), largest free %lu B\n",
                              static_cast<unsigned long>(platform::sysinfo::uptime_ms()),
                              static_cast<unsigned long>(m.total_size - m.free_size),
                              static_cast<unsigned long>(m.total_size), static_cast<unsigned>(m.used_pct),
                              static_cast<unsigned long>(m.free_biggest_size));
}
constexpr uint32_t kOverlayTimeoutMs = 20000;    // 02 §7.4
constexpr uint32_t kStartOverTimeoutMs = 15000;  // 02 §7.4: no response counts as Vazgeç

// Developer overrides (SYS-06, contracts/serial-console.md).
bool g_forced = false;
nvp::screen::SystemState g_forced_state = nvp::screen::SystemState::Ok;
int16_t g_forced_days = 0;
int g_fixture = 0;  // 1..17 while a fixture is shown, else 0

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

nvp::engine::Moment moment_of(int64_t local) {
    const int64_t days = local >= 0 ? local / 86400 : -((-local + 86399) / 86400);
    const int32_t sod = static_cast<int32_t>(local - days * 86400);
    return nvp::engine::Moment{nvp::engine::to_date(static_cast<nvp::engine::DayNumber>(days)), sod};
}

// What the main screen shows now (spec 003 FR-019, OD3-2 as revised, constitution II):
//  - a fixture, when one was called up over the serial port (sample data, on purpose);
//  - otherwise the board clock with no prayer times loaded in this slice: `no_data`
//    (reference 14), or `no_time` when the clock is not valid.
// A forced state (development command) replaces the chosen state.
void build_main_view(nvp::engine::MainView& v, nvp::ui::SystemView& s) {
    const clock::Now n = clock::now();
    if (g_fixture != 0) {
        const nvp::screen::Fixture* f = nvp::screen::fixture(g_fixture);
        const int32_t sod = (f->second_of_day < 0 || !n.valid) ? -1 : moment_of(n.local).second_of_day;
        nvp::ui::fixture_view_at(g_fixture, sod, v, s);
    } else {
        s = nvp::ui::SystemView{};
        if (n.valid) {
            nvp::engine::Input in;
            in.now = moment_of(n.local);
            nvp::engine::evaluate(in, v);  // no records loaded: has_data is false
            s.state = nvp::screen::SystemState::NoData;
        } else {
            nvp::engine::evaluate_no_time(nullptr, nvp::engine::Settings{}, v);
            s.state = nvp::screen::SystemState::NoTime;
        }
    }
    if (g_forced) {
        s.state = g_forced_state;
        s.days_left = g_forced_days;
    }
}

nvp::screen::SystemState current_state() {
    static nvp::engine::MainView v;  // large; not on the stack
    nvp::ui::SystemView s;
    build_main_view(v, s);
    return s.state;
}

void update_main() {
    static nvp::engine::MainView v;
    nvp::ui::SystemView s;
    build_main_view(v, s);
    nvp::ui::main_screen_update(v, s);
}

nvp::net::OverlayView overlay_view(const net::Snapshot& n) {
    if (g_fixture == 17) return nvp::net::fixture_overlay_view();
    nvp::net::OverlayInput in;
    if (n.has_location) {
        in.province = n.location.province;
        in.district = n.location.district;
    }
    in.wifi_up = n.wifi_up;
    in.ssid = n.ssid;
    in.rssi = n.rssi;
    in.ip = n.ip;
    in.version = nvp::kFirmwareVersion;
    in.show_qr = false;  // no settings page until Slice 8 (OD4-2)
    return nvp::net::make_overlay_view(in);
}

Product wanted_product(const net::Snapshot& n) {
    if (g_fixture == 15) return Product::Setup;
    if (g_fixture == 16) return Product::Connecting;
    if (g_fixture != 0) return Product::Main;
    switch (n.mode) {
        case nvp::net::Mode::Setup: return Product::Setup;
        case nvp::net::Mode::Connecting:
        case nvp::net::Mode::Grace: return Product::Connecting;
        default: return Product::Main;
    }
}

// Loads the screen the mode asks for and updates it; only what changed is redrawn (UI-03).
void update_product() {
    const net::Snapshot n = net::snapshot();
    const Product want = wanted_product(n);
    if (want != g_product) {
        if (want != Product::Main) nvp::ui::overlay_close();
        lv_obj_t* scr = want == Product::Setup ? g_setup_screen : want == Product::Connecting ? g_connecting_screen
                                                                                                : g_main_screen;
        if (scr != nullptr) lv_screen_load(scr);
        g_product = want;
    }
    switch (g_product) {
        case Product::Setup:
            nvp::ui::setup_screen_update(g_fixture == 15 ? nvp::net::fixture_setup_view()
                                                         : nvp::net::make_setup_view(n.ap_ssid, n.hotspot_password,
                                                                                     n.phones, n.failure));
            break;
        case Product::Connecting:
            nvp::ui::connecting_screen_update(g_fixture == 16 ? nvp::net::fixture_connecting_view()
                                                              : nvp::net::make_connecting_view(
                                                                    n.attempt_ssid, n.mode == nvp::net::Mode::Grace));
            break;
        case Product::Main:
            update_main();
            if (g_fixture == 17 && !nvp::ui::overlay_is_open()) nvp::ui::overlay_open(overlay_view(n));
            if (nvp::ui::overlay_is_open()) nvp::ui::overlay_update(overlay_view(n));
            break;
        case Product::None:
            break;
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
    char clock_line[64];
    clock::describe(clock_line, sizeof clock_line);
    in.clock_line = clock_line;
    char screen_line[48];
    if (g_mode == Mode::Main && g_product == Product::Setup) {
        std::snprintf(screen_line, sizeof screen_line, "setup%s", g_fixture != 0 ? " (fixture)" : "");
    } else if (g_mode == Mode::Main && g_product == Product::Connecting) {
        std::snprintf(screen_line, sizeof screen_line, "connecting%s", g_fixture != 0 ? " (fixture)" : "");
    } else if (g_mode == Mode::Main) {
        std::snprintf(screen_line, sizeof screen_line, "main (state %s%s%s)", nvp::screen::state_name(current_state()),
                      g_fixture != 0 ? ", fixture" : "", nvp::ui::overlay_is_open() ? ", overlay" : "");
    } else {
        std::snprintf(screen_line, sizeof screen_line, "%s",
                      g_screen == ui::ActiveScreen::Settings ? "settings" : "diagnostic");
    }
    in.screen_line = screen_line;
    // Spec 004: mode, phones, last result and the stored location; names and classes only (FR-023).
    char mode_line[64];
    switch (n.mode) {
        case nvp::net::Mode::Setup:
            std::snprintf(mode_line, sizeof mode_line, "setup, phones %u, last %s", static_cast<unsigned>(n.phones),
                          nvp::net::failure_name(n.failure));
            break;
        case nvp::net::Mode::RunJoining: std::snprintf(mode_line, sizeof mode_line, "run, joining"); break;
        case nvp::net::Mode::RunUp: std::snprintf(mode_line, sizeof mode_line, "run, connected"); break;
        case nvp::net::Mode::RunBackoff: std::snprintf(mode_line, sizeof mode_line, "run, waiting to retry"); break;
        case nvp::net::Mode::RunNoRadio: std::snprintf(mode_line, sizeof mode_line, "run, no radio (silence)"); break;
        default: std::snprintf(mode_line, sizeof mode_line, "%s", nvp::net::mode_name(n.mode)); break;
    }
    in.mode_line = mode_line;
    char location_line[100];
    if (n.has_location) {
        std::snprintf(location_line, sizeof location_line, "%s / %s (%lu)", n.location.province, n.location.district,
                      static_cast<unsigned long>(n.location.district_id));
    } else {
        std::snprintf(location_line, sizeof location_line, "none");
    }
    in.location_line = location_line;
    nvp::diag::report::format(in, g_report, sizeof g_report);
    platform::console::write(g_report);
}

void enter_main();

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
    enter_main();  // the product setup screen shows the device's network (spec 004 assumption)
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

void update_view();

// --- Modes (spec 003 FR-022, FR-023) ---

void enter_diag() {
    if (g_mode == Mode::Diagnostic || g_diag_screen == nullptr) return;
    nvp::ui::overlay_close();
    g_product = Product::None;
    g_mode = Mode::Diagnostic;
    g_screen = ui::ActiveScreen::Diagnostic;
    g_confirm = ui::Confirm::None;
    update_view();
    lv_screen_load(g_diag_screen);
    platform::console::write("diagnostic mode\n");
}

void enter_main() {
    if (g_mode == Mode::Main || g_main_screen == nullptr) return;
    if (net::snapshot().stress_running) stress(false);  // leaving the mode stops stress (FR-023)
    g_mode = Mode::Main;
    g_product = Product::None;  // update_product() loads the screen of the worker's mode
    update_product();
    platform::console::write("main screen\n");
}

void on_main_screen(void*) { enter_main(); }

// --- Development commands (SYS-06; contracts/serial-console.md of spec 003) ---

void format_date_time(int64_t seconds, char* out, size_t cap) {
    const nvp::engine::Moment m = moment_of(seconds);
    std::snprintf(out, cap, "%04d-%02u-%02u %02ld:%02ld:%02ld", static_cast<int>(m.date.year), m.date.month,
                  m.date.day, static_cast<long>(m.second_of_day / 3600), static_cast<long>((m.second_of_day / 60) % 60),
                  static_cast<long>(m.second_of_day % 60));
}

int64_t local_of(const nvp::diag::ParsedCommand& c) {
    const nvp::engine::Date d{static_cast<int16_t>(c.year), static_cast<uint8_t>(c.month), static_cast<uint8_t>(c.day)};
    return static_cast<int64_t>(nvp::engine::to_day_number(d)) * 86400 + c.hour * 3600 + c.minute * 60 + c.second;
}

void screen_command(const nvp::diag::ParsedCommand& c) {
    using nvp::diag::Command;
    char a[40];
    char b[40];
    switch (c.command) {
        case Command::Diag:
            if (g_mode == Mode::Main) {
                enter_diag();
            } else {
                platform::console::write("already in diagnostic mode\n");
            }
            return;
        case Command::Main:
            if (g_mode == Mode::Main) {
                platform::console::write("already on the main screen\n");
            } else {
                enter_main();
            }
            return;
        case Command::TimeGet: {
            const nvp::time::RtcReading r = clock::read_rtc();
            if (r.valid) {
                format_date_time(r.utc, a, sizeof a);
                format_date_time(nvp::time::utc_to_local(r.utc), b, sizeof b);
                platform::console::printf("rtc: %s UTC (%s local), valid\n", a, b + 11);
            } else {
                platform::console::printf("rtc: not valid (%s)\n", nvp::time::reason_name(r.reason));
            }
            return;
        }
        case Command::TimeSet: {
            const char* why = "";
            const int64_t local = local_of(c);
            if (clock::set_rtc_local(local, &why)) {
                g_fixture = 0;
                format_date_time(nvp::time::local_to_utc(local), a, sizeof a);
                platform::console::printf("rtc set: %s UTC\n", a);
            } else {
                platform::console::printf("rtc set failed (%s)\n", why);
            }
            break;
        }
        case Command::ClockRtc:
            clock::use_rtc();
            g_fixture = 0;
            platform::console::write("clock: rtc\n");
            break;
        case Command::ClockSim:
            clock::use_sim(local_of(c), false);
            format_date_time(local_of(c), a, sizeof a);
            platform::console::printf("clock: sim %s x1\n", a);
            break;
        case Command::ClockSpeed:
            if (clock::set_speed(static_cast<uint16_t>(c.number))) {
                platform::console::printf("clock: speed x%ld\n", static_cast<long>(c.number));
            } else {
                platform::console::write("refused: the board clock has no speed (use clock sim first)\n");
            }
            return;
        case Command::ClockPause:
            clock::pause();
            platform::console::write("clock: paused\n");
            break;
        case Command::ClockRun:
            clock::run();
            platform::console::write("clock: running\n");
            break;
        case Command::StateForce:
            g_forced = true;
            g_forced_state = c.state;
            g_forced_days = static_cast<int16_t>(c.number);
            if (c.state == nvp::screen::SystemState::CacheEnding) {
                platform::console::printf("state: forced %s %ld\n", nvp::screen::state_name(c.state), static_cast<long>(c.number));
            } else {
                platform::console::printf("state: forced %s\n", nvp::screen::state_name(c.state));
            }
            break;
        case Command::StateAuto:
            g_forced = false;
            platform::console::write("state: auto\n");
            break;
        case Command::Fixture: {
            const nvp::screen::Fixture* f = nvp::screen::fixture(c.number);
            g_fixture = c.number;
            g_forced = false;
            const int32_t sod = f->second_of_day < 0 ? 0 : f->second_of_day;
            const int64_t local = static_cast<int64_t>(nvp::engine::to_day_number(nvp::engine::Date{2026, 11, 17})) * 86400 + sod;
            clock::use_sim(local, true);
            platform::console::printf("fixture %ld: %s\n", static_cast<long>(c.number), f->reference);
            g_log_lvgl_memory = true;  // after the next update has built the screen
            break;
        }
        case Command::FixtureOff:
            g_fixture = 0;
            g_forced = false;
            clock::use_rtc();
            platform::console::write("fixture: off\n");
            break;
        default:
            return;
    }
    if (g_mode == Mode::Main) {
        if (c.command == Command::FixtureOff) nvp::ui::overlay_close();
        update_product();
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
    h.screen_command = screen_command;
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
    if (g_mode == Mode::Main) {
        // Tap, 5 s hold and the hidden two-finger hold (02 §8, spec 003 FR-022, spec 004 R12).
        // No product object is clickable in LVGL; loop() acts on the classified gesture.
        const uint32_t now = platform::sysinfo::uptime_ms();
        const uint8_t points = pressed ? platform::touch::last_points() : 0;
        if (pressed) {
            g_tap_x = *x;
            g_tap_y = *y;
            g_last_touch = now;
        }
        const nvp::screen::Touch t = g_classifier.feed(now, points, *x, *y);
        if (t != nvp::screen::Touch::None) g_touch_event = t;
    } else if (pressed && !g_touch.pressed && g_screen == ui::ActiveScreen::Diagnostic) {
        g_judge.judge(*x, *y);
    }
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

    // The board's real-time clock (spec 003 OD3-1, FR-026): read once, after the silence step
    // started the I2C bus.
    clock::start();

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
        actions.main_screen = on_main_screen;
        g_diag_screen = ui::diag_screen_create(usb_edge(), actions);
        g_settings_screen = ui::settings_screen_create(actions);
        g_screen = ui::ActiveScreen::Diagnostic;
        update_view();
        // Spec 003: the device starts on the main screen; diagnostics are a hidden mode (FR-019).
        g_main_screen = nvp::ui::main_screen_create();
        g_setup_screen = nvp::ui::setup_screen_create();          // spec 004 FR-004
        g_connecting_screen = nvp::ui::connecting_screen_create();  // spec 004 FR-006
        g_mode = Mode::Main;
        update_main();
        lv_screen_load(g_main_screen);
        g_product = Product::Main;
        platform::display::refresh_now();
        g_boot_to_diag_ms = platform::sysinfo::uptime_ms();
        platform::console::printf("[%8lu] main screen shown\n", static_cast<unsigned long>(g_boot_to_diag_ms));
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

// Acts on one classified gesture of the product mode (02 §8; spec 004 FR-007, FR-008, FR-020).
void handle_touch(uint32_t now) {
    using nvp::screen::Touch;
    const Touch t = g_touch_event;
    g_touch_event = Touch::None;
    if (t == Touch::TwoFingerHold5s && g_product == Product::Main) {
        enter_diag();
        return;
    }
    if (g_product != Product::Main) return;  // setup and connecting screens: nothing (02 §8)
    const bool open = nvp::ui::overlay_is_open();
    const bool confirm = nvp::ui::overlay_confirm_shown();
    if (open && confirm && now - g_confirm_shown_at >= kStartOverTimeoutMs) {
        nvp::ui::overlay_close();  // no response for 15 s counts as Vazgeç
        return;
    }
    if (open && !confirm && now - g_last_touch >= kOverlayTimeoutMs) {
        nvp::ui::overlay_close();
        return;
    }
    if (t == Touch::Tap) {
        if (!open) {
            nvp::ui::overlay_open(overlay_view(net::snapshot()));
        } else if (!confirm) {
            nvp::ui::overlay_close();
        } else {
            switch (nvp::ui::overlay_hit(g_tap_x, g_tap_y)) {
                case nvp::ui::OverlayHit::Cancel:
                    nvp::ui::overlay_close();
                    break;
                case nvp::ui::OverlayHit::StartOver:
                    nvp::ui::overlay_close();
                    net::request_start_over();
                    platform::console::write("start over: Wi-Fi details will be erased (MODE-06)\n");
                    break;
                case nvp::ui::OverlayHit::None:
                    break;
            }
        }
    } else if (t == Touch::Hold5s && open && !confirm) {
        nvp::ui::overlay_show_confirm(true);
        g_confirm_shown_at = now;
    }
}

void loop() {
    serial_console_poll(kHandlers);
    if (!g_display_ok) return;
    static uint32_t last = 0;
    const uint32_t now = platform::sysinfo::uptime_ms();
    if (g_mode == Mode::Main) handle_touch(now);
    if (now - last >= kViewRefreshMs) {
        last = now;
        if (g_mode == Mode::Main) {
            update_product();  // touches only what changed (UI-03)
            if (g_log_lvgl_memory) {
                g_log_lvgl_memory = false;
                log_lvgl_memory();
            }
        } else {
            update_view();
        }
    }
    platform::display::handle();
}

}  // namespace app
