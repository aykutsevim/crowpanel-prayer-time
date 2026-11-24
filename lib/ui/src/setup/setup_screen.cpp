#include "setup/setup_screen.h"

#include "fonts/fonts.h"
#include "main/mono_text.h"
#include "main/tokens.h"
#include "nvp/text/tr.h"
#include "setup/widgets.h"

namespace nvp::ui {
namespace {

using namespace setup;
namespace tr = nvp::text::tr;

// Geometry of 02 §7.2, with baselines measured on reference 15 (spec 004 T035).
constexpr int32_t kLeft = 24;
constexpr float kTitleBaseline = 45.0f, kSubtitleBaseline = 69.0f;
constexpr int32_t kCircle = 36, kCircleBorder = 2;
constexpr int32_t kCircleTop[3] = {92, 252, 318};
constexpr int32_t kStepX = 72, kStepWidth = 402, kStepPitch = 23;  // step list 24..474 (width 450)
constexpr float kStep1Baseline = 110.0f, kStep2Baseline = 270.0f, kStep3Baseline = 340.0f;
constexpr int32_t kRowLabelX = 72, kRowValueX = 148;  // 76 px label column
constexpr float kRowBaseline[2] = {181.0f, 225.0f};
constexpr int32_t kQrX = 508, kQrY = 86, kQrSize = 268, kQrPadding = 14, kQrRadius = 14;
constexpr int32_t kQrCentreX = kQrX + kQrSize / 2;
constexpr float kCaptionBaseline = 378.0f;
constexpr float kFooterBaseline = 457.0f;

struct Objs {
    lv_obj_t* screen = nullptr;
    lv_obj_t* ssid = nullptr;
    lv_obj_t* pass = nullptr;
    lv_obj_t* step3a = nullptr;
    lv_obj_t* address = nullptr;
    lv_obj_t* footer = nullptr;
    QrBox qr;
};
Objs g;

void circle(int index) {
    lv_obj_t* c = box(g.screen, kLeft, kCircleTop[index], kCircle, kCircle, tok::bg(), LV_RADIUS_CIRCLE);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(c, tok::line(), LV_PART_MAIN);
    lv_obj_set_style_border_width(c, kCircleBorder, LV_PART_MAIN);
    static const char* kNumbers[3] = {"1", "2", "3"};
    lv_obj_t* n = lv_label_create(c);
    lv_obj_set_style_text_font(n, &nvp_txt_600_19, LV_PART_MAIN);
    lv_obj_set_style_text_color(n, tok::ink(), LV_PART_MAIN);
    lv_label_set_text_static(n, kNumbers[index]);
    lv_obj_center(n);
}

lv_color_t footer_colour(net::FooterColour c) {
    switch (c) {
        case net::FooterColour::Ink: return tok::ink();
        case net::FooterColour::Alert: return tok::alert();
        case net::FooterColour::Muted: break;
    }
    return tok::muted();
}

}  // namespace

lv_obj_t* setup_screen_create() {
    g = Objs{};
    g.screen = lv_obj_create(nullptr);
    lv_obj_remove_style_all(g.screen);
    lv_obj_set_style_bg_color(g.screen, tok::bg(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g.screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollable(g.screen, false);

    lv_label_set_text_static(text_label(g.screen, &nvp_txt_600_26, tok::ink(), kLeft, kTitleBaseline), tr::kSetupTitle);
    lv_label_set_text_static(text_label(g.screen, &nvp_txt_400_17, tok::muted(), kLeft, kSubtitleBaseline),
                             tr::kSetupSubtitle);
    for (int i = 0; i < 3; ++i) circle(i);

    lv_obj_t* s1 = paragraph(g.screen, &nvp_txt_400_17, tok::ink(), kStepX, kStep1Baseline, kStepWidth, kStepPitch);
    lv_label_set_text_static(s1, tr::kSetupStep1);
    const char* labels[2] = {tr::kSetupNetworkLabel, tr::kSetupPasswordLabel};
    for (int i = 0; i < 2; ++i) {
        lv_label_set_text_static(text_label(g.screen, &nvp_txt_400_15, tok::muted(), kRowLabelX, kRowBaseline[i]),
                                 labels[i]);
    }
    // The network name is text; the password is grouped digits in fixed cells, like the reference.
    g.ssid = text_label(g.screen, &nvp_cond_600_38, tok::now(), kRowValueX, kRowBaseline[0]);
    g.pass = mono_text_create(g.screen, &nvp_cond_600_38, 38);
    mono_text_set_color(g.pass, tok::now());
    mono_text_place(g.pass, kRowValueX, kRowBaseline[1]);
    lv_obj_t* s2 = paragraph(g.screen, &nvp_txt_400_17, tok::ink(), kStepX, kStep2Baseline, kStepWidth, kStepPitch);
    lv_label_set_text_static(s2, tr::kSetupStep2);

    // Step 3: text, the address in Barlow Cond. 600 `now`, then " yaz." wrapping like the reference.
    g.step3a = text_label(g.screen, &nvp_txt_400_17, tok::ink(), kStepX, kStep3Baseline);
    lv_label_set_text_static(g.step3a, tr::kSetupStep3Before);
    // lv_text_get_size() leaves out a trailing space, so add the space's advance by hand.
    const int32_t space = lv_font_get_glyph_width(&nvp_txt_400_17, ' ', 0);
    const int32_t address_x = kStepX + text_width(tr::kSetupStep3Before, &nvp_txt_400_17) + space;
    g.address = mono_text_create(g.screen, &nvp_num_21, 21);
    mono_text_set_color(g.address, tok::now());
    mono_text_set(g.address, tr::kSetupAddress);
    mono_text_place(g.address, address_x, kStep3Baseline);
    const int32_t after_x = address_x + mono_text_width(g.address);
    const char* after = tr::kSetupStep3After;
    if (after_x + text_width(after, &nvp_txt_400_17) > kStepX + kStepWidth) {
        lv_label_set_text_static(text_label(g.screen, &nvp_txt_400_17, tok::ink(), kStepX, kStep3Baseline + kStepPitch),
                                 after + 1);  // without the leading space on the new line
    } else {
        lv_label_set_text_static(text_label(g.screen, &nvp_txt_400_17, tok::ink(), after_x, kStep3Baseline), after);
    }

    qr_box_create(g.qr, g.screen, kQrX, kQrY, kQrSize, kQrPadding, kQrRadius);
    lv_obj_t* caption = centred_label(g.screen, &nvp_txt_400_15, tok::muted(), kQrCentreX, kCaptionBaseline);
    lv_label_set_text_static(caption, tr::kSetupQrCaption);
    centre_on(caption, kQrCentreX);

    g.footer = text_label(g.screen, &nvp_txt_400_17, tok::muted(), kLeft, kFooterBaseline);
    return g.screen;
}

void setup_screen_update(const net::SetupView& v) {
    if (g.screen == nullptr) return;
    set_text(g.ssid, v.ap_ssid);
    mono_text_set(g.pass, v.ap_pass_grouped);
    qr_box_set(g.qr, v.qr_payload);
    set_text(g.footer, net::footer_text(v));
    set_color(g.footer, footer_colour(net::footer_colour(v)));
}

}  // namespace nvp::ui
