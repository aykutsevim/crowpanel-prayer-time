// Edge cases of the main screen (spec 003 convergence tasks T045, T046):
//  - a city wider than 300 px is cut with "…" (U+2026) and stays within 300 px (FR-011);
//  - the now marker at 00:00:00 and 23:59:59 is drawn and lies fully on the canvas.
#include <cstdio>
#include <cstring>
#include <initializer_list>

#include "lvgl.h"
#include "main/fixture_view.h"
#include "main/main_screen.h"

namespace {

lv_obj_t* fresh() {
    lv_obj_t* old = lv_screen_active();
    lv_obj_t* scr = nvp::ui::main_screen_create();
    lv_screen_load(scr);
    if (old != nullptr && old != scr) lv_obj_delete(old);
    return scr;
}

}  // namespace

int run_edge_checks() {
    int failures = 0;

    // T045: long city.
    fresh();
    nvp::engine::MainView v;
    nvp::ui::SystemView s;
    nvp::ui::fixture_view(1, v, s);
    std::snprintf(s.city, sizeof s.city, "%s", "Kahramanmaraş Onikişubat Büyükşehir Merkezi");
    nvp::ui::main_screen_update(v, s);
    lv_refr_now(nullptr);
    lv_obj_t* city = nvp::ui::main_screen_city_label();
    const char* text = lv_label_get_text(city);
    const size_t n = std::strlen(text);
    const bool ellipsis = n >= 3 && std::strcmp(text + n - 3, "\xE2\x80\xA6") == 0;
    lv_obj_update_layout(city);
    const int32_t w = lv_obj_get_width(city);
    const bool ok_city = ellipsis && w <= 300;
    std::printf("edge: long city \"%s\" width %ld px: %s\n", text, static_cast<long>(w), ok_city ? "ok" : "FAILED");
    failures += ok_city ? 0 : 1;

    // T046: marker at both ends of the day.
    for (int32_t second : {0, 86399}) {
        fresh();
        nvp::ui::fixture_view_at(1, second, v, s);
        nvp::ui::main_screen_update(v, s);
        lv_refr_now(nullptr);
        lv_obj_t* m = nvp::ui::main_screen_marker();
        lv_area_t a;
        lv_obj_get_coords(m, &a);
        const bool ok = !lv_obj_is_hidden(m) && a.x1 >= 0 && a.x2 <= 799;
        std::printf("edge: marker at %05ld s spans x %ld..%ld: %s\n", static_cast<long>(second), static_cast<long>(a.x1),
                    static_cast<long>(a.x2), ok ? "ok" : "FAILED");
        char path[64];
        std::snprintf(path, sizeof path, "out/snapshots/edge_marker_%05ld.png", static_cast<long>(second));
        std::remove(path);
        lv_test_screenshot_compare(path);
        failures += ok ? 0 : 1;
    }
    return failures;
}
