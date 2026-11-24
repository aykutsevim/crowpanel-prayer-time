// Day record validation and source ranks (ENG-01, FR-001 to FR-002, research R8).
// Written before the code.
#include <unity.h>

#include "nvp/engine/day_record.h"
#include "nvp/engine/fixture.h"

using namespace nvp::engine;

static DayRecord with_times(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e, uint16_t f) {
    DayRecord r = fixture::kToday;
    r.minutes[0] = a;
    r.minutes[1] = b;
    r.minutes[2] = c;
    r.minutes[3] = d;
    r.minutes[4] = e;
    r.minutes[5] = f;
    return r;
}

static void test_fixture_records_are_valid() {
    TEST_ASSERT_TRUE(valid(fixture::kYesterday));
    TEST_ASSERT_TRUE(valid(fixture::kToday));
    TEST_ASSERT_TRUE(valid(fixture::kTomorrow));
}

static void test_times_must_strictly_increase() {
    TEST_ASSERT_FALSE(valid(with_times(376, 466, 466, 927, 1071, 1154)));  // equal (ENG-01)
    TEST_ASSERT_FALSE(valid(with_times(376, 466, 774, 700, 1071, 1154)));  // decreasing
    TEST_ASSERT_TRUE(valid(with_times(0, 1, 2, 3, 4, 1439)));             // the ends of the day
    TEST_ASSERT_FALSE(valid(with_times(376, 466, 774, 927, 1071, 1440))); // past 23:59
}

static void test_invalid_date_makes_record_invalid() {
    DayRecord r = fixture::kToday;
    r.date = Date{2026, 2, 30};
    TEST_ASSERT_FALSE(valid(r));
}

static void test_source_ranks() {
    TEST_ASSERT_EQUAL_UINT8(50, rank(SourceId::Diyanet));
    TEST_ASSERT_EQUAL_UINT8(40, rank(SourceId::Imsakiyem));
    TEST_ASSERT_EQUAL_UINT8(40, rank(SourceId::Emushaf));
    TEST_ASSERT_EQUAL_UINT8(20, rank(SourceId::Aladhan));
    TEST_ASSERT_EQUAL_UINT8(10, rank(SourceId::Local));
}

void run_record_tests() {
    RUN_TEST(test_fixture_records_are_valid);
    RUN_TEST(test_times_must_strictly_increase);
    RUN_TEST(test_invalid_date_makes_record_invalid);
    RUN_TEST(test_source_ranks);
}
