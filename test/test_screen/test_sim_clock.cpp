// Simulated clock and time zone (spec 003 FR-018, FR-027, research R2, R11). Written before the code.
#include <unity.h>

#include "nvp/time/sim_clock.h"
#include "nvp/time/zone.h"

using namespace nvp::time;

static void test_speed_one() {
    SimClock c;
    c.set(1000000, 5000);
    TEST_ASSERT_TRUE(c.now(5000) == 1000000);
    TEST_ASSERT_TRUE(c.now(6000) == 1000001);
    TEST_ASSERT_TRUE(c.now(6999) == 1000001);
    TEST_ASSERT_TRUE(c.now(65000) == 1000060);
}

static void test_speed_sixty() {
    SimClock c;
    c.set(0, 0);
    TEST_ASSERT_TRUE(c.set_speed(60, 0));
    TEST_ASSERT_TRUE(c.now(1000) == 60);
    TEST_ASSERT_TRUE(c.now(10500) == 630);
}

static void test_speed_change_does_not_jump() {
    SimClock c;
    c.set(0, 0);
    c.set_speed(10, 0);
    const int64_t before = c.now(7300);  // 73 s
    c.set_speed(3600, 7300);
    TEST_ASSERT_TRUE(c.now(7300) == before);
    TEST_ASSERT_TRUE(c.now(8300) == before + 3600);
}

static void test_speed_limits() {
    SimClock c;
    TEST_ASSERT_FALSE(c.set_speed(0, 0));
    TEST_ASSERT_FALSE(c.set_speed(3601, 0));
    TEST_ASSERT_TRUE(c.set_speed(3600, 0));
    TEST_ASSERT_EQUAL_UINT16(3600, c.speed());
}

static void test_pause_and_run() {
    SimClock c;
    c.set(100, 0);
    c.pause(2000);
    TEST_ASSERT_TRUE(c.now(2000) == 102);
    TEST_ASSERT_TRUE(c.now(90000) == 102);
    c.run(90000);
    TEST_ASSERT_TRUE(c.now(91000) == 103);
    TEST_ASSERT_TRUE(c.paused() == false);
}

static void test_millisecond_wrap() {
    SimClock c;
    c.set(500, 0xFFFFF000u);
    TEST_ASSERT_TRUE(c.now(0xFFFFF000u + 4096u) == 504);  // wrapped to 0 and beyond
}

static void test_zone() {
    TEST_ASSERT_EQUAL_INT16(180, kDefaultUtcOffsetMinutes);
    TEST_ASSERT_TRUE(utc_to_local(0) == 10800);
    TEST_ASSERT_TRUE(local_to_utc(10800) == 0);
}

void run_sim_clock_tests() {
    RUN_TEST(test_speed_one);
    RUN_TEST(test_speed_sixty);
    RUN_TEST(test_speed_change_does_not_jump);
    RUN_TEST(test_speed_limits);
    RUN_TEST(test_pause_and_run);
    RUN_TEST(test_millisecond_wrap);
    RUN_TEST(test_zone);
}
