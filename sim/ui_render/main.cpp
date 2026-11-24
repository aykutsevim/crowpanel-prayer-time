// Host UI harness for the engineering screens (FR-008, research R12).
//
// Renders the boot and diagnostic screens (spec 001) and the main screen's fourteen reference
// fixtures (spec 003) to out/snapshots/*.png with
// LVGL's test display (800x480) and screenshot writer, so the implementer can check them
// before the owner looks at the panel (CLAUDE.md, "You cannot see the panel").
// Fixtures: contracts/diag-screens.md, "Snapshot fixtures".
#include <cstdio>
#include <cstring>
#include <initializer_list>

#include "diag/boot_screen.h"
#include "diag/diag_screen.h"
#include "diag/settings_screen.h"
#include "diag/view_model.h"
#include "lvgl.h"
#include "main/fixture_view.h"
#include "main/main_screen.h"
#include "nvp/screen/fixtures.h"
#include "nvp/version.h"

using namespace nvp::ui::diag;
using nvp::diag::CornerResult;
using nvp::diag::CornerState;
using nvp::diag::index;
using nvp::diag::Outcome;
using nvp::diag::set_result;
using nvp::diag::TestName;

int run_redraw_checks();  // redraw_check.cpp (spec 003 FR-015, FR-016)
int run_edge_checks();    // edge_check.cpp (spec 003 T045, T046)

namespace {

constexpr const char* kOutDir = "out/snapshots/";

// Writes the active screen to out/snapshots/<name>.png (the old file is removed first, so the
// test module writes a fresh image instead of comparing against it).
bool snapshot(const char* name) {
    char path[128];
    std::snprintf(path, sizeof path, "%s%s.png", kOutDir, name);
    std::remove(path);
    const lv_test_screenshot_result_t r = lv_test_screenshot_compare(path);
    // LVGL reports success even when the PNG write fails (it only logs), so check the file.
    std::FILE* f = std::fopen(path, "rb");
    const bool written = f != nullptr;
    if (f) std::fclose(f);
    const bool ok = r == LV_TEST_SCREENSHOT_RESULT_PASSED && written;
    std::printf("sim: %-14s %s\n", name, ok ? path : "FAILED");
    return ok;
}

DiagViewModel base() {
    DiagViewModel vm;
    vm.firmware = nvp::kFirmwareVersion;
    vm.model = "DIS02170A";
    vm.revision = "V1.4";
    vm.silence.ran = true;
    vm.silence.acknowledged = true;
    vm.silence.ran_first = true;
    vm.silence.attempts = 4;
    for (TestName t : {TestName::Silence, TestName::Display, TestName::Touch, TestName::Backlight,
                       TestName::Storage}) {
        set_result(vm.tests[index(t)], Outcome::Passed, "");
    }
    set_result(vm.tests[index(TestName::WiFi)], Outcome::Skipped, "not in this stage");
    vm.heap_internal_free = 337480;
    vm.heap_psram_free = 7616000;
    vm.heartbeat = true;
    vm.screen = ActiveScreen::Diagnostic;
    return vm;
}

}  // namespace

