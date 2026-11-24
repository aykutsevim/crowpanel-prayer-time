// US1: running period, bounds, countdown, fraction, soon, entered (ENG-02, ENG-03, ENG-05,
// FR-004 to FR-007, 02 §5). Expected values: 03-functional-spec.md §3 and
// specs/002-prayer-engine/contracts/engine-table.md. Written before the code.
#include <unity.h>

#include "helpers.h"
#include "nvp/text/tr.h"

using namespace engine_test;
namespace tr = nvp::text::tr;

static void test_fixture_at_143207() {  // US1 scenario 1, SC-002
    const MainView v = eval(fixture_at(fixture::kCheckSecond));
    TEST_ASSERT_TRUE(v.has_data);
    TEST_ASSERT_FALSE(v.approx_bounds);
    TEST_ASSERT_EQUAL_UINT8(kOgle, v.period);
    TEST_ASSERT_TRUE(v.start == local(fixture::kDate, 12, 54));
    TEST_ASSERT_TRUE(v.end == local(fixture::kDate, 15, 27));
    TEST_ASSERT_EQUAL_UINT8(kIkindi, v.next);
    TEST_ASSERT_FALSE(v.next_is_tomorrow);
    TEST_ASSERT_EQUAL_INT32(fixture::kExpectedRemaining, v.remaining_s);
    TEST_ASSERT_EQUAL_INT32(fixture::kExpectedElapsed, v.elapsed_s);
    TEST_ASSERT_FLOAT_WITHIN(0.0005f, 0.6413f, v.fraction);
    TEST_ASSERT_EQUAL_STRING("Öğle vakti", v.period_name);
    TEST_ASSERT_EQUAL_STRING("İkindiye kalan süre", v.countdown_label);
    TEST_ASSERT_EQUAL_STRING("12:54", v.start_text);
    TEST_ASSERT_EQUAL_STRING("15:27", v.end_text);
    TEST_ASSERT_EQUAL_STRING("00:54:53", v.countdown_text);
    TEST_ASSERT_FALSE(v.soon);
    TEST_ASSERT_FALSE(v.entered);
    TEST_ASSERT_EQUAL_INT32(fixture::kCheckSecond, v.now_second_of_day);
}

static void test_one_second_before_and_at_each_vakit() {  // US1 scenarios 2 and 3, ENG-02, ENG-07
    for (uint8_t k = 0; k < kVakitCount; ++k) {
        const int32_t at_s = fixture::kToday.minutes[k] * 60;
        const MainView before = eval(fixture_at(at_s - 1));
        const MainView at = eval(fixture_at(at_s));
        const uint8_t prev = k == 0 ? kYatsi : static_cast<uint8_t>(k - 1);
        TEST_ASSERT_EQUAL_UINT8(prev, before.period);
        TEST_ASSERT_EQUAL_INT32(1, before.remaining_s);
        TEST_ASSERT_EQUAL_UINT8(k, before.next);
        TEST_ASSERT_EQUAL_UINT8(k, at.period);
        TEST_ASSERT_EQUAL_INT32(0, at.elapsed_s);
        TEST_ASSERT_TRUE(at.remaining_s == at.end - at.start);
        TEST_ASSERT_TRUE(at.entered);
    }
}

static void test_before_imsak() {  // US1 scenario 4
    const MainView v = eval(fixture_at(hms(3, 30, 0)));
    TEST_ASSERT_EQUAL_UINT8(kYatsi, v.period);
    TEST_ASSERT_TRUE(v.start == local(Date{2026, 11, 16}, 19, 15));
    TEST_ASSERT_TRUE(v.end == local(fixture::kDate, 6, 16));
    TEST_ASSERT_EQUAL_UINT8(kImsak, v.next);
    TEST_ASSERT_FALSE(v.next_is_tomorrow);
    TEST_ASSERT_EQUAL_STRING("02:46:00", v.countdown_text);
    TEST_ASSERT_EQUAL_STRING("19:15", v.start_text);
    TEST_ASSERT_EQUAL_STRING("06:16", v.end_text);
    TEST_ASSERT_EQUAL_STRING("Yatsı vakti", v.period_name);
    TEST_ASSERT_EQUAL_STRING("İmsaka kalan süre", v.countdown_label);
}

static void test_after_yatsi() {  // US1 scenario 5
    const MainView v = eval(fixture_at(hms(21, 30, 0)));
    TEST_ASSERT_EQUAL_UINT8(kYatsi, v.period);
    TEST_ASSERT_TRUE(v.start == local(fixture::kDate, 19, 14));
    TEST_ASSERT_TRUE(v.end == local(Date{2026, 11, 18}, 6, 17));
    TEST_ASSERT_EQUAL_UINT8(kImsak, v.next);
    TEST_ASSERT_TRUE(v.next_is_tomorrow);
    TEST_ASSERT_EQUAL_INT32(31620, v.remaining_s);
    TEST_ASSERT_EQUAL_STRING("08:47:00", v.countdown_text);
    TEST_ASSERT_EQUAL_STRING("06:17", v.end_text);
}

static void test_soon() {  // US1 scenario 6, FR-007
    TEST_ASSERT_TRUE(eval(fixture_at(hms(17, 36, 0))).soon);
    TEST_ASSERT_FALSE(eval(fixture_at(hms(17, 35, 59))).soon);
    Settings none;
    none.soon_threshold_min = 0;
    TEST_ASSERT_FALSE(eval(fixture_at(hms(17, 50, 59), none)).soon);
    Settings thirty;
    thirty.soon_threshold_min = 30;
    TEST_ASSERT_TRUE(eval(fixture_at(hms(17, 21, 0), thirty)).soon);
    TEST_ASSERT_FALSE(eval(fixture_at(hms(17, 20, 59), thirty)).soon);
}

static void test_entered() {  // US1 scenario 7
    TEST_ASSERT_TRUE(eval(fixture_at(hms(12, 58, 59))).entered);
    TEST_ASSERT_FALSE(eval(fixture_at(hms(12, 59, 0))).entered);
    TEST_ASSERT_FALSE(eval(fixture_at(hms(3, 30, 0))).entered);  // night period started yesterday
}

static void test_period_names_follow_the_table() {  // 02 §9.2
    for (uint8_t k = 0; k < kVakitCount; ++k) {
        const MainView v = eval(fixture_at(fixture::kToday.minutes[k] * 60 + 120));
        TEST_ASSERT_EQUAL_STRING(tr::kPeriodNames[k], v.period_name);
        TEST_ASSERT_EQUAL_STRING(tr::kCountdownLabels[k], v.countdown_label);
    }
}

void run_period_tests() {
    RUN_TEST(test_fixture_at_143207);
    RUN_TEST(test_one_second_before_and_at_each_vakit);
    RUN_TEST(test_before_imsak);
    RUN_TEST(test_after_yatsi);
    RUN_TEST(test_soon);
    RUN_TEST(test_entered);
    RUN_TEST(test_period_names_follow_the_table);
}
