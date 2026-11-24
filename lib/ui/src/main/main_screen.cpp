#include "main/main_screen.h"

#include <cstring>

#include "fonts/fonts.h"
#include "main/columns.h"
#include "main/icons.h"
#include "main/mono_text.h"
#include "main/timeline.h"
#include "main/tokens.h"
#include "nvp/screen/status_text.h"
#include "nvp/text/tr.h"

namespace nvp::ui {
namespace {

using namespace tok;
using screen::SystemState;
namespace tr = nvp::text::tr;

struct Objs {
    lv_obj_t* screen;
    lv_obj_t* pin;
    lv_obj_t* city;
    lv_obj_t* status_icon;
    lv_obj_t* status;
    lv_obj_t* clock;
    lv_obj_t* seconds;
    lv_obj_t* date;
    lv_obj_t* hijri;
    lv_obj_t* swatch;
    lv_obj_t* legend;
    lv_obj_t* period;
    lv_obj_t* bound_start;
    lv_obj_t* bound_end;
    lv_obj_t* track;
    lv_obj_t* fill;
    lv_obj_t* label;
    lv_obj_t* countdown;
};
Objs g;

lv_obj_t* text_label(const lv_font_t* font, lv_color_t color, int32_t x, float baseline) {
    lv_obj_t* l = lv_label_create(g.screen);
    lv_obj_set_style_text_font(l, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, color, LV_PART_MAIN);
    lv_obj_set_pos(l, x, top_for_baseline(font, baseline));
    lv_label_set_text_static(l, "");
    return l;
}

lv_obj_t* box(int32_t x, int32_t y, int32_t w, int32_t h, lv_color_t c, int32_t radius) {
    lv_obj_t* o = lv_obj_create(g.screen);
    lv_obj_remove_style_all(o);
    lv_obj_set_scrollable(o, false);
    lv_obj_set_clickable(o, false);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, c, LV_PART_MAIN);
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

void set_x(lv_obj_t* o, int32_t x) {
    if (lv_obj_get_style_x(o, LV_PART_MAIN) != x) lv_obj_set_x(o, x);
}

void set_hidden(lv_obj_t* o, bool hidden) {
    if (lv_obj_is_hidden(o) != hidden) lv_obj_set_hidden(o, hidden);
}

// City (M2): cut at a UTF-8 character boundary and end with "…" when wider than 300 px.
void set_city(const char* city) {
    char buf[64];
    std::strncpy(buf, city, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';
    if (text_width(buf, &nvp_txt_600_18) > kCityMaxW) {
        size_t n = std::strlen(buf);
        char cut[72];
        while (n > 0) {
            --n;
            while (n > 0 && (static_cast<unsigned char>(buf[n]) & 0xC0) == 0x80) --n;  // char start
            std::memcpy(cut, buf, n);
            std::strcpy(cut + n, tr::kEllipsis);
            if (text_width(cut, &nvp_txt_600_18) <= kCityMaxW) break;
        }
        std::strncpy(buf, cut, sizeof buf - 1);
        buf[sizeof buf - 1] = '\0';
    }
    set_text(g.city, buf);
}

// Status (M3, M4): right edge 776, icon 8 px left of the text.
void set_status(const SystemView& s) {
    char text[64];
    screen::status_text(s.state, s.last_update, s.days_left, text, sizeof text);
    const lv_color_t c = screen::status_is_alert(s.state) ? alert() : muted();
    set_text(g.status, text);
    set_color(g.status, c);
    const int32_t w = text_width(text, &nvp_txt_400_14);
    set_x(g.status, kStatusRight - w);
    Icon icon = Icon::Wifi;
    switch (screen::status_icon(s.state)) {
        case screen::StatusIcon::Wifi: icon = Icon::Wifi; break;
        case screen::StatusIcon::WifiOff: icon = Icon::WifiOff; break;
        case screen::StatusIcon::Approx: icon = Icon::Approx; break;
    }
    icon_set(g.status_icon, icon, c);
    set_x(g.status_icon, kStatusRight - w - kStatusIconGap - kIconSize);
}

// Legend swatch (M9): 22 × 10, radius 2, in the strip's Güneş colour with the kerahat hatch.
void draw_swatch(lv_event_t* e) {
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_area_t a;
    lv_obj_get_coords(static_cast<lv_obj_t*>(lv_event_get_target(e)), &a);
    lv_draw_fill_dsc_t d;
    lv_draw_fill_dsc_init(&d);
    d.color = strip(1);
    d.radius = 2;
    lv_draw_fill(layer, &d, &a);
    d.radius = 0;
    d.color = bg();
    const float period = kHatchPeriod * 1.41421356f, run = kHatchStripe * 1.41421356f;
    for (int32_t y = a.y1; y <= a.y2; ++y) {
        for (int k = -2; k < 8; ++k) {
            const float s = a.x1 + k * period - (y - a.y1);
            int32_t x0 = round_px(s), x1 = round_px(s + run) - 1;
            if (x0 < a.x1) x0 = a.x1;
            if (x1 > a.x2) x1 = a.x2;
            if (x1 < x0) continue;
            lv_area_t r{x0, y, x1, y};
            lv_draw_fill(layer, &d, &r);
        }
    }
}

}  // namespace

lv_obj_t* main_screen_create() {
    g = Objs{};
    g.screen = lv_obj_create(nullptr);
    lv_obj_remove_style_all(g.screen);
    lv_obj_set_style_bg_color(g.screen, bg(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g.screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollable(g.screen, false);

    // Status row (M1..M4).
    g.pin = icon_create(g.screen, Icon::Pin, kPinX, kPinY, ink());
    g.city = text_label(&nvp_txt_600_18, ink(), kCityX, kCityBaseline);
    g.status_icon = icon_create(g.screen, Icon::Wifi, kStatusRight - 16, kPinY, muted());
    g.status = text_label(&nvp_txt_400_14, muted(), kStatusRight, kStatusBaseline);

    // Hero, left (M5..M9).
    g.clock = mono_text_create(g.screen, &nvp_num_96, 96);
    g.seconds = mono_text_create(g.screen, &nvp_num_46, 46);
    mono_text_set_color(g.seconds, muted());
    g.date = text_label(&nvp_txt_500_21, ink(), kDateX, kDateBaseline);
    g.hijri = text_label(&nvp_txt_400_17, muted(), kDateX, kHijriBaseline);
    g.swatch = box(kLegendSwatchX, kLegendSwatchY, kLegendSwatchW, kLegendSwatchH, strip(1), 2);
    lv_obj_add_event_cb(g.swatch, draw_swatch, LV_EVENT_DRAW_MAIN, nullptr);
    lv_obj_set_style_bg_opa(g.swatch, LV_OPA_TRANSP, LV_PART_MAIN);
    g.legend = text_label(&nvp_txt_400_14, muted(), kLegendTextX, kLegendBaseline);
    lv_label_set_text_static(g.legend, tr::kKerahatLegend);

    // Hero, right (M10..M15).
    g.period = text_label(&nvp_txt_600_26, ink(), kPeriodX, kPeriodBaseline);
    g.bound_start = mono_text_create(g.screen, &nvp_num_25, 25);
    g.bound_end = mono_text_create(g.screen, &nvp_num_25, 25);
    g.track = box(kTrackX, kTrackY, kTrackW, kTrackH, track(), kTrackRadius);
    g.fill = box(kTrackX, kTrackY, 0, kTrackH, now(), kTrackRadius);
    g.label = text_label(&nvp_txt_400_20, muted(), kLabelX, kLabelBaseline);
    g.countdown = mono_text_create(g.screen, &nvp_num_114, 114);
    mono_text_place(g.countdown, kCountdownX, kCountdownBaseline);

    timeline_create(g.screen);
    columns_create(g.screen);
    return g.screen;
}

void main_screen_update(const engine::MainView& v, const SystemView& s) {
    const bool no_time = s.state == SystemState::NoTime;
    const bool no_data = s.state == SystemState::NoData;
    const bool running = !no_time && !no_data && v.has_data;

    set_city(s.city);
    set_status(s);

    // Clock (M5, M6): "--:--" without seconds when the time is not valid.
    mono_text_set(g.clock, no_time ? "--:--" : v.clock_hhmm);
    mono_text_place(g.clock, kClockX, kClockBaseline);
    set_hidden(g.seconds, no_time);
    if (!no_time) {
        mono_text_set(g.seconds, v.clock_ss);
        mono_text_place(g.seconds, kClockX + mono_text_width(g.clock) + kSecondsGap, kClockBaseline);
    }

    // Date and Hijri (M7, M8).
    set_text(g.date, no_time ? tr::kHeroNoTimeDate : v.date_text);
    set_text(g.hijri, no_time ? "" : (no_data ? s.hijri_fallback : v.hijri_text));

    // Period, bounds, progress, label, countdown (M10..M15; 02 §5, §6).
    if (no_time) {
        set_text(g.period, tr::kHeroNoTimeTitle);
        set_text(g.label, tr::kHeroNoTimeLabel);
    } else if (no_data || !v.has_data) {
        set_text(g.period, tr::kHeroNoDataTitle);
        set_text(g.label, tr::kHeroNoDataLabel);
    } else {
        set_text(g.period, v.period_name);
        set_text(g.label, v.countdown_label);
    }
    set_hidden(g.bound_start, !running);
    set_hidden(g.bound_end, !running);
    set_hidden(g.track, !running);
    set_hidden(g.fill, !running);
    if (running) {
        mono_text_set(g.bound_start, v.start_text);
        mono_text_set(g.bound_end, v.end_text);
        mono_text_place(g.bound_start, kBoundStartRight - mono_text_width(g.bound_start), kBoundsBaseline);
        mono_text_place(g.bound_end, kBoundEndRight - mono_text_width(g.bound_end), kBoundsBaseline);
        const int32_t w = round_px(v.fraction * kTrackW);
        if (lv_obj_get_width(g.fill) != w) lv_obj_set_width(g.fill, w);
        set_hidden(g.fill, w == 0);
    }
    mono_text_set(g.countdown, running ? v.countdown_text : "--:--:--");
    mono_text_set_color(g.countdown, !running ? dim() : (v.soon ? alert() : now()));

    // Timeline (M16..M20) and marker (M19).
    TimelineState t;
    t.segments = !no_data && v.has_stored_times;
    for (int k = 0; k < 6; ++k) t.minutes[k] = v.strip_minutes[k];
    for (int i = 0; i < 3; ++i) {
        t.hatch[i] = t.segments && s.kerahat_enabled && v.kerahat[i].shown;
        t.hatch_start[i] = v.kerahat[i].start_s;
        t.hatch_end[i] = v.kerahat[i].end_s;
    }
    t.current = -1;
    if (running) {
        for (int k = 0; k < 6; ++k) {
            if (v.columns[k].state == engine::ColumnState::Current) t.current = static_cast<int8_t>(k);
        }
    }
    timeline_set(t);
    marker_set(!no_time, v.now_second_of_day);
    const bool legend = t.hatch[0] || t.hatch[1] || t.hatch[2];
    set_hidden(g.swatch, !legend);
    set_hidden(g.legend, !legend);

    // Columns (M21).
    ColumnsMode mode = ColumnsMode::Normal;
    if (no_time) {
        mode = v.has_stored_times ? ColumnsMode::StoredOnly : ColumnsMode::Placeholder;
    } else if (no_data || !v.has_data) {
        mode = ColumnsMode::Placeholder;
    } else if (s.state == SystemState::Calculated) {
        mode = ColumnsMode::Calculated;
    }
    columns_set(v, mode);
}

lv_obj_t* main_screen_city_label() { return g.city; }
lv_obj_t* main_screen_marker() { return marker_obj(); }

}  // namespace nvp::ui
