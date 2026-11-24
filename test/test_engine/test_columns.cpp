// US2: the six columns (FR-010 to FR-013, 02-ui-spec.md §4.5). Written before the code.
#include <unity.h>

#include "helpers.h"
#include "nvp/text/tr.h"

using namespace engine_test;
namespace tr = nvp::text::tr;

static void expect(const Column& c, ColumnState state, const char* note) {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(state), static_cast<int>(c.state));
    TEST_ASSERT_EQUAL_STRING(note, c.note);
}

static void test_afternoon() {  // US2 scenario 1
    const MainView v = eval(fixture_at(fixture::kCheckSecond));
    expect(v.columns[kImsak], ColumnState::Past, "");
    expect(v.columns[kGunes], ColumnState::Past, "");
    expect(v.columns[kOgle], ColumnState::Current, "şu an");
    expect(v.columns[kIkindi], ColumnState::Next, "54 dk sonra");
    expect(v.columns[kAksam], ColumnState::Later, "3 sa 18 dk sonra");
    expect(v.columns[kYatsi], ColumnState::Later, "4 sa 41 dk sonra");
    TEST_ASSERT_EQUAL_INT32(3293, v.columns[kIkindi].seconds_until);
    TEST_ASSERT_EQUAL_INT32(0, v.columns[kOgle].seconds_until);
    TEST_ASSERT_EQUAL_INT32(0, v.columns[kImsak].seconds_until);
    TEST_ASSERT_EQUAL_STRING("15:27", v.columns[kIkindi].time_text);
}

static void test_entered_note() {  // US2 scenario 2
    expect(eval(fixture_at(hms(12, 55, 0))).columns[kOgle], ColumnState::Current, "vakit girdi");
    expect(eval(fixture_at(hms(19, 14, 0))).columns[kYatsi], ColumnState::Current, "vakit girdi");
}

static void test_before_imsak() {  // US2 scenario 3, FR-012
    const MainView v = eval(fixture_at(hms(3, 30, 0)));
    expect(v.columns[kImsak], ColumnState::Next, "2 sa 46 dk sonra");
    for (uint8_t k = 1; k < kVakitCount; ++k) {
        TEST_ASSERT_EQUAL_INT(static_cast<int>(ColumnState::Later), static_cast<int>(v.columns[k].state));
    }
    for (uint8_t k = 0; k < kVakitCount; ++k) {
        TEST_ASSERT_EQUAL_UINT16(fixture::kToday.minutes[k], v.columns[k].minutes);
    }
    TEST_ASSERT_EQUAL_STRING("15 sa 44 dk sonra", v.columns[kYatsi].note);
}

static void test_after_yatsi() {  // US2 scenario 4, FR-012
    const MainView v = eval(fixture_at(hms(21, 30, 0)));
    expect(v.columns[kYatsi], ColumnState::Current, "şu an");
    expect(v.columns[kImsak], ColumnState::Next, "yarın");
    TEST_ASSERT_EQUAL_STRING("06:17", v.columns[kImsak].time_text);
    TEST_ASSERT_EQUAL_UINT16(377, v.columns[kImsak].minutes);
    TEST_ASSERT_EQUAL_INT32(31620, v.columns[kImsak].seconds_until);
    for (uint8_t k = kGunes; k <= kAksam; ++k) expect(v.columns[k], ColumnState::Past, "");
}

static void test_states_switch_at_the_exact_second() {  // US2 scenario 5
    for (uint8_t k = 1; k < kVakitCount; ++k) {  // İmsak is covered by the before-İmsak case
        const int32_t at_s = fixture::kToday.minutes[k] * 60;
        const MainView before = eval(fixture_at(at_s - 1));
        const MainView at = eval(fixture_at(at_s));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(ColumnState::Next), static_cast<int>(before.columns[k].state));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(ColumnState::Current), static_cast<int>(before.columns[k - 1].state));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(ColumnState::Current), static_cast<int>(at.columns[k].state));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(ColumnState::Past), static_cast<int>(at.columns[k - 1].state));
        TEST_ASSERT_EQUAL_STRING("1 dk’dan az sonra", before.columns[k].note);
    }
    const int32_t imsak = fixture::kToday.minutes[kImsak] * 60;
    const MainView before = eval(fixture_at(imsak - 1));
    const MainView at = eval(fixture_at(imsak));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ColumnState::Next), static_cast<int>(before.columns[kImsak].state));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ColumnState::Current), static_cast<int>(at.columns[kImsak].state));
}

static void test_names_and_one_next() {
    const MainView v = eval(fixture_at(hms(10, 0, 0)));
    int next = 0;
    for (uint8_t k = 0; k < kVakitCount; ++k) {
        TEST_ASSERT_EQUAL_STRING(tr::kVakitNames[k], v.columns[k].name);
        if (v.columns[k].state == ColumnState::Next) ++next;
    }
    TEST_ASSERT_EQUAL_INT(1, next);
}

void run_column_tests() {
    RUN_TEST(test_afternoon);
    RUN_TEST(test_entered_note);
    RUN_TEST(test_before_imsak);
    RUN_TEST(test_after_yatsi);
    RUN_TEST(test_states_switch_at_the_exact_second);
    RUN_TEST(test_names_and_one_next);
}
