#include "main/mono_text.h"

#include <cmath>
#include <cstring>

#include "main/tokens.h"

namespace nvp::ui {
namespace {

struct Mono {
    const lv_font_t* font;
    int32_t size;
    int32_t digit_cell;
    int32_t colon_cell;
    int32_t dash_cell;
    int32_t space_cell;  // the font's own space: grouped digits ("4827 1935", spec 004 references 15, 17)
    lv_color_t color;
    char text[16];
};

// The placeholder dash is Barlow Condensed's tabular hyphen in the reference renders (reached there
// through the OpenType tnum feature, which lv_font_conv cannot apply). It is drawn as a bar with the
// proportions measured from references 13 and 14 at 48, 96 and 114 px (research R15): advance
// 0.328 em, bar 0.274 em wide and 0.104 em thick, centred 0.307 em above the baseline.
constexpr float kDashAdvance = 0.328f, kDashWidth = 0.274f, kDashThick = 0.104f, kDashRaise = 0.307f;

int32_t cell(const Mono& m, char c) {
    if (c == ':' || c == '.') return m.colon_cell;
    if (c == '-') return m.dash_cell;
    if (c == ' ') return m.space_cell;
    return m.digit_cell;
}

int32_t width_of(const Mono& m) {
    int32_t w = 0;
    for (const char* p = m.text; *p != '\0'; ++p) w += cell(m, *p);
    return w;
}

void draw(lv_event_t* e) {
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    const Mono* m = static_cast<const Mono*>(lv_obj_get_user_data(obj));
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.font = m->font;
    dsc.color = m->color;
    int32_t x = a.x1;
    for (const char* p = m->text; *p != '\0'; ++p) {
        const int32_t c = cell(*m, *p);
        if (*p != ' ') {
            // Centre the glyph's ink (its bounding box), not its advance: this matches the
            // tabular figures of the reference renders, where a narrow "1" sits in the middle of
            // its cell (review of fixture 01, research R15).
            lv_font_glyph_dsc_t g;
            int32_t shift = 0;
            if (lv_font_get_glyph_dsc(m->font, &g, static_cast<uint32_t>(*p), 0)) {
                shift = (c - g.box_w) / 2 - g.ofs_x;
            } else {
                shift = (c - lv_font_get_glyph_width(m->font, static_cast<uint32_t>(*p), 0)) / 2;
            }
            lv_point_t pos{x + shift, a.y1};
            lv_draw_character(layer, &dsc, &pos, static_cast<uint32_t>(*p));
        }
        x += c;
    }
}

void on_delete(lv_event_t* e) {
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    delete static_cast<Mono*>(lv_obj_get_user_data(obj));
}

}  // namespace

lv_obj_t* mono_text_create(lv_obj_t* parent, const lv_font_t* font, int32_t size_px) {
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_scrollable(obj, false);
    lv_obj_set_clickable(obj, false);
    Mono* m = new Mono{font,
                       size_px,
                       static_cast<int32_t>(std::lround(0.475 * size_px)),
                       static_cast<int32_t>(std::lround(0.252 * size_px)),
                       static_cast<int32_t>(std::lround(kDashAdvance * size_px)),
                       static_cast<int32_t>(lv_font_get_glyph_width(font, ' ', 0)),
                       tok::ink(),
                       {}};
    lv_obj_set_user_data(obj, m);
    lv_obj_set_size(obj, 0, font->line_height);
    lv_obj_add_event_cb(obj, draw, LV_EVENT_DRAW_MAIN, nullptr);
    lv_obj_add_event_cb(obj, on_delete, LV_EVENT_DELETE, nullptr);
    return obj;
}

void mono_text_set(lv_obj_t* obj, const char* text) {
    Mono* m = static_cast<Mono*>(lv_obj_get_user_data(obj));
    if (std::strncmp(m->text, text, sizeof m->text) == 0) return;  // unchanged: no redraw
    char next[sizeof m->text];
    std::strncpy(next, text, sizeof next - 1);
    next[sizeof next - 1] = '\0';
    // Same layout (same cell widths): invalidate only the cells that changed, so a ticking second
    // redraws one or two digits, not the whole number (UI-03, research R8).
    bool same_layout = std::strlen(next) == std::strlen(m->text);
    for (size_t i = 0; same_layout && next[i] != '\0'; ++i) same_layout = cell(*m, next[i]) == cell(*m, m->text[i]);
    if (same_layout) {
        lv_area_t a;
        lv_obj_get_coords(obj, &a);
        int32_t x = a.x1;
        for (size_t i = 0; next[i] != '\0'; ++i) {
            const int32_t c = cell(*m, next[i]);
            if (next[i] != m->text[i]) {
                lv_area_t r{x, a.y1, x + c - 1, a.y2};
                lv_obj_invalidate_area(obj, &r);
            }
            x += c;
        }
        std::memcpy(m->text, next, sizeof next);
        return;
    }
    lv_obj_invalidate(obj);
    std::memcpy(m->text, next, sizeof next);
    lv_obj_set_width(obj, width_of(*m));
    lv_obj_invalidate(obj);
}

void mono_text_set_color(lv_obj_t* obj, lv_color_t color) {
    Mono* m = static_cast<Mono*>(lv_obj_get_user_data(obj));
    if (lv_color_eq(m->color, color)) return;
    m->color = color;
    lv_obj_invalidate(obj);
}

int32_t mono_text_width(const lv_obj_t* obj) {
    return width_of(*static_cast<const Mono*>(lv_obj_get_user_data(const_cast<lv_obj_t*>(obj))));
}

void mono_text_place(lv_obj_t* obj, int32_t x, float baseline) {
    const Mono* m = static_cast<const Mono*>(lv_obj_get_user_data(obj));
    const int32_t y = tok::top_for_baseline(m->font, baseline);
    // Setting an unchanged position still refreshes the style in LVGL; skip it (UI-03).
    if (lv_obj_get_style_x(obj, LV_PART_MAIN) == x && lv_obj_get_style_y(obj, LV_PART_MAIN) == y) return;
    lv_obj_set_pos(obj, x, y);
}

}  // namespace nvp::ui
