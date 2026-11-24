// US3: midnight, missing and invalid days, "no data" (ENG-04, ENG-07, ENG-08, FR-003, FR-008,
// FR-009, TIME-06). Written before the code.
#include <unity.h>

#include <initializer_list>

#include "helpers.h"

using namespace engine_test;

static DayRecord record(Date d, std::initializer_list<int> minutes) {
    DayRecord r;
    r.date = d;
    int k = 0;
    for (int m : minutes) r.minutes[k++] = static_cast<uint16_t>(m);
    r.source = SourceId::Imsakiyem;
    return r;
}

static void test_midnight_continuity() {  // US3 scenario 1, TIME-06
    const MainView before = eval(fixture_at(hms(23, 59, 59)));
    const DayRecord next_day = record(Date{2026, 11, 19}, {378, 468, 774, 925, 1069, 1152});
    Input in;
    in.now = Moment{Date{2026, 11, 18}, 0};
    in.yesterday = &fixture::kToday;
    in.today = &fixture::kTomorrow;
    in.tomorrow = &next_day;
    const MainView after = eval(in);
    TEST_ASSERT_EQUAL_UINT8(kYatsi, before.period);
    TEST_ASSERT_EQUAL_UINT8(kYatsi, after.period);
    TEST_ASSERT_TRUE(before.start == after.start);
    TEST_ASSERT_TRUE(before.end == after.end);
    TEST_ASSERT_EQUAL_INT32(before.remaining_s - 1, after.remaining_s);
    for (uint8_t k = 0; k < kVakitCount; ++k) {
        TEST_ASSERT_EQUAL_UINT16(fixture::kTomorrow.minutes[k], after.columns[k].minutes);
    }
}

static void test_missing_yesterday() {  // US3 scenario 2, ENG-04
    Input in = fixture_at(hms(3, 30, 0));
    in.yesterday = nullptr;
    const MainView v = eval(in);
    TEST_ASSERT_TRUE(v.approx_bounds);
    TEST_ASSERT_TRUE(v.start == local(Date{2026, 11, 16}, 19, 14));
    TEST_ASSERT_TRUE(v.end == local(fixture::kDate, 6, 16));
}

static void test_missing_tomorrow() {  // US3 scenario 3, ENG-04
    Input in = fixture_at(hms(21, 30, 0));
    in.tomorrow = nullptr;
    const MainView v = eval(in);
    TEST_ASSERT_TRUE(v.approx_bounds);
    TEST_ASSERT_TRUE(v.end == local(Date{2026, 11, 18}, 6, 16));
    TEST_ASSERT_EQUAL_STRING("06:16", v.columns[kImsak].time_text);
    TEST_ASSERT_EQUAL_STRING("yarın", v.columns[kImsak].note);
    // During the day neither neighbour is used, so nothing is approximate.
    Input day = fixture_at(fixture::kCheckSecond);
    day.tomorrow = nullptr;
    day.yesterday = nullptr;
    TEST_ASSERT_FALSE(eval(day).approx_bounds);
}

static void expect_no_data(const MainView& v) {
    TEST_ASSERT_FALSE(v.has_data);
    TEST_ASSERT_NOT_NULL(v.period_name);
    TEST_ASSERT_EQUAL_STRING("", v.period_name);
    TEST_ASSERT_EQUAL_STRING("", v.countdown_label);
    TEST_ASSERT_EQUAL_STRING("", v.countdown_text);
    for (uint8_t k = 0; k < kVakitCount; ++k) {
        TEST_ASSERT_EQUAL_STRING("", v.columns[k].time_text);
        TEST_ASSERT_EQUAL_STRING("", v.columns[k].note);
    }
    for (const KerahatInterval& i : v.kerahat) TEST_ASSERT_FALSE(i.shown);
}

static void test_missing_today_is_no_data() {  // US3 scenario 4, ENG-08
    Input in = fixture_at(fixture::kCheckSecond);
    in.today = nullptr;
    const MainView v = eval(in);
    expect_no_data(v);
    TEST_ASSERT_EQUAL_STRING("14:32", v.clock_hhmm);
    TEST_ASSERT_EQUAL_STRING(":07", v.clock_ss);
    TEST_ASSERT_EQUAL_STRING("17 Kasım 2026 Salı", v.date_text);
}

