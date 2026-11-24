#include "setup/connecting_screen.h"

#include <cstring>

#include "fonts/fonts.h"
#include "main/tokens.h"
#include "setup/widgets.h"

namespace nvp::ui {
namespace {

using namespace setup;

// Three centred lines from y 150, 14 px apart (02 §7.3); baselines measured on reference 16.
constexpr int32_t kCentreX = 400;
constexpr float kTitleBaseline = 176.0f, kSsidBaseline = 229.0f, kNoteBaseline = 269.0f;

struct Objs {
    lv_obj_t* screen = nullptr;
    lv_obj_t* title = nullptr;
    lv_obj_t* ssid = nullptr;
    lv_obj_t* note = nullptr;
};
Objs g;

void update_centred(lv_obj_t* l, const char* text) {
    const char* cur = lv_label_get_text(l);
    if (cur != nullptr && std::strcmp(cur, text) == 0) return;
    lv_label_set_text(l, text);
    centre_on(l, kCentreX);
}

}  // namespace

lv_obj_t* connecting_screen_create() {
    g = Objs{};
    g.screen = lv_obj_create(nullptr);
    lv_obj_remove_style_all(g.screen);
    lv_obj_set_style_bg_color(g.screen, tok::bg(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g.screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollable(g.screen, false);
    g.title = centred_label(g.screen, &nvp_txt_600_26, tok::ink(), kCentreX, kTitleBaseline);
    g.ssid = centred_label(g.screen, &nvp_cond_600_38, tok::now(), kCentreX, kSsidBaseline);
    g.note = centred_label(g.screen, &nvp_txt_400_17, tok::muted(), kCentreX, kNoteBaseline);
    return g.screen;
}

void connecting_screen_update(const net::ConnectingView& v) {
    if (g.screen == nullptr) return;
    update_centred(g.title, net::connecting_title(v));
    update_centred(g.ssid, v.ssid);
    update_centred(g.note, net::connecting_note(v));
}

}  // namespace nvp::ui
