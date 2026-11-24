#include "main/columns.h"

#include <cstring>

#include "fonts/fonts.h"
#include "main/mono_text.h"
#include "main/tokens.h"
#include "nvp/text/tr.h"

namespace nvp::ui {
namespace {

using namespace tok;
using engine::ColumnState;

struct ColumnObjs {
    lv_obj_t* box;
    lv_obj_t* name;
    lv_obj_t* time;     // 48 px
    lv_obj_t* time44;   // 44 px, calculated state
    lv_obj_t* approx;   // ≈ prefix
    lv_obj_t* note;
};
ColumnObjs g_cols[6];

lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, int k, float baseline) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, LV_PART_MAIN);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(l, round_px(kColumnW));
    lv_obj_set_pos(l, round_px(column_x(k)), top_for_baseline(font, baseline));
    lv_label_set_text_static(l, "");
    return l;
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

}  // namespace

void columns_create(lv_obj_t* parent) {
    for (int k = 0; k < 6; ++k) {
        ColumnObjs& c = g_cols[k];
        c.box = lv_obj_create(parent);
        lv_obj_remove_style_all(c.box);
        lv_obj_set_scrollable(c.box, false);
        lv_obj_set_clickable(c.box, false);
        // Box: x = 24 + k × 125.33, y 336, 125.33 × 128 (rounded to whole pixels).
        const int32_t x0 = round_px(column_x(k)), x1 = round_px(column_x(k + 1));
        lv_obj_set_pos(c.box, x0, kColumnTop);
        lv_obj_set_size(c.box, x1 - x0, kColumnH);
        lv_obj_set_style_radius(c.box, kColumnRadius, LV_PART_MAIN);
        lv_obj_set_style_bg_color(c.box, now(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(c.box, LV_OPA_TRANSP, LV_PART_MAIN);
        c.name = label(parent, &nvp_txt_500_19, k, kColumnNameBaseline);
        c.note = label(parent, &nvp_txt_400_15, k, kColumnNoteBaseline);
        c.time = mono_text_create(parent, &nvp_num_48, 48);
        c.time44 = mono_text_create(parent, &nvp_num_44, 44);
        c.approx = lv_label_create(parent);
        lv_obj_set_style_text_font(c.approx, &nvp_txt_500_21, LV_PART_MAIN);
        lv_label_set_text_static(c.approx, nvp::text::tr::kApproxPrefix);
        lv_obj_set_hidden(c.time44, true);
        lv_obj_set_hidden(c.approx, true);
    }
}

void columns_set(const engine::MainView& v, ColumnsMode mode) {
    for (int k = 0; k < 6; ++k) {
        ColumnObjs& c = g_cols[k];
        const engine::Column& col = v.columns[k];
        const bool current = mode != ColumnsMode::Placeholder && mode != ColumnsMode::StoredOnly &&
                             col.state == ColumnState::Current;
        const lv_opa_t opa = current ? LV_OPA_COVER : LV_OPA_TRANSP;
        if (lv_obj_get_style_bg_opa(c.box, LV_PART_MAIN) != opa) lv_obj_set_style_bg_opa(c.box, opa, LV_PART_MAIN);

        // Colours per 02 §4.5 (and §6 for the placeholder states).
        lv_color_t text = ink();
        lv_color_t note = muted();
        if (mode == ColumnsMode::Placeholder) {
            text = dim();
        } else if (mode == ColumnsMode::StoredOnly) {
            text = ink();
        } else if (current) {
            text = bg();
            note = bg();
        } else if (col.state == ColumnState::Past) {
            text = dim();
        } else if (col.state == ColumnState::Next) {
            note = now();
        }

        set_text(c.name, nvp::text::tr::kVakitNames[k]);
        set_color(c.name, text);

        const char* time = mode == ColumnsMode::Placeholder ? "--:--" : col.time_text;
        const bool calc = mode == ColumnsMode::Calculated;
        lv_obj_set_hidden(c.time, calc);
        lv_obj_set_hidden(c.time44, !calc);
        lv_obj_set_hidden(c.approx, !calc);
        if (calc) {
            // ≈ (Barlow 500 21, raised 8 px) + 3 px gap + 44 px time, centred together on Cx(k).
            mono_text_set(c.time44, time);
            mono_text_set_color(c.time44, text);
            const int32_t aw = lv_font_get_glyph_width(&nvp_txt_500_21, 0x2248, 0);
            const int32_t tw = mono_text_width(c.time44);
            const int32_t left = round_px(column_cx(k) - (aw + 3 + tw) / 2.0f);
            const int32_t ay = top_for_baseline(&nvp_txt_500_21, kColumnTimeBaseline - 8.0f);
            if (lv_obj_get_style_x(c.approx, LV_PART_MAIN) != left || lv_obj_get_style_y(c.approx, LV_PART_MAIN) != ay) {
                lv_obj_set_pos(c.approx, left, ay);
            }
            set_color(c.approx, text);
            mono_text_place(c.time44, left + aw + 3, kColumnTimeBaseline);
        } else {
            mono_text_set(c.time, time);
            mono_text_set_color(c.time, text);
            mono_text_place(c.time, round_px(column_cx(k) - mono_text_width(c.time) / 2.0f), kColumnTimeBaseline);
        }

        const bool notes = mode == ColumnsMode::Normal || mode == ColumnsMode::Calculated;
        set_text(c.note, notes ? col.note : "");
        set_color(c.note, note);
    }
}

}  // namespace nvp::ui
