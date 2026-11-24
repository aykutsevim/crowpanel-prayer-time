// Redraw accounting for the main screen (spec 003 FR-015, FR-016, UI-03, research R8).
//
// For each step from one moment to the next, the screen is first drawn fully at the earlier
// moment, then updated to the later one. The harness sums the areas LVGL invalidates during the
// update, then checks that the updated picture equals a fresh full redraw of the later moment
// (no stale or leftover pixels). A one-second step must invalidate less than 8 % of the screen.
#include <cstdio>
#include <cstring>

#include "lvgl.h"
#include "main/main_screen.h"
#include "main/main_view.h"
#include "nvp/engine/engine.h"
#include "nvp/engine/fixture.h"
#include "nvp/screen/fixtures.h"

namespace {

using namespace nvp;

int64_t g_invalidated = 0;

void on_invalidate(lv_event_t* e) {
    const lv_area_t* a = static_cast<const lv_area_t*>(lv_event_get_param(e));
    if (a != nullptr) g_invalidated += lv_area_get_size(a);
}

// The day after the fixture's tomorrow, for steps across midnight.
const engine::DayRecord kDayAfter{engine::Date{2026, 11, 19},
                                  {378, 468, 774, 925, 1069, 1152},
                                  engine::HijriDate{9, 6, 1448, true},
                                  engine::SourceId::Imsakiyem};

struct Moment {
    int day;  // 0 = 2026-11-17, 1 = 2026-11-18
    int32_t second;
};

void view_at(Moment m, engine::MainView& v, ui::SystemView& s) {
    s = ui::SystemView{};
    s.state = screen::SystemState::Ok;
    std::snprintf(s.city, sizeof s.city, "%s", screen::kFixtureCity);
    std::snprintf(s.last_update, sizeof s.last_update, "%s", screen::kFixtureLastUpdate);
    engine::Input in;
    if (m.day == 0) {
        in.now = engine::Moment{engine::fixture::kDate, m.second};
        in.yesterday = &engine::fixture::kYesterday;
        in.today = &engine::fixture::kToday;
        in.tomorrow = &engine::fixture::kTomorrow;
    } else {
        in.now = engine::Moment{engine::fixture::kTomorrow.date, m.second};
        in.yesterday = &engine::fixture::kToday;
        in.today = &engine::fixture::kTomorrow;
        in.tomorrow = &kDayAfter;
    }
    engine::evaluate(in, v);
}

lv_obj_t* fresh_screen(Moment m) {
    lv_obj_t* old = lv_screen_active();
    lv_obj_t* scr = ui::main_screen_create();
    lv_screen_load(scr);
    if (old != nullptr && old != scr) lv_obj_delete(old);
    engine::MainView v;
    ui::SystemView s;
    view_at(m, v, s);
    ui::main_screen_update(v, s);
    lv_refr_now(nullptr);
    return scr;
}

struct Step {
    const char* name;
    Moment from;
    Moment to;
    double max_share;  // of 800 x 480; 1.0 = no bound
};

}  // namespace

int run_redraw_checks() {
    lv_display_t* disp = lv_display_get_default();
    lv_display_add_event_cb(disp, on_invalidate, LV_EVENT_INVALIDATE_AREA, nullptr);
    const Step steps[] = {
        {"second", {0, 52327}, {0, 52328}, 0.08},
        {"minute", {0, 52379}, {0, 52380}, 1.0},
        {"vakit", {0, 55619}, {0, 55620}, 1.0},       // 15:26:59 -> 15:27:00 (İkindi)
        {"midnight", {0, 86399}, {1, 0}, 1.0},
        {"jump_back", {0, 54000}, {0, 50400}, 1.0},   // 15:00 -> 14:00
        {"jump_fwd", {0, 10800}, {0, 72000}, 1.0},    // 03:00 -> 20:00
    };
    int failures = 0;
    for (const Step& st : steps) {
        fresh_screen(st.from);
        g_invalidated = 0;
        engine::MainView v;
        ui::SystemView s;
        view_at(st.to, v, s);
        ui::main_screen_update(v, s);
        const int64_t area = g_invalidated;
        lv_refr_now(nullptr);

        char path[96];
        std::snprintf(path, sizeof path, "out/snapshots/redraw_%s.png", st.name);
        std::remove(path);
        lv_test_screenshot_compare(path);  // writes the incrementally updated picture
        fresh_screen(st.to);
        const bool same = lv_test_screenshot_compare(path) == LV_TEST_SCREENSHOT_RESULT_PASSED;

        const double share = static_cast<double>(area) / (800.0 * 480.0);
        const bool within = share < st.max_share;
        std::printf("redraw: %-10s invalidated %6lld px (%5.1f %%)%s, matches a full redraw: %s\n", st.name,
                    static_cast<long long>(area), 100.0 * share, st.max_share < 1.0 ? (within ? " within bound" : " OVER BOUND") : "",
                    same ? "yes" : "NO");
        if (!same || !within) ++failures;
        std::remove(path);
    }
    lv_display_remove_event_cb_with_user_data(disp, on_invalidate, nullptr);
    return failures;
}
