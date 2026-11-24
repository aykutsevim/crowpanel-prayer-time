// The fourteen reference fixtures (02-ui-spec.md §11; spec 003 FR-017, research R10).
// Written before the code.
#include <unity.h>

#include <cstdio>
#include <cstring>

#include "nvp/screen/fixtures.h"

using namespace nvp::screen;

static int32_t hms(int h, int m, int s) { return h * 3600 + m * 60 + s; }

static void test_numbering_and_names() {
    const char* names[17] = {"01-main-ogle-1432.png",       "02-main-gece-0330.png",     "03-main-imsak-0650.png",
                             "04-main-kusluk-1000.png",     "05-main-vakit-girdi-1255.png",
                             "06-main-ikindi-1620.png",     "07-main-az-kaldi-1742.png", "08-main-aksam-1820.png",
                             "09-main-yatsi-2130.png",      "10-state-offline.png",      "11-state-cache-ending.png",
                             "12-state-calculated.png",     "13-state-time-not-set.png", "14-state-no-data.png",
                             "15-setup-ap.png",             "16-connecting.png",         "17-status-overlay.png"};
    TEST_ASSERT_EQUAL_UINT32(17, kFixtureCount);
    for (uint8_t i = 0; i < kFixtureCount; ++i) {
        TEST_ASSERT_EQUAL_UINT8(i + 1, kFixtures[i].number);
        TEST_ASSERT_EQUAL_STRING(names[i], kFixtures[i].reference);
    }
}

static void test_reference_files_exist() {
    for (uint8_t i = 0; i < kFixtureCount; ++i) {
        char path[128];
        std::snprintf(path, sizeof path, "docs/design/screens/%s", kFixtures[i].reference);
        std::FILE* f = std::fopen(path, "rb");
        if (f == nullptr) TEST_FAIL_MESSAGE(path);
        std::fclose(f);
    }
}

static void test_times_and_states() {
    const int32_t times[17] = {hms(14, 32, 7), hms(3, 30, 0),  hms(6, 50, 0),  hms(10, 0, 0),  hms(12, 55, 0),
                               hms(16, 20, 0), hms(17, 42, 0), hms(18, 20, 0), hms(21, 30, 0), hms(14, 32, 7),
                               hms(14, 32, 7), hms(14, 32, 7), -1,             hms(14, 32, 7), hms(14, 32, 7),
                               hms(14, 32, 7), hms(14, 32, 7)};
    const SystemState states[17] = {SystemState::Ok,          SystemState::Ok,         SystemState::Ok,
                                    SystemState::Ok,          SystemState::Ok,         SystemState::Ok,
                                    SystemState::Ok,          SystemState::Ok,         SystemState::Ok,
                                    SystemState::Offline,     SystemState::CacheEnding, SystemState::Calculated,
                                    SystemState::NoTime,      SystemState::NoData,     SystemState::Ok,
                                    SystemState::Ok,          SystemState::Ok};
    for (uint8_t i = 0; i < kFixtureCount; ++i) {
        TEST_ASSERT_EQUAL_INT32(times[i], kFixtures[i].second_of_day);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(states[i]), static_cast<int>(kFixtures[i].state));
    }
    TEST_ASSERT_EQUAL_INT16(5, kFixtures[10].days_left);
    for (uint8_t i = 0; i < 14; ++i) TEST_ASSERT_TRUE(kFixtures[i].screen == FixtureScreen::Main);
    TEST_ASSERT_TRUE(kFixtures[14].screen == FixtureScreen::Setup);       // spec 004 FR-009
    TEST_ASSERT_TRUE(kFixtures[15].screen == FixtureScreen::Connecting);
    TEST_ASSERT_TRUE(kFixtures[16].screen == FixtureScreen::Overlay);
}

static void test_lookup() {
    TEST_ASSERT_NOT_NULL(fixture(1));
    TEST_ASSERT_NOT_NULL(fixture(14));
    TEST_ASSERT_NULL(fixture(0));
    TEST_ASSERT_NOT_NULL(fixture(17));
    TEST_ASSERT_NULL(fixture(18));
    TEST_ASSERT_EQUAL_STRING("İstanbul", kFixtureCity);
    TEST_ASSERT_EQUAL_STRING("03:00", kFixtureLastUpdate);
}

void run_fixture_tests() {
    RUN_TEST(test_numbering_and_names);
    RUN_TEST(test_reference_files_exist);
    RUN_TEST(test_times_and_states);
    RUN_TEST(test_lookup);
}
