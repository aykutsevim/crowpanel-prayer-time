// The four 16 × 16 stroke icons of the status row (02-ui-spec.md §4.1 M1, M4, §6), drawn from the
// paths in docs/design/reference.html <defs> with LVGL arcs and lines: 1.6 px, round caps
// (spec 003 research R6). No bitmaps (02 §1).
#pragma once

#include "lvgl.h"

namespace nvp::ui {

enum class Icon : uint8_t { Pin, Wifi, WifiOff, Approx };

// A 16 × 16 object at (x, y) that draws `icon` in `color`.
lv_obj_t* icon_create(lv_obj_t* parent, Icon icon, int32_t x, int32_t y, lv_color_t color);
void icon_set(lv_obj_t* obj, Icon icon, lv_color_t color);

}  // namespace nvp::ui
