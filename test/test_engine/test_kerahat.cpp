// US5: kerahat intervals (ENG-06, FR-014, 02-ui-spec.md §4.2). Written before the code.
#include <unity.h>

#include "helpers.h"

using namespace engine_test;

static void expect_interval(const KerahatInterval& i, int32_t start, int32_t end) {
    TEST_ASSERT_TRUE(i.shown);
    TEST_ASSERT_EQUAL_INT32(start, i.start_s);
    TEST_ASSERT_EQUAL_INT32(end, i.end_s);
}

static void test_default_lengths() {  // US5 scenario 1
    const MainView v = eval(fixture_at(fixture::kCheckSecond));
    expect_interval(v.kerahat[0], hms(7, 46, 0), hms(8, 31, 0));
    expect_interval(v.kerahat[1], hms(12, 9, 0), hms(12, 54, 0));
    expect_interval(v.kerahat[2], hms(17, 6, 0), hms(17, 51, 0));
}

static void test_zero_length_omits_one() {  // US5 scenario 2
    Settings s;
    s.kerahat_noon_min = 0;
    const MainView v = eval(fixture_at(fixture::kCheckSecond, s));
    expect_interval(v.kerahat[0], hms(7, 46, 0), hms(8, 31, 0));
    TEST_ASSERT_FALSE(v.kerahat[1].shown);
    expect_interval(v.kerahat[2], hms(17, 6, 0), hms(17, 51, 0));
}

static void test_layer_off() {  // US5 scenario 3
    Settings s;
    s.kerahat_enabled = false;
    const MainView v = eval(fixture_at(fixture::kCheckSecond, s));
    for (const KerahatInterval& i : v.kerahat) TEST_ASSERT_FALSE(i.shown);
}

static void test_clipped_to_the_day() {  // US5 scenario 4
    DayRecord edge = fixture::kToday;
    edge.minutes[kImsak] = 5;
    edge.minutes[kGunes] = 30;
    edge.minutes[kOgle] = 60;   // 01:00, noon interval of 90 min would start at 23:30 the day before
    edge.minutes[kIkindi] = 1380;
    edge.minutes[kAksam] = 1430;
    edge.minutes[kYatsi] = 1439;
    Settings s;
    s.kerahat_noon_min = 90;
    s.kerahat_sunrise_min = 90;
    Input in = fixture_at(hms(1, 0, 0), s);
    in.today = &edge;
    const MainView v = eval(in);
    expect_interval(v.kerahat[0], hms(0, 30, 0), hms(2, 0, 0));
    expect_interval(v.kerahat[1], 0, hms(1, 0, 0));
    expect_interval(v.kerahat[2], hms(23, 5, 0), hms(23, 50, 0));
    TEST_ASSERT_TRUE(v.kerahat[2].end_s <= kSecondsPerDay);
}

static void test_lengths_limited_to_90() {
    Settings s;
    s.kerahat_sunrise_min = 200;
    const MainView v = eval(fixture_at(fixture::kCheckSecond, s));
    expect_interval(v.kerahat[0], hms(7, 46, 0), hms(9, 16, 0));
}

static void test_same_at_any_moment() {
    const MainView a = eval(fixture_at(hms(3, 30, 0)));
    const MainView b = eval(fixture_at(hms(21, 30, 0)));
    for (int i = 0; i < 3; ++i) {
        TEST_ASSERT_EQUAL_INT32(a.kerahat[i].start_s, b.kerahat[i].start_s);
        TEST_ASSERT_EQUAL_INT32(a.kerahat[i].end_s, b.kerahat[i].end_s);
    }
}

void run_kerahat_tests() {
    RUN_TEST(test_default_lengths);
    RUN_TEST(test_zero_length_omits_one);
    RUN_TEST(test_layer_off);
    RUN_TEST(test_clipped_to_the_day);
    RUN_TEST(test_lengths_limited_to_90);
    RUN_TEST(test_same_at_any_moment);
}
