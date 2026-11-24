// Setup screen, access-point mode (02-ui-spec.md §7.2, reference 15-setup-ap.png; spec 004 FR-004,
// FR-005, contracts/screen-api.md). Depends only on LVGL and lib/core.
#pragma once

#include "lvgl.h"
#include "nvp/net/views.h"

namespace nvp::ui {

lv_obj_t* setup_screen_create();
// Change-driven: only what differs from the last call is touched.
void setup_screen_update(const net::SetupView& v);

}  // namespace nvp::ui
