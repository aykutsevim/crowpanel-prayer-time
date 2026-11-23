// Plain data shown by the engineering screens (data-model.md, contracts/diag-screens.md).
// Filled by src/app on the device and by fixtures in the host harness. No logic, no LVGL.
#pragma once

#include <cstdint>

#include "nvp/diag/report.h"
#include "nvp/diag/selftest.h"
#include "nvp/diag/touch_judge.h"

namespace nvp::ui::diag {

// Panel edge, as seen with the picture upright.
enum class Edge : uint8_t { Left, Right, Top, Bottom };

enum class ActiveScreen : uint8_t { Boot, Diagnostic, Settings };

// A confirmation open on the settings screen (FR-019a, Stage 3).
enum class Confirm : uint8_t { None, StartSetup, Forget };

struct StressView {
    bool running = false;
    bool paused = false;  // hotspot open (spec edge case)
    uint32_t elapsed_s = 0;
    uint32_t cycles = 0;
    uint32_t scan_ok = 0, scan_err = 0;
    uint32_t write_ok = 0, write_err = 0;
    uint32_t https_ok = 0, https_err = 0, https_skip = 0;
    char last_error[48] = {};
};

struct DiagViewModel {
    const char* firmware = "";
    const char* model = "";
    const char* revision = "";
    nvp::diag::SilenceLine silence;
    nvp::diag::SelfTestResult tests[nvp::diag::kTestCount];
    nvp::diag::WifiLine wifi;
    uint32_t heap_internal_free = 0;
    uint32_t heap_psram_free = 0;
    StressView stress;
    bool touch_active = false;
    int16_t touch_x = 0;
    int16_t touch_y = 0;
    nvp::diag::CornerResult corners[nvp::diag::kCornerCount];
    bool heartbeat = false;  // toggles once per second
    ActiveScreen screen = ActiveScreen::Boot;
    Confirm confirm = Confirm::None;
    char status[48] = {};  // short message, for example "Stress: not in this stage"
};

// Actions the screens report back to the app. Called from the LVGL task.
struct DiagActions {
    void (*stress_toggle)(void* ctx) = nullptr;
    void (*open_settings)(void* ctx) = nullptr;
    void* ctx = nullptr;
};

}  // namespace nvp::ui::diag
