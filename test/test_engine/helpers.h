// Helpers shared by the engine tests (spec 002).
#pragma once

#include "nvp/engine/engine.h"
#include "nvp/engine/fixture.h"

namespace engine_test {

using namespace nvp::engine;

inline int32_t hms(int h, int m, int s) { return h * 3600 + m * 60 + s; }

// The fixture with all three days present, at `second` of 2026-11-17.
inline Input fixture_at(int32_t second, Settings settings = Settings{}) {
    Input in;
    in.now = Moment{fixture::kDate, second};
    in.yesterday = &fixture::kYesterday;
    in.today = &fixture::kToday;
    in.tomorrow = &fixture::kTomorrow;
    in.settings = settings;
    return in;
}

inline MainView eval(const Input& in) {
    MainView v;
    evaluate(in, v);
    return v;
}

inline LocalSeconds local(const Date& d, int h, int m, int s = 0) {
    return static_cast<LocalSeconds>(to_day_number(d)) * kSecondsPerDay + hms(h, m, s);
}

inline const DayRecord* fixture_record(int index) {  // 0 yesterday, 1 today, 2 tomorrow
    return index == 0 ? &fixture::kYesterday : index == 1 ? &fixture::kToday : &fixture::kTomorrow;
}

}  // namespace engine_test
