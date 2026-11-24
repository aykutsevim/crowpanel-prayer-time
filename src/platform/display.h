// RGB panel and LVGL display (research R7, R8 starting configuration L0; FR-005).
//
// esp_lcd RGB panel with the vendor timing, one frame buffer in PSRAM, no bounce buffer, and LVGL
// 9.6 in PARTIAL mode with an 800x40 draw buffer in internal RAM. LVGL runs on the core that
// calls begin() and handle(): the UI task on core 1 (src/main.cpp).
#pragma once

#include <cstdint>

namespace platform::display {

struct Result {
    bool ok = false;
    char reason[48] = {};
};

// Starts LVGL, the panel and the LVGL display. Safe to call once.
Result begin();

bool ready();

// Mitigation ladder L1/L2 (research R8): changes the pixel clock at run time.
bool set_pclk(uint32_t hz);

// Touch input for LVGL. The callback returns true while pressed and fills x, y in panel
// coordinates; it is called from handle() about every 33 ms.
using TouchReadFn = bool (*)(int16_t* x, int16_t* y);
void attach_touch(TouchReadFn read);

// Runs LVGL timers and rendering. Call from loop().
void handle();

// Renders the active screen now (used right after loading the boot and diagnostic screens).
void refresh_now();

}  // namespace platform::display
