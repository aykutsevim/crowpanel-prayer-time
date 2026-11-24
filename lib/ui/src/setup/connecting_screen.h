// Connecting screen (02-ui-spec.md §7.3, reference 16-connecting.png; spec 004 FR-006).
#pragma once

#include "lvgl.h"
#include "nvp/net/views.h"

namespace nvp::ui {

lv_obj_t* connecting_screen_create();
void connecting_screen_update(const net::ConnectingView& v);  // change-driven

}  // namespace nvp::ui
