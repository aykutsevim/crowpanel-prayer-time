#include "diag/boot_screen.h"

#include <cstdio>

#include "diag/palette.h"

namespace nvp::ui::diag {

lv_obj_t* boot_screen_create(const char* firmware, const char* model, const char* revision) {
    lv_obj_t* scr = lv_obj_create(nullptr);
    lv_obj_set_scrollable(scr, false);
    lv_obj_set_style_bg_color(scr, palette::black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    char board[48];
    std::snprintf(board, sizeof board, "%s %s", model, revision);

    struct Line {
        const char* text;
        const lv_font_t* font;
        lv_color_t color;
        int32_t y;
    };
    // The built-in font has no U+2026, so the contract's "starting…" is written with three dots.
    const Line lines[] = {
        {"Namaz Vakti Panel", &lv_font_montserrat_20, palette::white(), -45},
        {firmware, &lv_font_montserrat_14, palette::white(), -10},
        {board, &lv_font_montserrat_14, palette::white(), 15},
        {"starting...", &lv_font_montserrat_14, palette::muted(), 50},
    };
    for (const Line& l : lines) {
        lv_obj_t* label = lv_label_create(scr);
        lv_label_set_text(label, l.text);
        lv_obj_set_style_text_font(label, l.font, 0);
        lv_obj_set_style_text_color(label, l.color, 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, l.y);
    }
    return scr;
}

}  // namespace nvp::ui::diag
