#include "main/timeline.h"

#include <cstdio>
#include <cstring>

#include "fonts/fonts.h"
#include "main/tokens.h"

namespace nvp::ui {
namespace {

using namespace tok;

lv_obj_t* g_band = nullptr;    // scale + strip + leaders, y 236..336
lv_obj_t* g_marker = nullptr;  // gap, line and triangle
TimelineState g_state;
int32_t g_marker_x = -1000;

constexpr int32_t kBandY = 236;
constexpr int32_t kBandH = 336 - 236;
constexpr int32_t kMarkerHalf = 10;  // marker object spans x-10 .. x+10
constexpr int32_t kMarkerTop = 248;
constexpr int32_t kMarkerBottom = 300;

void fill(lv_layer_t* layer, lv_color_t c, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t radius = 0) {
    if (x2 < x1 || y2 < y1) return;
    lv_draw_fill_dsc_t d;
    lv_draw_fill_dsc_init(&d);
    d.color = c;
    d.radius = radius;
    lv_area_t a{x1, y1, x2, y2};
    lv_draw_fill(layer, &d, &a);
}

void vline(lv_layer_t* layer, lv_color_t c, float x, int32_t y0, int32_t y1, int32_t width) {
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.color = c;
    d.width = width;
    d.p1.x = x;
    d.p1.y = y0;
    d.p2.x = x;
    d.p2.y = y1;
    lv_draw_line(layer, &d);
}

void draw_scale(lv_layer_t* layer) {
    lv_draw_label_dsc_t ld;
    lv_draw_label_dsc_init(&ld);
    ld.font = &nvp_txt_400_14;  // hour labels use the 14 px font (02 §2.2 consolidation)
    ld.color = muted();
    const int32_t top = top_for_baseline(&nvp_txt_400_14, kHourLabelBaseline);
    for (int h = 0; h <= 24; ++h) {
        const float x = x_of_seconds(h * 3600);
        if (h % 3 == 0) {
            char text[4];
            std::snprintf(text, sizeof text, "%02d", h);
            const int32_t w = lv_font_get_glyph_width(&nvp_txt_400_14, text[0], text[1]) +
                              lv_font_get_glyph_width(&nvp_txt_400_14, text[1], 0);
            lv_point_t p{round_px(x - w / 2.0f), top};
            lv_draw_character(layer, &ld, &p, static_cast<uint32_t>(text[0]));
            p.x += lv_font_get_glyph_width(&nvp_txt_400_14, text[0], text[1]);
            lv_draw_character(layer, &ld, &p, static_cast<uint32_t>(text[1]));
            vline(layer, tick(), x, kTickMajorY0, kTickY1, 2);  // 1.5 px in the reference
        } else {
            vline(layer, tick(), x, kTickMinorY0, kTickY1, 1);
        }
    }
}

// 45° stripes in `bg`, 3 px wide and 6 px apart measured across the stripe, over [xa, xb) of the
// strip. Drawn as one-pixel rows of runs so the clip to the interval is exact.
void draw_hatch(lv_layer_t* layer, float xa, float xb) {
    const int32_t x0 = round_px(xa), x1 = round_px(xb) - 1;
    const float period = kHatchPeriod * 1.41421356f;  // horizontal period of a 45° stripe
    const float run = kHatchStripe * 1.41421356f;
    for (int32_t y = kStripY; y < kStripY + kStripH; ++y) {
        // Stripe k covers x in [k*period - y, k*period - y + run) (rising to the right).
        const float shift = static_cast<float>(y);
        int k = static_cast<int>((x0 + shift) / period) - 1;
        for (;; ++k) {
            const float s = k * period - shift;
            if (s > x1) break;
            const int32_t a = round_px(s) < x0 ? x0 : round_px(s);
            const int32_t b = round_px(s + run) - 1 > x1 ? x1 : round_px(s + run) - 1;
            fill(layer, bg(), a, y, b, y);
        }
    }
}

void draw_strip(lv_layer_t* layer) {
    const int32_t x0 = round_px(kTimelineX0), x1 = round_px(kTimelineX0 + kTimelineWidth) - 1;
    const int32_t y0 = kStripY, y1 = kStripY + kStripH - 1;
    // Night colour over the whole rounded strip: both ends (00:00→İmsak, Yatsı→24:00) are `yatsi`.
    fill(layer, strip_night(), x0, y0, x1, y1, kStripRadius);
    if (!g_state.segments) return;
    for (int k = 0; k < 5; ++k) {
        const int32_t a = round_px(x_of_seconds(g_state.minutes[k] * 60));
        const int32_t b = round_px(x_of_seconds(g_state.minutes[k + 1] * 60)) - 1;
        fill(layer, strip(k), a, y0, b, y1);
    }
    for (int i = 0; i < 3; ++i) {
        if (g_state.hatch[i]) draw_hatch(layer, x_of_seconds(g_state.hatch_start[i]), x_of_seconds(g_state.hatch_end[i]));
    }
    for (int k = 0; k < 6; ++k) {  // 2 px separators centred on X(t)
        const int32_t a = round_px(x_of_seconds(g_state.minutes[k] * 60) - kSeparatorW / 2.0f);
        fill(layer, bg(), a, y0, a + kSeparatorW - 1, y1);
    }
}

void draw_leaders(lv_layer_t* layer) {
    if (!g_state.segments) return;
    for (int pass = 0; pass < 2; ++pass) {  // current leader last, on top
        for (int k = 0; k < 6; ++k) {
            const bool current = k == g_state.current;
            if (current != (pass == 1)) continue;
            lv_point_precise_t pts[4];
            const float x = x_of_seconds(g_state.minutes[k] * 60);
            const float cx = column_cx(k);
            pts[0] = {x, static_cast<float>(kLeaderY0)};
            pts[1] = {x, static_cast<float>(kLeaderY1)};
            pts[2] = {cx, static_cast<float>(kLeaderY2)};
            pts[3] = {cx, static_cast<float>(kLeaderY3)};
            lv_draw_line_dsc_t d;
            lv_draw_line_dsc_init(&d);
            d.color = current ? now() : line();
            d.width = current ? 3 : 2;  // 2.5 px and 1.5 px in the reference
            d.points = pts;
            d.point_cnt = 4;
            d.round_start = 1;
            d.round_end = 1;
            lv_draw_line(layer, &d);
        }
    }
}

void draw_band(lv_event_t* e) {
    lv_layer_t* layer = lv_event_get_layer(e);
    draw_scale(layer);
    draw_strip(layer);
    draw_leaders(layer);
}

void draw_marker(lv_event_t* e) {
    lv_layer_t* layer = lv_event_get_layer(e);
    const float x = static_cast<float>(g_marker_x);
    // 1. 9 px gap in bg, 2. 3 px line in now, y 259..299 (02 §4.3).
    fill(layer, bg(), round_px(x - kMarkerGapW / 2.0f), kMarkerY0, round_px(x - kMarkerGapW / 2.0f) + kMarkerGapW - 1,
         kMarkerY1);
    fill(layer, now(), round_px(x - kMarkerW / 2.0f), kMarkerY0, round_px(x - kMarkerW / 2.0f) + kMarkerW - 1, kMarkerY1);
    // 3. Triangle (x-7,251) (x+7,251) (x,261) in now with a 2 px bg outline.
    lv_draw_triangle_dsc_t t;
    lv_draw_triangle_dsc_init(&t);
    t.color = bg();
    t.p[0] = {x - 9.8f, 249.0f};
    t.p[1] = {x + 9.8f, 249.0f};
    t.p[2] = {x, 263.8f};
    lv_draw_triangle(layer, &t);
    t.color = now();
    t.p[0] = {x - 7.0f, 251.0f};
    t.p[1] = {x + 7.0f, 251.0f};
    t.p[2] = {x, 261.0f};
    lv_draw_triangle(layer, &t);
}

lv_obj_t* plain(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h) {
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_scrollable(o, false);
    lv_obj_set_clickable(o, false);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    return o;
}

}  // namespace

bool operator==(const TimelineState& a, const TimelineState& b) {
    if (a.segments != b.segments || a.current != b.current) return false;
    for (int i = 0; i < 6; ++i) {
        if (a.minutes[i] != b.minutes[i]) return false;
    }
    for (int i = 0; i < 3; ++i) {
        if (a.hatch[i] != b.hatch[i] || a.hatch_start[i] != b.hatch_start[i] || a.hatch_end[i] != b.hatch_end[i]) {
            return false;
        }
    }
    return true;
}

void timeline_create(lv_obj_t* parent) {
    g_state = TimelineState{};
    g_marker_x = -1000;
    g_band = plain(parent, 0, kBandY, kWidth, kBandH);
    lv_obj_add_event_cb(g_band, draw_band, LV_EVENT_DRAW_MAIN, nullptr);
    g_marker = plain(parent, 0, kMarkerTop, 2 * kMarkerHalf + 1, kMarkerBottom - kMarkerTop);
    lv_obj_add_event_cb(g_marker, draw_marker, LV_EVENT_DRAW_MAIN, nullptr);
    lv_obj_set_hidden(g_marker, true);
}

void timeline_set(const TimelineState& s) {
    if (s == g_state) return;
    g_state = s;
    lv_obj_invalidate(g_band);
    lv_obj_invalidate(g_marker);  // the marker sits on top of the band
}

void marker_set(bool show, int32_t second_of_day) {
    const int32_t x = round_px(x_of_seconds(second_of_day));
    const bool hidden = lv_obj_is_hidden(g_marker);
    if (show == !hidden && x == g_marker_x) return;
    g_marker_x = x;
    lv_obj_set_pos(g_marker, x - kMarkerHalf, kMarkerTop);
    lv_obj_set_hidden(g_marker, !show);
}

lv_obj_t* marker_obj() { return g_marker; }

}  // namespace nvp::ui
