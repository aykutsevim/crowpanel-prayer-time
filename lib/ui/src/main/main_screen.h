// The main screen (02-ui-spec.md §4 to §6; spec 003, contracts/screen-api.md).
// Depends only on LVGL and lib/core (05-architecture.md §4).
#pragma once

#include "lvgl.h"
#include "main/main_view.h"
#include "nvp/engine/engine.h"

namespace nvp::ui {

// Builds the screen once and returns it (load it with lv_screen_load).
lv_obj_t* main_screen_create();

// Draws the given moment and state. Only objects whose content changed since the last call are
// updated, so a normal second touches only the clock, countdown and progress (UI-03, research R8).
void main_screen_update(const engine::MainView& v, const SystemView& s);

// For the host harness's checks (spec 003 T045, T046): the city label and the marker object.
lv_obj_t* main_screen_city_label();
lv_obj_t* main_screen_marker();

}  // namespace nvp::ui
