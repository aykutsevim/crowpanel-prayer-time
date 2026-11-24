// Calendar: day numbers, weekdays, validity (research R1; ENG-07 month and year ends).
// Written before the code.
#include <unity.h>

#include "nvp/engine/calendar.h"

using namespace nvp::engine;

static void test_epoch_is_day_zero() {
    TEST_ASSERT_EQUAL_INT32(0, to_day_number(Date{1970, 1, 1}));
    TEST_ASSERT_EQUAL_INT32(-1, to_day_number(Date{1969, 12, 31}));
    TEST_ASSERT_EQUAL_INT32(1, to_day_number(Date{1970, 1, 2}));
}

static void test_round_trip_over_a_century() {
    const DayNumber first = to_day_number(Date{1999, 12, 25});
    const DayNumber last = to_day_number(Date{2101, 1, 5});
    Date prev = to_date(first - 1);
    for (DayNumber d = first; d <= last; ++d) {
        const Date date = to_date(d);
        TEST_ASSERT_TRUE(valid(date));
        TEST_ASSERT_EQUAL_INT32(d, to_day_number(date));
        // Consecutive day numbers are consecutive calendar days.
        const bool same_month = date.year == prev.year && date.month == prev.month && date.day == prev.day + 1;
        const bool new_month = date.day == 1 && (date.month == prev.month + 1 || (date.month == 1 && prev.month == 12));
        TEST_ASSERT_TRUE(same_month || new_month);
        prev = date;
    }
}

static void test_weekdays() {
    // 0 = Monday ... 6 = Sunday
    TEST_ASSERT_EQUAL_INT(3, weekday(to_day_number(Date{1970, 1, 1})));    // Thursday
    TEST_ASSERT_EQUAL_INT(2, weekday(to_day_number(Date{1969, 12, 31})));  // Wednesday
    TEST_ASSERT_EQUAL_INT(1, weekday(to_day_number(Date{2026, 11, 17})));  // Tuesday (Salı), design fixture
    TEST_ASSERT_EQUAL_INT(3, weekday(to_day_number(Date{2026, 12, 31})));  // Thursday
    TEST_ASSERT_EQUAL_INT(4, weekday(to_day_number(Date{2027, 1, 1})));    // Friday
    TEST_ASSERT_EQUAL_INT(3, weekday(to_day_number(Date{2024, 2, 29})));   // Thursday
}

static void test_validity() {
    TEST_ASSERT_TRUE(valid(Date{2024, 2, 29}));
    TEST_ASSERT_FALSE(valid(Date{2026, 2, 29}));
    TEST_ASSERT_TRUE(valid(Date{2000, 2, 29}));
    TEST_ASSERT_FALSE(valid(Date{2100, 2, 29}));
    TEST_ASSERT_FALSE(valid(Date{2026, 0, 1}));
    TEST_ASSERT_FALSE(valid(Date{2026, 13, 1}));
    TEST_ASSERT_FALSE(valid(Date{2026, 1, 0}));
    TEST_ASSERT_FALSE(valid(Date{2026, 1, 32}));
    TEST_ASSERT_FALSE(valid(Date{2026, 4, 31}));
    TEST_ASSERT_TRUE(valid(Date{2026, 12, 31}));
}

static void test_at_minutes() {
    const DayNumber d = to_day_number(Date{2026, 11, 17});
    TEST_ASSERT_TRUE(at(d, 754) == static_cast<LocalSeconds>(d) * 86400 + 754 * 60);
    TEST_ASSERT_TRUE(at(d + 1, 0) == at(d, 0) + kSecondsPerDay);
}

void run_calendar_tests() {
    RUN_TEST(test_epoch_is_day_zero);
    RUN_TEST(test_round_trip_over_a_century);
    RUN_TEST(test_weekdays);
    RUN_TEST(test_validity);
    RUN_TEST(test_at_minutes);
}
