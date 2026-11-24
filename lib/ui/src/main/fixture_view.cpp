#include "main/fixture_view.h"

#include <cstdio>

#include "nvp/engine/fixture.h"
#include "nvp/screen/fixtures.h"
#include "nvp/text/format.h"

namespace nvp::ui {

bool fixture_view_at(int number, int32_t second_of_day, engine::MainView& v, SystemView& s) {
    const screen::Fixture* f = screen::fixture(number);
    if (f == nullptr) return false;
    s = SystemView{};
    s.state = f->state;
    std::snprintf(s.city, sizeof s.city, "%s", screen::kFixtureCity);
    std::snprintf(s.last_update, sizeof s.last_update, "%s", screen::kFixtureLastUpdate);
    s.days_left = f->days_left;
    s.kerahat_enabled = true;
    // no_data keeps the Hijri date of the newest stored record (reference 14): the sample day.
    text::format_hijri(engine::fixture::kToday.hijri, s.hijri_fallback, sizeof s.hijri_fallback);

    engine::Settings settings;
    if (f->state == screen::SystemState::NoTime || second_of_day < 0) {
        engine::evaluate_no_time(&engine::fixture::kToday, settings, v);
        return true;
    }
    engine::Input in;
    in.now = engine::Moment{engine::fixture::kDate, second_of_day};
    in.yesterday = &engine::fixture::kYesterday;
    in.today = f->state == screen::SystemState::NoData ? nullptr : &engine::fixture::kToday;
    in.tomorrow = &engine::fixture::kTomorrow;
    in.settings = settings;
    engine::evaluate(in, v);
    return true;
}

bool fixture_view(int number, engine::MainView& v, SystemView& s) {
    const screen::Fixture* f = screen::fixture(number);
    if (f == nullptr) return false;
    return fixture_view_at(number, f->second_of_day, v, s);
}

}  // namespace nvp::ui
