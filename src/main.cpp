// Namaz Vakti Panel firmware entry point.
//
// Startup order: specs/001-silent-board-bringup/plan.md, "Startup order" (MODE-01, FR-001,
// FR-005). The silence step is the first statement of setup(), before any other peripheral is
// started (SIL-02, MODE-01). Everything else lives in src/app and src/platform.
#include "app/bringup_app.h"
#include "platform/audio_off.h"
#include "platform/console_io.h"

void setup() {
    // 1. Silence first (SIL-02, MODE-01, FR-001).
    platform::silence::audio_off();

    // 2. Serial console, then the silence result on its own line.
    platform::console::begin();
    app::print_silence_line();
    platform::silence::on_console_ready();

    // 3+. Backlight, storage, report (Stage 1), via the app.
    app::start();
}

void loop() { app::loop(); }
