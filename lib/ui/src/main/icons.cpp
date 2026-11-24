#include "main/icons.h"

#include <cmath>
#include <initializer_list>

namespace nvp::ui {
namespace {

constexpr int32_t kStroke = 2;  // 1.6 px in the reference, nearest whole width

struct IconState {
    Icon icon;
    lv_color_t color;
};

void arc(lv_layer_t* layer, lv_color_t c, float cx, float cy, float r, float a0, float a1, int32_t x0, int32_t y0) {
    lv_draw_arc_dsc_t d;
    lv_draw_arc_dsc_init(&d);
    d.color = c;
    d.width = kStroke;
    d.rounded = 1;
    d.center.x = x0 + static_cast<int32_t>(std::lround(cx));
    d.center.y = y0 + static_cast<int32_t>(std::lround(cy));
    d.radius = static_cast<uint16_t>(std::lround(r));
    d.start_angle = a0;
    d.end_angle = a1;
    lv_draw_arc(layer, &d);
}

void line(lv_layer_t* layer, lv_color_t c, float ax, float ay, float bx, float by, int32_t x0, int32_t y0) {
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.color = c;
    d.width = kStroke;
    d.round_start = 1;
    d.round_end = 1;
    d.p1.x = x0 + ax;
    d.p1.y = y0 + ay;
    d.p2.x = x0 + bx;
    d.p2.y = y0 + by;
    lv_draw_line(layer, &d);
}

// A cubic Bézier as a short polyline (enough for a 16-px icon).
void cubic(lv_layer_t* layer, lv_color_t c, const float p[8], int32_t x0, int32_t y0) {
    float px = p[0], py = p[1];
    for (int i = 1; i <= 8; ++i) {
        const float t = i / 8.0f, u = 1 - t;
        const float x = u * u * u * p[0] + 3 * u * u * t * p[2] + 3 * u * t * t * p[4] + t * t * t * p[6];
        const float y = u * u * u * p[1] + 3 * u * u * t * p[3] + 3 * u * t * t * p[5] + t * t * t * p[7];
        line(layer, c, px, py, x, y, x0, y0);
        px = x;
        py = y;
    }
}

void dot(lv_layer_t* layer, lv_color_t c, float cx, float cy, float r, int32_t x0, int32_t y0) {
    lv_draw_fill_dsc_t d;
    lv_draw_fill_dsc_init(&d);
    d.color = c;
    d.radius = LV_RADIUS_CIRCLE;
    const int32_t rr = static_cast<int32_t>(std::ceil(r + 0.5f));
    lv_area_t a{x0 + static_cast<int32_t>(std::lround(cx)) - rr, y0 + static_cast<int32_t>(std::lround(cy)) - rr,
                x0 + static_cast<int32_t>(std::lround(cx)) + rr - 1, y0 + static_cast<int32_t>(std::lround(cy)) + rr - 1};
    lv_draw_fill(layer, &d, &a);
}

void draw_wifi(lv_layer_t* l, lv_color_t c, int32_t x0, int32_t y0, bool off) {
    // reference.html i-wifi: three arcs through (1.8,6.2)-(14.2,6.2) r 9.2, (4.1,8.8)-(11.9,8.8)
    // r 5.9, (6.3,11.3)-(9.7,11.3) r 2.7, and a dot at (8,13.4). Centres and angles derived from
    // the chord and radius (angles in LVGL's clockwise degrees from 3 o'clock).
    arc(l, c, 8.0f, 13.0f, 9.2f, 227.6f, 312.4f, x0, y0);
    arc(l, c, 8.0f, 13.23f, 5.9f, 228.6f, 311.4f, x0, y0);
    arc(l, c, 8.0f, 13.4f, 2.7f, 231.0f, 309.0f, x0, y0);
    dot(l, c, 8.0f, 13.4f, 0.6f + 0.8f, x0, y0);
    if (off) line(l, c, 2.4f, 1.6f, 13.6f, 14.4f, x0, y0);
}

void draw_pin(lv_layer_t* l, lv_color_t c, int32_t x0, int32_t y0) {
    // i-pin: upper half circle r 4.4 about (8,6), two curves down to the tip (8,14.4), ring r 1.3.
    arc(l, c, 8.0f, 6.0f, 4.4f, 180.0f, 360.0f, x0, y0);
    const float left[8] = {3.6f, 6.0f, 3.6f, 9.1f, 8.0f, 14.4f, 8.0f, 14.4f};
    const float right[8] = {12.4f, 6.0f, 12.4f, 9.1f, 8.0f, 14.4f, 8.0f, 14.4f};
    cubic(l, c, left, x0, y0);
    cubic(l, c, right, x0, y0);
    arc(l, c, 8.0f, 6.0f, 1.3f, 0.0f, 360.0f, x0, y0);
}

void draw_approx(lv_layer_t* l, lv_color_t c, int32_t x0, int32_t y0) {
    // i-approx: "M2.5 6.4c1.8-2 3.6-2 5.5 0s3.7 2 5.5 0" and the same 4.2 px lower.
    for (float dy : {0.0f, 4.2f}) {
        const float a[8] = {2.5f, 6.4f + dy, 4.3f, 4.4f + dy, 6.1f, 4.4f + dy, 8.0f, 6.4f + dy};
        const float b[8] = {8.0f, 6.4f + dy, 9.9f, 8.4f + dy, 11.7f, 8.4f + dy, 13.5f, 6.4f + dy};
        cubic(l, c, a, x0, y0);
        cubic(l, c, b, x0, y0);
    }
}

void draw(lv_event_t* e) {
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    const IconState* s = static_cast<const IconState*>(lv_obj_get_user_data(obj));
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    switch (s->icon) {
        case Icon::Pin: draw_pin(layer, s->color, a.x1, a.y1); break;
        case Icon::Wifi: draw_wifi(layer, s->color, a.x1, a.y1, false); break;
        case Icon::WifiOff: draw_wifi(layer, s->color, a.x1, a.y1, true); break;
        case Icon::Approx: draw_approx(layer, s->color, a.x1, a.y1); break;
    }
}

void on_delete(lv_event_t* e) {
    delete static_cast<IconState*>(lv_obj_get_user_data(static_cast<lv_obj_t*>(lv_event_get_target(e))));
}

}  // namespace

lv_obj_t* icon_create(lv_obj_t* parent, Icon icon, int32_t x, int32_t y, lv_color_t color) {
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_scrollable(obj, false);
    lv_obj_set_clickable(obj, false);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, 16, 16);
    lv_obj_set_user_data(obj, new IconState{icon, color});
    // Strokes reach slightly outside the 16 × 16 box; let the draw extend by 2 px.
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_add_event_cb(obj, draw, LV_EVENT_DRAW_MAIN, nullptr);
    lv_obj_add_event_cb(obj, on_delete, LV_EVENT_DELETE, nullptr);
    lv_obj_add_event_cb(
        obj, [](lv_event_t* e) { *static_cast<int32_t*>(lv_event_get_param(e)) = 2; }, LV_EVENT_REFR_EXT_DRAW_SIZE,
        nullptr);
    return obj;
}

void icon_set(lv_obj_t* obj, Icon icon, lv_color_t color) {
    IconState* s = static_cast<IconState*>(lv_obj_get_user_data(obj));
    if (s->icon == icon && lv_color_eq(s->color, color)) return;
    s->icon = icon;
    s->color = color;
    lv_obj_invalidate(obj);
}

}  // namespace nvp::ui
