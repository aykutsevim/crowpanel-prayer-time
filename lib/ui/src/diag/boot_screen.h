// Boot screen (contracts/diag-screens.md). Shown right after the display starts, until the
// diagnostic screen is ready (at most 5 s after power-on, SC-006).
#pragma once

#include "lvgl.h"

namespace nvp::ui::diag {

// Creates the boot screen (not loaded). The strings must outlive the call only.
lv_obj_t* boot_screen_create(const char* firmware, const char* model, const char* revision);

}  // namespace nvp::ui::diag
