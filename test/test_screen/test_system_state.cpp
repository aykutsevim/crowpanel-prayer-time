// Choosing the system state, top to bottom (02-ui-spec.md §6; spec 003 FR-012, UI-01).
// Written before the code.
#include <unity.h>

#include "nvp/screen/system_state.h"

using namespace nvp::screen;

static StateInputs all_good() {
    StateInputs in;
    in.clock_valid = true;
    in.today_present = true;
    in.today_rank = 40;
    in.refresh_failed_or_wifi_down = false;
    in.stored_days_after_today = 200;
    in.wifi_down_minutes = 0;
    return in;
}

static void expect(SystemState s, const StateInputs& in) {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(s), static_cast<int>(select_state(in)));
}

static void test_ok() { expect(SystemState::Ok, all_good()); }

static void test_each_row() {
    StateInputs in = all_good();
    in.clock_valid = false;
    expect(SystemState::NoTime, in);

    in = all_good();
    in.today_present = false;
    expect(SystemState::NoData, in);

    in = all_good();
    in.today_rank = 20;
    expect(SystemState::Calculated, in);

    in = all_good();
    in.refresh_failed_or_wifi_down = true;
    in.stored_days_after_today = 7;
    expect(SystemState::CacheEnding, in);
    in.stored_days_after_today = 8;
    expect(SystemState::Ok, in);  // not ending yet, and Wi-Fi not down long enough

    in = all_good();
    in.wifi_down_minutes = 10;
    expect(SystemState::Ok, in);  // "more than 10 min"
    in.wifi_down_minutes = 11;
    expect(SystemState::Offline, in);
}

static void test_cache_ending_needs_failure() {
    StateInputs in = all_good();
    in.stored_days_after_today = 3;  // few days, but refreshing works
    expect(SystemState::Ok, in);
}

static void test_first_match_wins() {
    StateInputs in = all_good();
    in.clock_valid = false;
    in.today_present = false;
    in.today_rank = 10;
    in.refresh_failed_or_wifi_down = true;
    in.stored_days_after_today = 0;
    in.wifi_down_minutes = 999;
    expect(SystemState::NoTime, in);
    in.clock_valid = true;
    expect(SystemState::NoData, in);
    in.today_present = true;
    expect(SystemState::Calculated, in);
    in.today_rank = 40;
    expect(SystemState::CacheEnding, in);
    in.stored_days_after_today = 30;
    expect(SystemState::Offline, in);
}

static void test_forced_state_overrides() {
    StateInputs in = all_good();
    in.forced = true;
    in.forced_state = SystemState::Offline;
    expect(SystemState::Offline, in);
    in.clock_valid = false;
    expect(SystemState::Offline, in);
}

static void test_names() {
    TEST_ASSERT_EQUAL_STRING("no_time", state_name(SystemState::NoTime));
    TEST_ASSERT_EQUAL_STRING("cache_ending", state_name(SystemState::CacheEnding));
    SystemState s;
    TEST_ASSERT_TRUE(parse_state_name("offline", &s));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SystemState::Offline), static_cast<int>(s));
    TEST_ASSERT_FALSE(parse_state_name("auto", &s));
    TEST_ASSERT_FALSE(parse_state_name("", &s));
}

void run_system_state_tests() {
    RUN_TEST(test_ok);
    RUN_TEST(test_each_row);
    RUN_TEST(test_cache_ending_needs_failure);
    RUN_TEST(test_first_match_wins);
    RUN_TEST(test_forced_state_overrides);
    RUN_TEST(test_names);
}