int main() {
    lv_init();
    lv_display_t* display = lv_test_display_create(800, 480);
    if (display == nullptr) {
        std::fprintf(stderr, "sim: could not create the 800x480 test display\n");
        return 1;
    }
    int failures = 0;

    lv_obj_t* boot = boot_screen_create(nvp::kFirmwareVersion, "DIS02170A", "V1.4");
    lv_screen_load(boot);
    failures += !snapshot("boot");

    lv_obj_t* diag = diag_screen_create(Edge::Left, DiagActions{});
    lv_screen_load(diag);

    DiagViewModel idle = base();
    diag_screen_update(idle);
    failures += !snapshot("diag_idle");

    DiagViewModel failed = base();
    set_result(failed.tests[index(TestName::Touch)], Outcome::Failed, "GT911 not found at 0x5D");
    set_result(failed.tests[index(TestName::Storage)], Outcome::Failed, "mount failed");
    set_result(failed.tests[index(TestName::WiFi)], Outcome::Skipped, "no Wi-Fi set up");
    failed.heartbeat = false;
    diag_screen_update(failed);
    failures += !snapshot("diag_failures");

    DiagViewModel stress = base();
    stress.stress.running = true;
    stress.stress.elapsed_s = 312;
    stress.stress.cycles = 31;
    stress.stress.scan_ok = 31;
    stress.stress.write_ok = 31;
    stress.stress.https_ok = 9;
    stress.stress.https_err = 1;
    std::snprintf(stress.stress.last_error, sizeof stress.stress.last_error, "https: timeout");
    stress.wifi.kind = nvp::diag::WifiKind::Connected;
    std::snprintf(stress.wifi.ssid, sizeof stress.wifi.ssid, "EvAgi");
    stress.wifi.rssi_dbm = -54;
    diag_screen_update(stress);
    failures += !snapshot("diag_stress");

    DiagViewModel touch = base();
    touch.touch_active = true;
    touch.touch_x = 300;
    touch.touch_y = 120;
    touch.corners[0] = CornerResult{CornerState::Passed, 2, -1};
    touch.corners[1] = CornerResult{CornerState::Passed, -3, 4};
    touch.corners[2] = CornerResult{CornerState::Failed, 14, 3};
    diag_screen_update(touch);
    failures += !snapshot("diag_touch");

    lv_obj_t* settings = settings_screen_create(DiagActions{});
    lv_screen_load(settings);

    DiagViewModel none = base();
    none.screen = ActiveScreen::Settings;
    none.wifi.kind = nvp::diag::WifiKind::NotSetUp;
    settings_screen_update(none);
    failures += !snapshot("settings_none");

    DiagViewModel confirm = base();
    confirm.screen = ActiveScreen::Settings;
    confirm.wifi.kind = nvp::diag::WifiKind::Connected;
    std::snprintf(confirm.wifi.ssid, sizeof confirm.wifi.ssid, "EvAgi");
    confirm.confirm = Confirm::Forget;
    settings_screen_update(confirm);
    failures += !snapshot("settings_confirm");

    DiagViewModel hotspot = base();
    hotspot.screen = ActiveScreen::Settings;
    hotspot.wifi.kind = nvp::diag::WifiKind::Hotspot;
    std::snprintf(hotspot.wifi.ap_ssid, sizeof hotspot.wifi.ap_ssid, "NamazVakti-1A2B");
    std::snprintf(hotspot.wifi.ap_address, sizeof hotspot.wifi.ap_address, "192.168.4.1");
    hotspot.hotspot_open = true;
    std::snprintf(hotspot.hotspot_password, sizeof hotspot.hotspot_password, "12345678");  // fixture value
    std::snprintf(hotspot.setup_message, sizeof hotspot.setup_message, "Connecting to EvAgi...");
    settings_screen_update(hotspot);
    failures += !snapshot("settings_hotspot");

    // Main screen: the fourteen reference fixtures (spec 003 FR-020), named after their references.
    lv_obj_t* main_screen = nvp::ui::main_screen_create();
    lv_screen_load(main_screen);
    for (const nvp::screen::Fixture& f : nvp::screen::kFixtures) {
        nvp::engine::MainView view;
        nvp::ui::SystemView sys;
        nvp::ui::fixture_view(f.number, view, sys);
        nvp::ui::main_screen_update(view, sys);
        char name[64];
        std::snprintf(name, sizeof name, "%s", f.reference);
        char* dot = std::strrchr(name, '.');
        if (dot != nullptr) *dot = '\0';
        failures += !snapshot(name);
    }

    failures += run_redraw_checks();
    failures += run_edge_checks();

    lv_deinit();
    if (failures > 0) {
        std::fprintf(stderr, "sim: %d snapshot(s) failed\n", failures);
        return 1;
    }
    return 0;
}
