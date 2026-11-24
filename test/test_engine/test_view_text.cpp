// US4: the hero's clock, dates and strings in MainView (FR-015 to FR-018, SC-004,
// 02-ui-spec.md §4.1 M5..M8, §9). Written before the code.
#include <unity.h>

#include "helpers.h"

using namespace engine_test;

static void test_clock_and_dates_at_143207() {
    const MainView v = eval(fixture_at(fixture::kCheckSecond));
    TEST_ASSERT_EQUAL_STRING("14:32", v.clock_hhmm);
    TEST_ASSERT_EQUAL_STRING(":07", v.clock_ss);
    TEST_ASSERT_EQUAL_STRING("17 Kasım 2026 Salı", v.date_text);
    TEST_ASSERT_EQUAL_STRING("7 Cemaziyelahir 1448", v.hijri_text);
}

static void test_hijri_absent_is_empty() {
    DayRecord today = fixture::kToday;
    today.hijri.present = false;
    Input in = fixture_at(fixture::kCheckSecond);
    in.today = &today;
    TEST_ASSERT_EQUAL_STRING("", eval(in).hijri_text);
}

static void test_year_end_dates() {
    DayRecord last = fixture::kToday;
    last.date = Date{2026, 12, 31};
    DayRecord first = fixture::kToday;
    first.date = Date{2027, 1, 1};
    Input a;
    a.now = Moment{last.date, hms(12, 0, 0)};
    a.today = &last;
    Input b;
    b.now = Moment{first.date, hms(12, 0, 0)};
    b.today = &first;
    TEST_ASSERT_EQUAL_STRING("31 Aralık 2026 Perşembe", eval(a).date_text);
    TEST_ASSERT_EQUAL_STRING("1 Ocak 2027 Cuma", eval(b).date_text);
}

static void test_long_countdown() {
    const MainView v = eval(fixture_at(hms(19, 14, 0)));
    TEST_ASSERT_EQUAL_STRING("11:03:00", v.countdown_text);
    TEST_ASSERT_EQUAL_STRING(":00", v.clock_ss);
    TEST_ASSERT_EQUAL_STRING("19:14", v.clock_hhmm);
}

static void test_midnight_clock() {
    const MainView v = eval(fixture_at(0));
    TEST_ASSERT_EQUAL_STRING("00:00", v.clock_hhmm);
    TEST_ASSERT_EQUAL_STRING(":00", v.clock_ss);
}

void run_view_text_tests() {
    RUN_TEST(test_clock_and_dates_at_143207);
    RUN_TEST(test_hijri_absent_is_empty);
    RUN_TEST(test_year_end_dates);
    RUN_TEST(test_long_countdown);
    RUN_TEST(test_midnight_clock);
}
