#include "diag/diag_screen.h"

#include <cstdio>
#include <cstring>

#include "diag/palette.h"
#include "nvp/diag/report.h"

namespace nvp::ui::diag {
namespace {

using nvp::diag::CornerState;
using nvp::diag::kCornerCount;
using nvp::diag::kCornerTargets;
using nvp::diag::kTestCount;
using nvp::diag::Outcome;
using nvp::diag::TestName;

constexpr int32_t kW = 800;
constexpr int32_t kH = 480;

// Regions (contracts/diag-screens.md).
constexpr int32_t kBarsTop = 1, kBarsBottom = 179;
constexpr int32_t kFieldTop = 180, kFieldBottom = 279;
// Ten rows of 16 px from y=283 end at y=443, clear of the bottom corner targets (447..471).
constexpr int32_t kInfoX = 40, kInfoY = 283, kInfoW = 470, kRowH = 16;
constexpr int32_t kTargetSize = 24;

struct Bar {
    const char* name;
    uint32_t rgb;
    bool light;  // light bars get black text and black ticks
};
constexpr Bar kBars[8] = {
    {"WHITE", 0xFFFFFF, true},  {"YELLOW", 0xFFFF00, true}, {"CYAN", 0x00FFFF, true},
    {"GREEN", 0x00FF00, true},  {"MAGENTA", 0xFF00FF, false}, {"RED", 0xFF0000, false},
    {"BLUE", 0x0000FF, false},  {"BLACK", 0x000000, false},
};
constexpr int32_t kBarWidth = 100;

int32_t bar_index(int32_t x) {
    const int32_t i = (x - 1) / kBarWidth;
    return i < 0 ? 0 : (i > 7 ? 7 : i);
}

struct Ui {
    lv_obj_t* screen = nullptr;
    lv_obj_t* rows[6] = {};  // header, audio-off, wifi, backlight, heap, stress
    lv_obj_t* touch_row = nullptr;
    lv_obj_t* status_row = nullptr;
    lv_obj_t* tests[kTestCount] = {};
    lv_obj_t* heartbeat = nullptr;
    lv_obj_t* stress_label = nullptr;
    lv_obj_t* targets[kCornerCount] = {};
    lv_obj_t* target_labels[kCornerCount] = {};
    lv_obj_t* marker = nullptr;
    DiagActions actions;
};
Ui g;

void fill(lv_layer_t* layer, int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color) {
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = color;
    dsc.bg_opa = LV_OPA_COVER;
    dsc.radius = 0;
    dsc.border_width = 0;
    lv_area_t a = {x1, y1, x2, y2};
    lv_draw_rect(layer, &dsc, &a);
}

// Border, bars, ticks and the one-pixel line field (research R8, test pattern rationale).
void draw_pattern(lv_event_t* e) {
    lv_layer_t* layer = lv_event_get_layer(e);
    for (int32_t i = 0; i < 8; ++i) {
        const int32_t x1 = 1 + i * kBarWidth;
        const int32_t x2 = i == 7 ? kW - 2 : x1 + kBarWidth - 1;
        fill(layer, x1, kBarsTop, x2, kBarsBottom, lv_color_hex(kBars[i].rgb));
    }
    for (int32_t x = 10; x < kW - 1; x += 10) {  // ticks every 10 px, 3 px long, inside the border
        const lv_color_t c = kBars[bar_index(x)].light ? palette::black() : palette::white();
        fill(layer, x, 1, x, 3, c);
    }
    for (int32_t x = 2; x <= kW - 2; x += 2) {  // white on even x, black (background) on odd x
        fill(layer, x, kFieldTop, x, kFieldBottom, palette::white());
    }
    const lv_color_t w = palette::white();
    fill(layer, 0, 0, kW - 1, 0, w);
    fill(layer, 0, kH - 1, kW - 1, kH - 1, w);
    fill(layer, 0, 0, 0, kH - 1, w);
    fill(layer, kW - 1, 0, kW - 1, kH - 1, w);
}

lv_obj_t* plain(lv_obj_t* parent) {
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_clickable(o, false);
    lv_obj_set_scrollable(o, false);
    return o;
}

lv_obj_t* label(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const lv_font_t* font) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, palette::white(), 0);
    lv_obj_set_pos(l, x, y);
    if (w > 0) {
        lv_obj_set_width(l, w);
        lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_CLIP);
    }
    lv_label_set_text(l, "");
    return l;
}

