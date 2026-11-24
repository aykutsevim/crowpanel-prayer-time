// Settings screen (contracts/diag-screens.md; FR-019, FR-019a, FR-020). Engineering text in
// English. Nothing on it refers to sound, and there is no brightness control (OD1).
#pragma once

#include "diag/view_model.h"
#include "lvgl.h"

namespace nvp::ui::diag {

// Creates the settings screen (not loaded). One instance per program.
lv_obj_t* settings_screen_create(const DiagActions& actions);

// Applies the view model: Wi-Fi state, buttons, hotspot details and the open confirmation.
void settings_screen_update(const DiagViewModel& vm);

}  // namespace nvp::ui::diag
