// Fixed-cell numbers (02-ui-spec.md §2.2; spec 003 FR-004, research R4): every character sits
// centred in its own cell, digit cell = round(0.475 × size), colon and dot cell = round(0.252 × size),
// space = the font's space (spec 004), so
// ticking digits never move. One LVGL object per value; it redraws only when its text or colour
// changes.
#pragma once

#include <cstdint>

#include "lvgl.h"

namespace nvp::ui {

// `size_px` is the font's nominal size (the cells derive from it). Position with
// mono_text_place(); the object's size is set from the text.
lv_obj_t* mono_text_create(lv_obj_t* parent, const lv_font_t* font, int32_t size_px);
void mono_text_set(lv_obj_t* obj, const char* text);  // ASCII digits, ':', '-', '.', ' '
void mono_text_set_color(lv_obj_t* obj, lv_color_t color);
int32_t mono_text_width(const lv_obj_t* obj);
// Places the line so its baseline is at `baseline`, left edge at `x`.
void mono_text_place(lv_obj_t* obj, int32_t x, float baseline);

}  // namespace nvp::ui