// A label on a black box, used where text sits on the test pattern.
lv_obj_t* boxed_label(lv_obj_t* parent, const char* text, const lv_font_t* font) {
    lv_obj_t* l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, palette::white(), 0);
    lv_obj_set_style_bg_color(l, palette::black(), 0);
    lv_obj_set_style_bg_opa(l, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(l, 3, 0);
    return l;
}

void set_text(lv_obj_t* l, const char* text) {
    if (std::strcmp(lv_label_get_text(l), text) != 0) lv_label_set_text(l, text);
}

void set_color(lv_obj_t* l, lv_color_t c) {
    if (!lv_color_eq(lv_obj_get_style_text_color(l, LV_PART_MAIN), c)) lv_obj_set_style_text_color(l, c, 0);
}

void on_stress(lv_event_t*) {
    if (g.actions.stress_toggle) g.actions.stress_toggle(g.actions.ctx);
}

void on_settings(lv_event_t*) {
    if (g.actions.open_settings) g.actions.open_settings(g.actions.ctx);
}

void on_main(lv_event_t*) {
    if (g.actions.main_screen) g.actions.main_screen(g.actions.ctx);
}

lv_obj_t* button(lv_obj_t* parent, int32_t y, const char* text, lv_event_cb_t cb, lv_obj_t** text_label) {
    lv_obj_t* b = lv_button_create(parent);
    lv_obj_set_pos(b, 570, y);
    lv_obj_set_size(b, 180, 50);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
    lv_obj_center(l);
    if (text_label) *text_label = l;
    return b;
}

void create_targets(lv_obj_t* scr) {
    for (size_t i = 0; i < kCornerCount; ++i) {
        lv_obj_t* t = plain(scr);
        lv_obj_set_size(t, kTargetSize, kTargetSize);
        lv_obj_set_pos(t, kCornerTargets[i].x - kTargetSize / 2, kCornerTargets[i].y - kTargetSize / 2);
        lv_obj_set_style_bg_color(t, palette::black(), 0);
        lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
        lv_obj_t* h = plain(t);  // crosshair through the centre pixel
        lv_obj_set_size(h, kTargetSize, 1);
        lv_obj_set_pos(h, 0, kTargetSize / 2);
        lv_obj_set_style_bg_color(h, palette::white(), 0);
        lv_obj_set_style_bg_opa(h, LV_OPA_COVER, 0);
        lv_obj_t* v = plain(t);
        lv_obj_set_size(v, 1, kTargetSize);
        lv_obj_set_pos(v, kTargetSize / 2, 0);
        lv_obj_set_style_bg_color(v, palette::white(), 0);
        lv_obj_set_style_bg_opa(v, LV_OPA_COVER, 0);
        g.targets[i] = t;

        lv_obj_t* l = boxed_label(scr, "", &lv_font_montserrat_14);
        const bool right = kCornerTargets[i].x > kW / 2;
        const bool bottom = kCornerTargets[i].y > kH / 2;
        lv_obj_align(l, right ? (bottom ? LV_ALIGN_BOTTOM_RIGHT : LV_ALIGN_TOP_RIGHT)
                              : (bottom ? LV_ALIGN_BOTTOM_LEFT : LV_ALIGN_TOP_LEFT),
                     right ? -38 : 38, bottom ? -12 : 12);
        lv_obj_set_hidden(l, true);
        g.target_labels[i] = l;
    }
}

void create_usb_arrow(lv_obj_t* scr, Edge side) {
    char text[40];
    switch (side) {
        case Edge::Left:
            std::snprintf(text, sizeof text, "%s USB-C this side", LV_SYMBOL_LEFT);
            break;
        case Edge::Right:
            std::snprintf(text, sizeof text, "USB-C this side %s", LV_SYMBOL_RIGHT);
            break;
        case Edge::Top:
            std::snprintf(text, sizeof text, "%s USB-C this side", LV_SYMBOL_UP);
            break;
        case Edge::Bottom:
            std::snprintf(text, sizeof text, "%s USB-C this side", LV_SYMBOL_DOWN);
            break;
    }
    lv_obj_t* l = boxed_label(scr, text, &lv_font_montserrat_14);
    switch (side) {
        case Edge::Left: lv_obj_align(l, LV_ALIGN_LEFT_MID, 4, -10); break;
        case Edge::Right: lv_obj_align(l, LV_ALIGN_RIGHT_MID, -4, -10); break;
        case Edge::Top: lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 150); break;
        case Edge::Bottom: lv_obj_align(l, LV_ALIGN_BOTTOM_MID, 0, -6); break;
    }
}

