// Stress schedule and counters (tasks T039; data-model.md StressCounters; FR-010, FR-011).
// Written before the code.
#include <unity.h>

#include "nvp/diag/stress.h"

using namespace nvp::diag;

static void test_first_cycle_runs_at_start() {
    StressSchedule s;
    s.start(1000);
    const StressPlan p = s.next(1000, true, false);
    TEST_ASSERT_TRUE(p.scan);
    TEST_ASSERT_TRUE(p.write);
    TEST_ASSERT_TRUE(p.https);  // never attempted yet, so it is due
}

static void test_scan_and_write_every_10_s() {
    StressSchedule s;
    s.start(0);
    s.next(0, true, false);
    TEST_ASSERT_FALSE(s.next(9999, true, false).scan);
    const StressPlan p = s.next(10000, true, false);
    TEST_ASSERT_TRUE(p.scan);
    TEST_ASSERT_TRUE(p.write);
    TEST_ASSERT_FALSE(s.next(10001, true, false).write);
}

static void test_https_at_most_every_30_s() {
    StressSchedule s;
    s.start(0);
    TEST_ASSERT_TRUE(s.next(0, true, false).https);
    TEST_ASSERT_FALSE(s.next(10000, true, false).https);
    TEST_ASSERT_FALSE(s.next(20000, true, false).https);
    TEST_ASSERT_TRUE(s.next(30000, true, false).https);
}

static void test_skip_only_when_due_and_not_connected() {
    StressSchedule s;
    s.start(0);
    const StressPlan first = s.next(0, false, false);
    TEST_ASSERT_FALSE(first.https);
    TEST_ASSERT_TRUE(first.https_skip);
    const StressPlan not_due = s.next(10000, false, false);
    TEST_ASSERT_FALSE(not_due.https_skip);  // a cycle with no request due is not a skip (FR-010)
    TEST_ASSERT_TRUE(s.next(30000, false, false).https_skip);
}

static void test_paused_while_hotspot_is_up() {
    StressSchedule s;
    s.start(0);
    const StressPlan p = s.next(0, true, true);
    TEST_ASSERT_TRUE(p.paused);
    TEST_ASSERT_FALSE(p.scan);
    TEST_ASSERT_FALSE(p.write);
    TEST_ASSERT_FALSE(p.https);
    TEST_ASSERT_FALSE(p.https_skip);
    // When the hotspot closes, the cycle that was due runs.
    TEST_ASSERT_TRUE(s.next(500, true, false).scan);
}

static void test_counters_over_ten_minutes() {
    StressSchedule s;
    StressCounters c;
    s.start(0);
    c.start(0);
    for (uint32_t t = 0; t <= 600000; t += 500) {
        const StressPlan p = s.next(t, true, false);
        if (p.scan) c.record_scan(true);
        if (p.write) c.record_write(true);
        if (p.https) c.record_https(HttpsOutcome::Ok);
        if (p.https_skip) c.record_https(HttpsOutcome::Skipped);
        if (p.scan && p.write) c.cycle_done(t);
    }
    TEST_ASSERT_EQUAL_UINT32(61, c.cycles);  // t = 0, 10 s, ..., 600 s
    TEST_ASSERT_EQUAL_UINT32(61, c.scan_ok);
    TEST_ASSERT_EQUAL_UINT32(21, c.https_ok);  // t = 0, 30 s, ..., 600 s
    TEST_ASSERT_EQUAL_UINT32(0, c.https_skip);
    TEST_ASSERT_EQUAL_UINT32(600, c.elapsed_s);
}

static void test_errors_and_last_error() {
    StressCounters c;
    c.start(0);
    c.record_write(false, "write failed");
    c.record_https(HttpsOutcome::Error, "https: timeout");
    TEST_ASSERT_EQUAL_UINT32(1, c.write_err);
    TEST_ASSERT_EQUAL_UINT32(1, c.https_err);
    TEST_ASSERT_EQUAL_STRING("https: timeout", c.last_error);
}

static void test_elapsed_follows_the_clock_between_cycles() {
    // A run stopped between cycles must report its real length, not the time of its last cycle.
    StressCounters c;
    c.start(1000);
    c.cycle_done(1000);
    c.update_elapsed(13500);
    TEST_ASSERT_EQUAL_UINT32(12, c.elapsed_s);
    TEST_ASSERT_EQUAL_UINT32(1, c.cycles);
}

void run_stress_tests() {
    RUN_TEST(test_elapsed_follows_the_clock_between_cycles);
    RUN_TEST(test_first_cycle_runs_at_start);
    RUN_TEST(test_scan_and_write_every_10_s);
    RUN_TEST(test_https_at_most_every_30_s);
    RUN_TEST(test_skip_only_when_due_and_not_connected);
    RUN_TEST(test_paused_while_hotspot_is_up);
    RUN_TEST(test_counters_over_ten_minutes);
    RUN_TEST(test_errors_and_last_error);
}
