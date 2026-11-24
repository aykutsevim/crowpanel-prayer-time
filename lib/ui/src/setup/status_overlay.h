// Status overlay and the restart-setup confirmation (02-ui-spec.md §7.4, reference
// 17-status-overlay.png; spec 004 FR-007, FR-008, contracts/screen-api.md). Drawn on
// lv_layer_top() over whatever screen is loaded (the main screen). Touch is classified by the app
// (nvp::screen::TouchClassifier); overlay_hit() tells it which button a tap landed on.
#pragma once

#include <cstdint>

#include "lvgl.h"
#include "nvp/net/views.h"

namespace nvp::ui {

enum class OverlayHit : uint8_t { None, Cancel, StartOver };

void overlay_open(const net::OverlayView& v);
void overlay_update(const net::OverlayView& v);  // change-driven
void overlay_show_confirm(bool on);
void overlay_close();
bool overlay_is_open();
bool overlay_confirm_shown();
OverlayHit overlay_hit(int32_t x, int32_t y);    // buttons of the confirmation only

}  // namespace nvp::ui
