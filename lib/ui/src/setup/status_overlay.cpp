#include "setup/status_overlay.h"

#include <cstring>

#include "fonts/fonts.h"
#include "main/mono_text.h"
#include "main/tokens.h"
#include "nvp/text/tr.h"
#include "setup/widgets.h"

namespace nvp::ui {
namespace {

using namespace setup;
namespace tr = nvp::text::tr;

// Geometry of 02 §7.4; baselines measured on reference 17 (spec 004 T057).
constexpr int32_t kPanelX = 100, kPanelY = 52, kPanelW = 600, kPanelH = 376, kPanelRadius = 16;
constexpr int32_t kPadSide = 26;
constexpr int32_t kLabelX = kPanelX + kPadSide;    // 126
constexpr int32_t kValueX = kLabelX + 150;          // label column 150 px
constexpr float kTitleBaseline = 102.0f;
constexpr float kRowBaseline[8] = {138.0f, 170.0f, 201.0f, 233.0f, 264.0f, 296.0f, 327.0f, 364.0f};
constexpr float kPinDigitsBaseline = 331.0f;
constexpr int32_t kQrSize = 132, kQrPadding = 8, kQrRadius = 10;
constexpr int32_t kQrX = kPanelX + kPanelW - 24 - kQrSize, kQrY = kPanelY + 70;
constexpr float kQrCaptionBaseline = 277.0f;
constexpr float kHintBaseline = 406.0f;
constexpr int32_t kScrimOpa = 184;  // 72 % of 255: the main screen stays visible at 28 %
// Confirmation (02 §7.4): two buttons 200x56, radius 12, at the panel's bottom right.
constexpr int32_t kButtonW = 200, kButtonH = 56, kButtonRadius = 12, kButtonGap = 16;
constexpr int32_t kButtonY = kPanelY + kPanelH - 22 - kButtonH;
constexpr int32_t kStartOverX = kPanelX + kPanelW - kPadSide - kButtonW;
constexpr int32_t kCancelX = kStartOverX - kButtonGap - kButtonW;
constexpr float kConfirmTextBaseline = 138.0f;

enum Row { Location, Source, LastUpdate, LastDay, Wifi, Address, Pin, Version, kRows };

struct Objs {
    lv_obj_t* scrim = nullptr;
    lv_obj_t* panel = nullptr;
    lv_obj_t* status = nullptr;  // everything of the status page
    lv_obj_t* confirm = nullptr; // everything of the confirmation
    lv_obj_t* values[kRows] = {};
    lv_obj_t* pin_digits = nullptr;
    QrBox qr;
    lv_obj_t* qr_caption = nullptr;
    bool open = false;
};
Objs g;

lv_obj_t* group(lv_obj_t* parent) {
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_scrollable(o, false);
    lv_obj_set_clickable(o, false);
    lv_obj_set_pos(o, 0, 0);
    lv_obj_set_size(o, tok::kWidth, tok::kHeight);
    return o;
}

lv_obj_t* button(lv_obj_t* parent, int32_t x, const char* text, bool primary) {
    lv_obj_t* b = box(parent, x, kButtonY, kButtonW, kButtonH, primary ? tok::alert() : tok::bg(), kButtonRadius);
    if (!primary) {
        lv_obj_set_style_bg_opa(b, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_color(b, tok::line(), LV_PART_MAIN);
        lv_obj_set_style_border_width(b, 2, LV_PART_MAIN);
    }
    lv_obj_t* l = lv_label_create(b);
    lv_obj_set_style_text_font(l, &nvp_txt_600_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, primary ? tok::bg() : tok::ink(), LV_PART_MAIN);
    lv_label_set_text_static(l, text);
    lv_obj_center(l);
    return b;
}

void build() {
    lv_obj_t* top = lv_layer_top();
    g.scrim = box(top, 0, 0, tok::kWidth, tok::kHeight, tok::bg(), 0);
    lv_obj_set_style_bg_opa(g.scrim, kScrimOpa, LV_PART_MAIN);
    g.panel = box(g.scrim, kPanelX, kPanelY, kPanelW, kPanelH, lv_color_hex(0x172552), kPanelRadius);  // `panel`
    lv_obj_set_style_border_color(g.panel, tok::line(), LV_PART_MAIN);
    lv_obj_set_style_border_width(g.panel, 1, LV_PART_MAIN);
    // Children are placed in screen coordinates: two full-screen groups over the scrim.
    g.status = group(g.scrim);
    g.confirm = group(g.scrim);

    lv_label_set_text_static(text_label(g.status, &nvp_txt_600_26, tok::ink(), kLabelX, kTitleBaseline),
                             tr::kOverlayTitle);
    for (int r = 0; r < kRows; ++r) {
        lv_label_set_text_static(text_label(g.status, &nvp_txt_400_15, tok::muted(), kLabelX, kRowBaseline[r]),
                                 tr::kOverlayLabels[r]);
        g.values[r] = text_label(g.status, &nvp_txt_400_18, tok::ink(), kValueX, kRowBaseline[r]);
    }
    g.pin_digits = mono_text_create(g.status, &nvp_num_21, 21);
    mono_text_set_color(g.pin_digits, tok::now());
    mono_text_place(g.pin_digits, kValueX, kPinDigitsBaseline);
    qr_box_create(g.qr, g.status, kQrX, kQrY, kQrSize, kQrPadding, kQrRadius);
    g.qr_caption = centred_label(g.status, &nvp_txt_400_15, tok::muted(), kQrX + kQrSize / 2, kQrCaptionBaseline);
    lv_label_set_text_static(g.qr_caption, tr::kOverlayQrCaption);
    centre_on(g.qr_caption, kQrX + kQrSize / 2);
    lv_label_set_text_static(text_label(g.status, &nvp_txt_400_15, tok::muted(), kLabelX, kHintBaseline),
                             tr::kOverlayHint);

    lv_label_set_text_static(text_label(g.confirm, &nvp_txt_600_26, tok::ink(), kLabelX, kTitleBaseline),
                             tr::kConfirmTitle);
    lv_obj_t* text = paragraph(g.confirm, &nvp_txt_400_18, tok::ink(), kLabelX, kConfirmTextBaseline,
                               kPanelW - 2 * kPadSide, 26);
    lv_label_set_text_static(text, tr::kConfirmText);
    button(g.confirm, kCancelX, tr::kConfirmCancel, false);
    button(g.confirm, kStartOverX, tr::kConfirmStartOver, true);
}

bool is_dash(const char* s) { return std::strcmp(s, tr::kDash) == 0; }

}  // namespace

void overlay_open(const net::OverlayView& v) {
    if (g.scrim == nullptr) build();
    set_hidden(g.scrim, false);
    overlay_show_confirm(false);
    g.open = true;
    overlay_update(v);
}

void overlay_update(const net::OverlayView& v) {
    if (g.scrim == nullptr) return;
    const char* texts[kRows] = {v.location, v.source, v.last_update, v.last_day, v.wifi, v.address, v.pin, v.version};
    for (int r = 0; r < kRows; ++r) {
        const bool dash = is_dash(texts[r]);
        // A value a later slice fills is a `dim` dash (analysis U3); the PIN, when known, is
        // Barlow Cond. 600 in `now` (02 §7.4).
        if (r == Pin) {
            set_hidden(g.pin_digits, dash);
            set_hidden(g.values[r], !dash);
            if (!dash) mono_text_set(g.pin_digits, texts[r]);
        }
        set_text(g.values[r], texts[r]);
        set_color(g.values[r], dash ? tok::dim() : tok::ink());
    }
    set_hidden(g.qr.square, !v.show_qr);
    set_hidden(g.qr_caption, !v.show_qr);
    if (v.show_qr) qr_box_set(g.qr, v.qr_url);
}

void overlay_show_confirm(bool on) {
    if (g.scrim == nullptr) return;
    set_hidden(g.status, on);
    set_hidden(g.confirm, !on);
}

void overlay_close() {
    if (g.scrim == nullptr) return;
    set_hidden(g.scrim, true);
    g.open = false;
}

bool overlay_is_open() { return g.open; }

bool overlay_confirm_shown() { return g.open && !lv_obj_is_hidden(g.confirm); }

OverlayHit overlay_hit(int32_t x, int32_t y) {
    if (!overlay_confirm_shown() || y < kButtonY || y >= kButtonY + kButtonH) return OverlayHit::None;
    if (x >= kCancelX && x < kCancelX + kButtonW) return OverlayHit::Cancel;
    if (x >= kStartOverX && x < kStartOverX + kButtonW) return OverlayHit::StartOver;
    return OverlayHit::None;
}

}  // namespace nvp::ui