const char* wifi_text(const nvp::diag::WifiLine& w, char* buf, size_t cap) {
    switch (w.kind) {
        case nvp::diag::WifiKind::Off: return "Wi-Fi: off";
        case nvp::diag::WifiKind::NotSetUp: return "Wi-Fi: not set up";
        case nvp::diag::WifiKind::Joining: std::snprintf(buf, cap, "Wi-Fi: joining %s", w.ssid); return buf;
        case nvp::diag::WifiKind::Connected:
            std::snprintf(buf, cap, "Wi-Fi: connected to %s, %d dBm", w.ssid, static_cast<int>(w.rssi_dbm));
            return buf;
        case nvp::diag::WifiKind::Hotspot:
            std::snprintf(buf, cap, "Wi-Fi: hotspot %s at %s", w.ap_ssid, w.ap_address);
            return buf;
    }
    return "Wi-Fi: unknown";
}

}  // namespace

lv_obj_t* diag_screen_create(Edge usb_side, const DiagActions& actions) {
    g = Ui{};
    g.actions = actions;
    lv_obj_t* scr = lv_obj_create(nullptr);
    g.screen = scr;
    lv_obj_set_scrollable(scr, false);
    lv_obj_set_style_bg_color(scr, palette::black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_add_event_cb(scr, draw_pattern, LV_EVENT_DRAW_MAIN, nullptr);

    // Bar labels at the bottom of each bar.
    for (int32_t i = 0; i < 8; ++i) {
        lv_obj_t* l = lv_label_create(scr);
        lv_label_set_text(l, kBars[i].name);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(l, kBars[i].light ? palette::black() : palette::white(), 0);
        lv_obj_set_width(l, kBarWidth);
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(l, 1 + i * kBarWidth, kBarsBottom - 22);
    }

    lv_obj_t* size = boxed_label(scr, "800 x 480", &lv_font_montserrat_20);
    lv_obj_align(size, LV_ALIGN_TOP_MID, 0, (kFieldTop + kFieldBottom) / 2 - 14);

    // Info panel.
    int32_t y = kInfoY;
    g.rows[0] = label(scr, kInfoX, y, kInfoW - 20, &lv_font_montserrat_14);
    y += kRowH;
    g.rows[1] = label(scr, kInfoX, y, kInfoW, &lv_font_montserrat_14);
    y += kRowH;
    for (size_t i = 0; i < kTestCount; ++i) {  // two columns, three rows
        const int32_t col = static_cast<int32_t>(i % 2);
        const int32_t row = static_cast<int32_t>(i / 2);
        g.tests[i] = label(scr, kInfoX + col * (kInfoW / 2), y + row * kRowH, kInfoW / 2 - 6,
                           &lv_font_montserrat_14);
    }
    y += 3 * kRowH;
    for (int r = 2; r < 6; ++r) {
        g.rows[r] = label(scr, kInfoX, y, kInfoW, &lv_font_montserrat_14);
        y += kRowH;
    }
    g.touch_row = label(scr, kInfoX, y, kInfoW / 2 - 6, &lv_font_montserrat_14);
    g.status_row = label(scr, kInfoX + kInfoW / 2, y, kInfoW / 2, &lv_font_montserrat_14);
    set_color(g.status_row, palette::muted());

    g.heartbeat = plain(scr);
    lv_obj_set_size(g.heartbeat, 10, 10);
    lv_obj_set_pos(g.heartbeat, kInfoX + kInfoW - 10, kInfoY + 4);
    lv_obj_set_style_radius(g.heartbeat, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(g.heartbeat, palette::ok(), 0);
    lv_obj_set_style_bg_opa(g.heartbeat, LV_OPA_COVER, 0);

    // Controls. No brightness control (OD1).
    button(scr, 200, "Main screen", on_main, nullptr);  // spec 003 FR-023 (clear of the checked rows 182 and 276)
    button(scr, 300, "Stress on", on_stress, &g.stress_label);
    button(scr, 370, "Settings", on_settings, nullptr);

    create_targets(scr);
    create_usb_arrow(scr, usb_side);

    // Touch marker: a 20 px ring, white with a black outline so it shows on every colour.
    g.marker = plain(scr);
    lv_obj_set_size(g.marker, 20, 20);
    lv_obj_set_style_radius(g.marker, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(g.marker, 2, 0);
    lv_obj_set_style_border_color(g.marker, palette::white(), 0);
    lv_obj_set_style_outline_width(g.marker, 1, 0);
    lv_obj_set_style_outline_color(g.marker, palette::black(), 0);
    lv_obj_set_hidden(g.marker, true);
    return scr;
}

void diag_screen_update(const DiagViewModel& vm) {
    if (g.screen == nullptr) return;
    char buf[96];

    std::snprintf(buf, sizeof buf, "Namaz Vakti Panel %s   %s %s", vm.firmware, vm.model, vm.revision);
    set_text(g.rows[0], buf);

    nvp::diag::report::format_silence_line(vm.silence, buf, sizeof buf);
    buf[std::strcspn(buf, "\n")] = '\0';
    set_text(g.rows[1], buf);
    set_color(g.rows[1], vm.silence.acknowledged ? palette::white() : palette::alert());

    for (size_t i = 0; i < kTestCount; ++i) {
        const nvp::diag::SelfTestResult& r = vm.tests[i];
        if (r.outcome == Outcome::Failed || r.outcome == Outcome::Skipped) {
            std::snprintf(buf, sizeof buf, "%s: %s (%s)", nvp::diag::test_name(static_cast<TestName>(i)),
                          nvp::diag::outcome_name(r.outcome), r.reason);
        } else {
            std::snprintf(buf, sizeof buf, "%s: %s", nvp::diag::test_name(static_cast<TestName>(i)),
                          nvp::diag::outcome_name(r.outcome));
        }
        set_text(g.tests[i], buf);
        const lv_color_t c = r.outcome == Outcome::Passed   ? palette::ok()
                             : r.outcome == Outcome::Failed ? palette::alert()
                                                            : palette::muted();
        set_color(g.tests[i], c);
    }

    set_text(g.rows[2], wifi_text(vm.wifi, buf, sizeof buf));
    set_text(g.rows[3], "Backlight: full (dimming disabled)");
    std::snprintf(buf, sizeof buf, "Heap: %lu B internal free, %lu B PSRAM free",
                  static_cast<unsigned long>(vm.heap_internal_free), static_cast<unsigned long>(vm.heap_psram_free));
    set_text(g.rows[4], buf);

    const StressView& s = vm.stress;
    if (!s.running) {
        std::snprintf(buf, sizeof buf, "Stress: off");
    } else {
        std::snprintf(buf, sizeof buf, "Stress: %s %lus, cycles %lu, scan %lu/%lu, write %lu/%lu, https %lu/%lu/%lu",
                      s.paused ? "paused" : "on", static_cast<unsigned long>(s.elapsed_s),
                      static_cast<unsigned long>(s.cycles), static_cast<unsigned long>(s.scan_ok),
                      static_cast<unsigned long>(s.scan_err), static_cast<unsigned long>(s.write_ok),
                      static_cast<unsigned long>(s.write_err), static_cast<unsigned long>(s.https_ok),
                      static_cast<unsigned long>(s.https_err), static_cast<unsigned long>(s.https_skip));
    }
    set_text(g.rows[5], buf);
    const bool stress_error = s.running && (s.scan_err + s.write_err + s.https_err) > 0;
    set_color(g.rows[5], stress_error ? palette::alert() : palette::white());
    set_text(g.stress_label, s.running ? "Stress off" : "Stress on");

    if (vm.touch_active) {
        std::snprintf(buf, sizeof buf, "Touch: %d, %d", static_cast<int>(vm.touch_x), static_cast<int>(vm.touch_y));
        lv_obj_set_pos(g.marker, vm.touch_x - 10, vm.touch_y - 10);
        lv_obj_set_hidden(g.marker, false);
    } else {
        std::snprintf(buf, sizeof buf, "Touch: no touch");
        lv_obj_set_hidden(g.marker, true);
    }
    set_text(g.touch_row, buf);
    set_text(g.status_row, vm.status);

    for (size_t i = 0; i < kCornerCount; ++i) {
        const nvp::diag::CornerResult& c = vm.corners[i];
        const lv_color_t bg = c.state == CornerState::Passed   ? palette::ok()
                              : c.state == CornerState::Failed ? palette::alert()
                                                               : palette::black();
        lv_obj_set_style_bg_color(g.targets[i], bg, 0);
        if (c.state == CornerState::NotTouched) {
            lv_obj_set_hidden(g.target_labels[i], true);
        } else {
            std::snprintf(buf, sizeof buf, "%+d,%+d", static_cast<int>(c.dx), static_cast<int>(c.dy));
            set_text(g.target_labels[i], buf);
            set_color(g.target_labels[i], c.state == CornerState::Passed ? palette::ok() : palette::alert());
            lv_obj_set_hidden(g.target_labels[i], false);
        }
    }

    lv_obj_set_style_bg_opa(g.heartbeat, vm.heartbeat ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
}

}  // namespace nvp::ui::diag
