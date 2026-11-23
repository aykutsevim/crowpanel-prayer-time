// Host UI harness for the engineering screens (FR-008, research R12).
//
// Renders view-model fixtures of the boot, diagnostic and settings screens to
// out/snapshots/*.png with LVGL's test display and screenshot writer.
//
// Stage 1 of spec 001 has no screens yet: this program only proves that LVGL 9.6.0 and the
// 800x480 test display build and run on the development machine. The fixtures arrive in
// Stage 2 (tasks.md, Phase 4).
#include "lvgl.h"

#include <cstdio>

int main() {
    lv_init();
    lv_display_t* display = lv_test_display_create(800, 480);
    if (display == nullptr) {
        std::fprintf(stderr, "sim: could not create the 800x480 test display\n");
        return 1;
    }
    lv_refr_now(display);
    std::printf("sim: LVGL %d.%d.%d test display 800x480 ready; no screens in Stage 1\n",
                LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
    lv_deinit();
    return 0;
}
