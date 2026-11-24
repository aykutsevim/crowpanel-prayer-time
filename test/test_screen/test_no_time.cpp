// Engine output without a valid clock (02-ui-spec.md §6 row 1, reference 13; spec 003 research R7).
// Written before the code.
#include <unity.h>

#include "nvp/engine/engine.h"
#include "nvp/engine/fixture.h"

using namespace nvp::engine;

static void test_with_stored_record() {
    MainView v;
    evaluate_no_time(&fixture::kToday, Settings{}, v);
    TEST_ASSERT_FALSE(v.has_data);
    TEST_ASSERT_TRUE(v.has_stored_times);
    for (uint8_t k = 0; k < kVakitCount; ++k) {
        TEST_ASSERT_EQUAL_INT(static_cast<int>(ColumnState::Later), static_cast<int>(v.columns[k].state));
        TEST_ASSERT_EQUAL_STRING("", v.columns[k].note);
        TEST_ASSERT_EQUAL_UINT16(fixture::kToday.minutes[k], v.columns[k].minutes);
    }
    TEST_ASSERT_EQUAL_STRING("06:16", v.columns[kImsak].time_text);
    TEST_ASSERT_EQUAL_STRING("19:14", v.columns[kYatsi].time_text);
    TEST_ASSERT_TRUE(v.kerahat[0].shown);  // the strip is drawn from the stored times
    TEST_ASSERT_EQUAL_STRING("", v.countdown_text);
    TEST_ASSERT_EQUAL_STRING("", v.period_name);
    TEST_ASSERT_EQUAL_STRING("", v.clock_hhmm);
    TEST_ASSERT_EQUAL_STRING("", v.hijri_text);  // reference 13: Hijri line empty
}

static void test_without_record() {
    MainView v;
    evaluate_no_time(nullptr, Settings{}, v);
    TEST_ASSERT_FALSE(v.has_data);
    TEST_ASSERT_FALSE(v.has_stored_times);
    for (uint8_t k = 0; k < kVakitCount; ++k) TEST_ASSERT_EQUAL_STRING("", v.columns[k].time_text);
    for (const KerahatInterval& i : v.kerahat) TEST_ASSERT_FALSE(i.shown);
}

static void test_invalid_record_is_none() {
    DayRecord bad = fixture::kToday;
    bad.minutes[kOgle] = bad.minutes[kGunes];
    MainView v;
    evaluate_no_time(&bad, Settings{}, v);
    TEST_ASSERT_FALSE(v.has_stored_times);
}

static void test_normal_evaluation_reports_stored_times() {
    Input in;
    in.now = Moment{fixture::kDate, 14 * 3600};
    in.today = &fixture::kToday;
    MainView v;
    evaluate(in, v);
    TEST_ASSERT_TRUE(v.has_stored_times);
    in.today = nullptr;
    evaluate(in, v);
    TEST_ASSERT_FALSE(v.has_stored_times);
}

void run_no_time_tests() {
    RUN_TEST(test_with_stored_record);
    RUN_TEST(test_without_record);
    RUN_TEST(test_invalid_record_is_none);
    RUN_TEST(test_normal_evaluation_reports_stored_times);
}
