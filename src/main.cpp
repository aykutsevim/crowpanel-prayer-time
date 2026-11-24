// Namaz Vakti Panel firmware entry point (ESP-IDF).
//
// Startup order: specs/001-silent-board-bringup/plan.md, "Startup order" (MODE-01, FR-001,
// FR-005). The silence step is the first statement of app_main(), before any other peripheral is
// started (SIL-02, MODE-01). Everything else lives in src/app and src/platform.
#include "app/bringup_app.h"
#include "platform/audio_off.h"
#include "platform/console_io.h"
#include "platform/tasks.h"

namespace {

constexpr int kUiCore = 1;                 // LVGL and the display on core 1, network on core 0 (R8)
constexpr uint32_t kUiStackBytes = 16384;
constexpr uint32_t kUiPeriodMs = 5;        // lets the idle task run; LVGL is serviced every 5 ms

void ui_task(void*) {
    // 3+. Backlight, storage, display, touch, report and the network worker, via the app.
    app::start();
    for (;;) {
        app::loop();
        platform::tasks::sleep_ms(kUiPeriodMs);
    }
}

}  // namespace

extern "C" void app_main(void) {
    // 1. Silence first (SIL-02, MODE-01, FR-001).
    platform::silence::audio_off();

    // 2. Serial console, then the silence result on its own line.
    platform::console::begin();
    app::print_silence_line();
    platform::silence::on_console_ready();

    // The UI task owns LVGL and creates the RGB panel, so its interrupts stay on core 1.
    if (!platform::tasks::start_worker("ui", kUiCore, kUiStackBytes, ui_task, nullptr)) {
        platform::console::write("ui: task could not start\n");
    }
}
