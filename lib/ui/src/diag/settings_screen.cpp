#include "diag/settings_screen.h"

#include <cstdio>
#include <cstring>

#include "diag/palette.h"

namespace nvp::ui::diag {
namespace {

using nvp::diag::WifiKind;

struct Ui {
    lv_obj_t* screen = nullptr;
    lv_obj_t* wifi_line = nullptr;
    lv_obj_t* start_btn = nullptr;
    lv_obj_t* cancel_btn = nullptr;
    lv_obj_t* forget_btn = nullptr;
    lv_obj_t* hotspot_box = nullptr;
    lv_obj_t* hotspot_name = nullptr;
    lv_obj_t* hotspot_pass = nullptr;
    lv_obj_t* hotspot_url = nullptr;
    lv_obj_t* message = nullptr;
    lv_obj_t* status = nullptr;
    lv_obj_t* confirm_box = nullptr;
    lv_obj_t* confirm_text = nullptr;
    DiagActions actions;
};
Ui g;

void call(void (*fn)(void*)) {
    if (fn != nullptr) fn(g.actions.ctx);
}

void on_start(lv_event_t*) { call(g.actions.request_setup); }
void on_cancel(lv_event_t*) { call(g.actions.cancel_setup); }
void on_forget(lv_event_t*) { call(g.actions.request_forget); }
void on_yes(lv_event_t*) { call(g.actions.confirm_yes); }
void on_no(lv_event_t*) { call(g.actions.confirm_no); }
void on_back(lv_event_t*) { call(g.actions.back); }

lv_obj_t* text(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const lv_font_t* font, lv_color_t color) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_set_pos(l, x, y);
    lv_obj_set_width(l, w);
    lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_WRAP);
    lv_label_set_text(l, "");
    return l;
}

lv_obj_t* button(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const char* label, lv_event_cb_t cb) {
    lv_obj_t* b = lv_button_create(parent);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, 56);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, label);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
    lv_obj_center(l);
    return b;
}

void set_text(lv_obj_t* l, const char* t) {
    if (std::strcmp(lv_label_get_text(l), t) != 0) lv_label_set_text(l, t);
}

}  // namespace

lv_obj_t* settings_screen_create(const DiagActions& actions) {
    g = Ui{};
    g.actions = actions;
    lv_obj_t* scr = lv_obj_create(nullptr);
    g.screen = scr;
    lv_obj_set_scrollable(scr, false);
    lv_obj_set_style_bg_color(scr, palette::black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t* title = text(scr, 32, 24, 400, &lv_font_montserrat_20, palette::white());
    lv_label_set_text(title, "Settings");
    g.wifi_line = text(scr, 32, 70, 736, &lv_font_montserrat_20, palette::white());

    g.start_btn = button(scr, 32, 130, 280, "Start Wi-Fi setup", on_start);
    g.cancel_btn = button(scr, 32, 130, 280, "Cancel setup", on_cancel);
    g.forget_btn = button(scr, 32, 206, 280, "Forget Wi-Fi", on_forget);
    button(scr, 32, 392, 180, "Back", on_back);

    // Hotspot details, shown while the hotspot is open (FR-020).
    g.hotspot_box = lv_obj_create(scr);
    lv_obj_remove_style_all(g.hotspot_box);
    lv_obj_set_pos(g.hotspot_box, 360, 130);
    lv_obj_set_size(g.hotspot_box, 408, 160);
    g.hotspot_name = text(g.hotspot_box, 0, 0, 408, &lv_font_montserrat_20, palette::white());
    g.hotspot_pass = text(g.hotspot_box, 0, 40, 408, &lv_font_montserrat_20, palette::white());
    g.hotspot_url = text(g.hotspot_box, 0, 80, 408, &lv_font_montserrat_20, palette::white());

    g.message = text(scr, 360, 300, 408, &lv_font_montserrat_20, palette::white());
    g.status = text(scr, 240, 408, 528, &lv_font_montserrat_14, palette::muted());

    // Confirmation (FR-019a): Yes / No; the app closes it after 15 s.
    g.confirm_box = lv_obj_create(scr);
    lv_obj_set_size(g.confirm_box, 440, 200);
    lv_obj_center(g.confirm_box);
    lv_obj_set_scrollable(g.confirm_box, false);
    lv_obj_set_style_bg_color(g.confirm_box, lv_color_hex(0x1C1C1E), 0);
    lv_obj_set_style_border_color(g.confirm_box, palette::white(), 0);
    lv_obj_set_style_border_width(g.confirm_box, 2, 0);
    g.confirm_text = lv_label_create(g.confirm_box);
    lv_obj_set_style_text_font(g.confirm_text, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(g.confirm_text, palette::white(), 0);
    lv_obj_set_width(g.confirm_text, 400);
    lv_obj_set_style_text_align(g.confirm_text, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(g.confirm_text, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_t* yes = button(g.confirm_box, 20, 110, 180, "Yes", on_yes);
    lv_obj_t* no = button(g.confirm_box, 216, 110, 180, "No", on_no);
    (void)yes;
    (void)no;
    lv_obj_set_hidden(g.confirm_box, true);
    return scr;
}

void settings_screen_update(const DiagViewModel& vm) {
    if (g.screen == nullptr) return;
    char buf[96];
    const nvp::diag::WifiLine& w = vm.wifi;
    switch (w.kind) {
        case WifiKind::Off:
        case WifiKind::NotSetUp: std::snprintf(buf, sizeof buf, "Wi-Fi: Not set up"); break;
        case WifiKind::Joining: std::snprintf(buf, sizeof buf, "Wi-Fi: Joining %s", w.ssid); break;
        case WifiKind::Connected: std::snprintf(buf, sizeof buf, "Wi-Fi: Connected to %s", w.ssid); break;
        case WifiKind::Hotspot: std::snprintf(buf, sizeof buf, "Wi-Fi: Hotspot open"); break;
    }
    set_text(g.wifi_line, buf);

    lv_obj_set_hidden(g.start_btn, vm.hotspot_open);
    lv_obj_set_hidden(g.cancel_btn, !vm.hotspot_open);
    lv_obj_set_hidden(g.forget_btn, vm.hotspot_open);  // ignored while the hotspot is open
    lv_obj_set_hidden(g.hotspot_box, !vm.hotspot_open);
    if (vm.hotspot_open) {
        std::snprintf(buf, sizeof buf, "Hotspot: %s", w.ap_ssid);
        set_text(g.hotspot_name, buf);
        std::snprintf(buf, sizeof buf, "Password: %s", vm.hotspot_password);
        set_text(g.hotspot_pass, buf);
        std::snprintf(buf, sizeof buf, "Open http://%s/", w.ap_address);
        set_text(g.hotspot_url, buf);
    }
    set_text(g.message, vm.setup_message);
    set_text(g.status, vm.status);

    if (vm.confirm == Confirm::None) {
        lv_obj_set_hidden(g.confirm_box, true);
    } else {
        if (vm.confirm == Confirm::StartSetup) {
            std::snprintf(buf, sizeof buf, "Start hotspot?");
        } else {
            std::snprintf(buf, sizeof buf, "Forget %s?", w.ssid[0] ? w.ssid : "Wi-Fi");
        }
        set_text(g.confirm_text, buf);
        lv_obj_set_hidden(g.confirm_box, false);
    }
}

}  // namespace nvp::ui::diag
