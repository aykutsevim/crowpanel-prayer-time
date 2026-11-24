#include "setup/widgets.h"

#include <cstdio>
#include <cstring>

#include "main/tokens.h"

namespace nvp::ui::setup {

lv_obj_t* text_label(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, int32_t x, float baseline) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, color, LV_PART_MAIN);
    lv_obj_set_pos(l, x, tok::top_for_baseline(font, baseline));
    lv_label_set_text_static(l, "");
    return l;
}

lv_obj_t* centred_label(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, int32_t cx, float baseline) {
    lv_obj_t* l = text_label(parent, font, color, cx, baseline);
    lv_obj_set_user_data(l, reinterpret_cast<void*>(static_cast<intptr_t>(cx)));
    return l;
}

lv_obj_t* paragraph(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, int32_t x, float baseline,
                    int32_t width, int32_t pitch) {
    lv_obj_t* l = text_label(parent, font, color, x, baseline);
    lv_obj_set_width(l, width);
    lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_line_space(l, pitch - font->line_height, LV_PART_MAIN);
    return l;
}

lv_obj_t* box(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, lv_color_t fill, int32_t radius) {
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_scrollable(o, false);
    lv_obj_set_clickable(o, false);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, fill, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(o, radius, LV_PART_MAIN);
    return o;
}

int32_t text_width(const char* text, const lv_font_t* font) {
    lv_point_t p;
    lv_text_get_size(&p, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return p.x;
}

void set_text(lv_obj_t* l, const char* text) {
    const char* cur = lv_label_get_text(l);
    if (cur != nullptr && std::strcmp(cur, text) == 0) return;
    lv_label_set_text(l, text);
}

void set_color(lv_obj_t* o, lv_color_t c) {
    if (lv_color_eq(lv_obj_get_style_text_color(o, LV_PART_MAIN), c)) return;
    lv_obj_set_style_text_color(o, c, LV_PART_MAIN);
}

void set_hidden(lv_obj_t* o, bool hidden) {
    if (lv_obj_is_hidden(o) == hidden) return;
    lv_obj_set_hidden(o, hidden);
}

void centre_on(lv_obj_t* l, int32_t cx) {
    const lv_font_t* f = lv_obj_get_style_text_font(l, LV_PART_MAIN);
    const int32_t x = cx - text_width(lv_label_get_text(l), f) / 2;
    if (lv_obj_get_x(l) != x) lv_obj_set_x(l, x);
}

void qr_box_create(QrBox& q, lv_obj_t* parent, int32_t x, int32_t y, int32_t size, int32_t padding, int32_t radius) {
    q.square = box(parent, x, y, size, size, tok::ink(), radius);
    q.code = lv_qrcode_create(q.square);
    lv_qrcode_set_size(q.code, size - 2 * padding);
    lv_qrcode_set_dark_color(q.code, tok::bg());
    lv_qrcode_set_light_color(q.code, tok::ink());
    lv_obj_set_pos(q.code, padding, padding);
    q.data[0] = '\0';
}

void qr_box_set(QrBox& q, const char* data) {
    if (std::strcmp(q.data, data) == 0) return;
    std::snprintf(q.data, sizeof q.data, "%s", data);
    lv_qrcode_update(q.code, q.data, static_cast<uint32_t>(std::strlen(q.data)));
}

}  // namespace nvp::ui::setup