static void test_invalid_or_misdated_records() {  // US3 scenario 5, FR-002, FR-003
    DayRecord bad = fixture::kToday;
    bad.minutes[kOgle] = bad.minutes[kGunes];
    Input in = fixture_at(fixture::kCheckSecond);
    in.today = &bad;
    expect_no_data(eval(in));

    DayRecord other_day = fixture::kToday;
    other_day.date = Date{2026, 11, 20};
    in.today = &other_day;
    expect_no_data(eval(in));

    DayRecord old = fixture::kYesterday;
    old.date = Date{2026, 11, 15};
    Input night = fixture_at(hms(3, 30, 0));
    night.yesterday = &old;
    const MainView v = eval(night);
    TEST_ASSERT_TRUE(v.approx_bounds);
    TEST_ASSERT_TRUE(v.start == local(Date{2026, 11, 16}, 19, 14));  // today's Yatsı substituted
}

static void test_identical_records() {  // US3 scenario 6, ENG-07
    DayRecord y = fixture::kToday;
    y.date = Date{2026, 11, 16};
    DayRecord t = fixture::kToday;
    t.date = Date{2026, 11, 18};
    for (int32_t s : {hms(3, 30, 0), fixture::kCheckSecond, hms(21, 30, 0)}) {
        Input in = fixture_at(s);
        in.yesterday = &y;
        in.tomorrow = &t;
        const MainView v = eval(in);
        TEST_ASSERT_TRUE(v.has_data);
        TEST_ASSERT_FALSE(v.approx_bounds);
    }
}

static void test_plus_minus_two_minutes_at_midnight() {  // US3 scenario 7, ENG-07
    for (int delta : {-2, 2}) {
        DayRecord tomorrow = fixture::kTomorrow;
        tomorrow.minutes[kImsak] = static_cast<uint16_t>(fixture::kToday.minutes[kImsak] + delta);
        Input before = fixture_at(hms(23, 59, 59));
        before.tomorrow = &tomorrow;
        const DayRecord next_day = record(Date{2026, 11, 19}, {380, 470, 774, 925, 1069, 1152});
        Input after;
        after.now = Moment{Date{2026, 11, 18}, 0};
        after.yesterday = &fixture::kToday;
        after.today = &tomorrow;
        after.tomorrow = &next_day;
        const MainView b = eval(before);
        const MainView a = eval(after);
        TEST_ASSERT_TRUE(b.start == a.start);
        TEST_ASSERT_TRUE(b.end == a.end);
        TEST_ASSERT_EQUAL_INT32(b.remaining_s - 1, a.remaining_s);
    }
}

static void test_month_and_year_end() {  // ENG-07
    struct Case {
        Date last;
        Date first;
        Date after;
    };
    const Case cases[] = {{Date{2026, 11, 30}, Date{2026, 12, 1}, Date{2026, 12, 2}},
                          {Date{2026, 12, 31}, Date{2027, 1, 1}, Date{2027, 1, 2}},
                          {Date{2028, 2, 29}, Date{2028, 3, 1}, Date{2028, 3, 2}}};
    for (const Case& c : cases) {
        const DayRecord d0 = record(c.last, {380, 470, 774, 925, 1069, 1152});
        const DayRecord d1 = record(c.first, {381, 471, 774, 925, 1069, 1151});
        const DayRecord d2 = record(c.after, {382, 472, 775, 925, 1069, 1151});
        DayRecord dm = d0;
        dm.date = to_date(to_day_number(c.last) - 1);
        Input before;
        before.now = Moment{c.last, hms(23, 59, 59)};
        before.yesterday = &dm;
        before.today = &d0;
        before.tomorrow = &d1;
        Input after;
        after.now = Moment{c.first, 0};
        after.yesterday = &d0;
        after.today = &d1;
        after.tomorrow = &d2;
        const MainView b = eval(before);
        const MainView a = eval(after);
        TEST_ASSERT_TRUE(b.has_data && a.has_data);
        TEST_ASSERT_FALSE(b.approx_bounds || a.approx_bounds);
        TEST_ASSERT_TRUE(b.start == a.start);
        TEST_ASSERT_TRUE(b.end == a.end);
        TEST_ASSERT_EQUAL_INT32(b.remaining_s - 1, a.remaining_s);
    }
}

static void test_out_of_range_moment() {
    expect_no_data(eval(fixture_at(86400)));
    expect_no_data(eval(fixture_at(-1)));
}

void run_day_tests() {
    RUN_TEST(test_midnight_continuity);
    RUN_TEST(test_missing_yesterday);
    RUN_TEST(test_missing_tomorrow);
    RUN_TEST(test_missing_today_is_no_data);
    RUN_TEST(test_invalid_or_misdated_records);
    RUN_TEST(test_identical_records);
    RUN_TEST(test_plus_minus_two_minutes_at_midnight);
    RUN_TEST(test_month_and_year_end);
    RUN_TEST(test_out_of_range_moment);
}
