// Diagnostic screen (contracts/diag-screens.md, FR-006, FR-014, FR-014a).
//
// The test pattern (border, colour bars, ticks, one-pixel line field) is drawn by one draw
// callback, not by hundreds of objects. Everything else is a small set of labels and objects
// whose text changes only when the view model changes.
#pragma once

#include "diag/view_model.h"
#include "lvgl.h"

namespace nvp::ui::diag {

// Creates the diagnostic screen (not loaded). One instance per program.
lv_obj_t* diag_screen_create(Edge usb_side, const DiagActions& actions);

// Applies the view model; labels are changed only where the text differs.
void diag_screen_update(const DiagViewModel& vm);

}  // namespace nvp::ui::diag
