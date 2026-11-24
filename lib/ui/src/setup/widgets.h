// Small LVGL helpers shared by the setup, connecting and overlay screens (spec 004). Same rules as
// the main screen: text is placed by baseline (02 gives baselines), content changes only when it
// differs, no allocation after creation (contracts/screen-api.md).
#pragma once

#include <cstdint>

#include "lvgl.h"

namespace nvp::ui::setup {

lv_obj_t* text_label(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, int32_t x, float baseline);
// A label centred on `cx` (its width follows the text).
lv_obj_t* centred_label(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, int32_t cx, float baseline);
// A wrapped paragraph: `width` px wide, lines `pitch` px apart, first baseline at `baseline`.
lv_obj_t* paragraph(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, int32_t x, float baseline,
                    int32_t width, int32_t pitch);
lv_obj_t* box(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, lv_color_t fill, int32_t radius);

int32_t text_width(const char* text, const lv_font_t* font);
void set_text(lv_obj_t* label, const char* text);    // only when it differs
void set_color(lv_obj_t* label, lv_color_t color);   // only when it differs
void set_hidden(lv_obj_t* obj, bool hidden);         // only when it differs
void centre_on(lv_obj_t* label, int32_t cx);         // after a text change

// A QR code on an `ink` rounded square (02 §7.2, §7.4; research R11): modules `bg` on `ink`.
struct QrBox {
    lv_obj_t* square = nullptr;
    lv_obj_t* code = nullptr;
    char data[96] = {};
};
void qr_box_create(QrBox& q, lv_obj_t* parent, int32_t x, int32_t y, int32_t size, int32_t padding, int32_t radius);
void qr_box_set(QrBox& q, const char* data);  // re-encodes only when the data changed

}  // namespace nvp::ui::setup
