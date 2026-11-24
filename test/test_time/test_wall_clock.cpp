// The validity rule, sources and corrections (spec 005 FR-003..FR-006, TIME-03, TIME-05, TIME-08;
// research R4, data-model.md). Written before the code.
#include <unity.h>

#include <cstdint>

#include "nvp/time/wall_clock.h"

using namespace nvp::time;

namespace {

constexpr int64_t kBuild = 1795478400;            // 2026-11-24 00:00:00 UTC
constexpr int64_t kNow = 1795520477;              // 2026-11-24 11:41:17 UTC
constexpr int64_t kUs = 1000000;
constexpr int64_t kHour = 3600 * kUs;

TimeSample sample(TimeSource s, int64_t utc_s, int64_t at_uptime_us, int64_t frac_us = 0) {
    TimeSample t;
    t.source = s;
    t.utc_us = utc_s * kUs + frac_us;
    t.at_uptime_us = at_uptime_us;
    t.server = "test";
    return t;
}

}  // namespace

static void test_starts_not_valid() {
    WallClock c;
    TEST_ASSERT_FALSE(c.valid());
    TEST_ASSERT_TRUE(c.source() == TimeSource::None);
}

static void test_rtc_start_makes_valid_without_rtc_write() {
    WallClock c;
    const SyncResult r = c.accept(sample(TimeSource::Rtc, kNow, 0), kBuild * kUs, 0, kBuild);
    TEST_ASSERT_TRUE(r.outcome == SyncOutcome::Accepted);
    TEST_ASSERT_FALSE(r.write_rtc);
    TEST_ASSERT_TRUE(c.valid());
    TEST_ASSERT_TRUE(c.source() == TimeSource::Rtc);
}

static void test_before_build_refused() {
    WallClock c;
    const SyncResult r = c.accept(sample(TimeSource::Ntp, kBuild - 1, 0), kBuild * kUs, 0, kBuild);
    TEST_ASSERT_TRUE(r.outcome == SyncOutcome::RefusedBeforeBuild);
    TEST_ASSERT_FALSE(c.valid());
    c.accept(sample(TimeSource::Rtc, kNow, 0), kBuild * kUs, 0, kBuild);
    TEST_ASSERT_TRUE(c.accept(sample(TimeSource::Ntp, 1000, 0), kNow * kUs, 0, kBuild).outcome ==
                     SyncOutcome::RefusedBeforeBuild);
    TEST_ASSERT_TRUE(c.valid());  // a refusal leaves a valid clock valid
    TEST_ASSERT_TRUE(c.source() == TimeSource::Rtc);
}

static void test_ntp_correction_advanced_by_delay() {
    WallClock c;
    c.accept(sample(TimeSource::Rtc, kNow, 0), kNow * kUs, 0, kBuild);
    // Taken at uptime 10 s, processed at uptime 10.2 s; the system clock then reads kNow + 10.4 s.
    const SyncResult r = c.accept(sample(TimeSource::Ntp, kNow + 10, 10 * kUs), (kNow + 10) * kUs + 400000,
                                  10 * kUs + 200000, kBuild);
    TEST_ASSERT_TRUE(r.outcome == SyncOutcome::Accepted);
    TEST_ASSERT_TRUE(r.new_utc_us == (kNow + 10) * kUs + 200000);
    TEST_ASSERT_TRUE(r.correction_us == -200000);
    TEST_ASSERT_TRUE(r.write_rtc);
    TEST_ASSERT_TRUE(c.source() == TimeSource::Ntp);
    TEST_ASSERT_TRUE(c.last_correction_us() == -200000);
    TEST_ASSERT_TRUE(c.last_sync_uptime_us() == 10 * kUs + 200000);
}

static void test_http_ignored_while_ntp_fresh() {
    WallClock c;
    c.accept(sample(TimeSource::Ntp, kNow, 0), kNow * kUs, 0, kBuild);
    TEST_ASSERT_TRUE(c.accept(sample(TimeSource::Http, kNow + 60, 60 * kUs), (kNow + 60) * kUs, 60 * kUs, kBuild)
                         .outcome == SyncOutcome::IgnoredNtpFresh);
    // After 6 h the HTTP fallback may correct again.
    const int64_t later = 6 * kHour + kUs;
    TEST_ASSERT_TRUE(c.accept(sample(TimeSource::Http, kNow + 21601, later), (kNow + 21601) * kUs, later, kBuild)
                         .outcome == SyncOutcome::Accepted);
    TEST_ASSERT_TRUE(c.source() == TimeSource::Http);
}

static void test_forget_ntp_freshness_lets_http_in() {  // `ntp off` (R11)
    WallClock c;
    c.accept(sample(TimeSource::Ntp, kNow, 0), kNow * kUs, 0, kBuild);
    c.forget_ntp_freshness();
    const SyncResult r = c.accept(sample(TimeSource::Http, kNow + 61, 60 * kUs), (kNow + 60) * kUs, 60 * kUs, kBuild);
    TEST_ASSERT_TRUE(r.outcome == SyncOutcome::Accepted);
    TEST_ASSERT_TRUE(r.write_rtc);
    TEST_ASSERT_TRUE(r.correction_us == kUs);
}

static void test_every_network_sample_writes_rtc() {  // FR-004, analysis I1
    WallClock c;
    for (int i = 0; i < 3; ++i) {
        const int64_t up = i * 60 * kUs;
        TEST_ASSERT_TRUE(c.accept(sample(TimeSource::Ntp, kNow + i * 60, up), (kNow + i * 60) * kUs, up, kBuild).write_rtc);
    }
}

static void test_invalidate_until_next_sample() {  // `clock invalid` (R11)
    WallClock c;
    c.accept(sample(TimeSource::Ntp, kNow, 0), kNow * kUs, 0, kBuild);
    c.invalidate();
    TEST_ASSERT_FALSE(c.valid());
    TEST_ASSERT_TRUE(c.source() == TimeSource::None);
    c.accept(sample(TimeSource::Ntp, kNow + 5, 5 * kUs), kBuild * kUs, 5 * kUs, kBuild);
    TEST_ASSERT_TRUE(c.valid());
}

static void test_stays_valid_for_days_without_samples() {  // FR-006
    WallClock c;
    c.accept(sample(TimeSource::Ntp, kNow, 0), kNow * kUs, 0, kBuild);
    TEST_ASSERT_TRUE(c.valid());  // no clock-based expiry exists: nothing to advance
    TEST_ASSERT_TRUE(c.source() == TimeSource::Ntp);
}

static void test_correction_across_midnight_is_one_step() {  // analysis C2
    WallClock c;
    const int64_t before_midnight = kBuild + 86400 - 2;  // 23:59:58 UTC
    c.accept(sample(TimeSource::Rtc, before_midnight, 0), before_midnight * kUs, 0, kBuild);
    const SyncResult r = c.accept(sample(TimeSource::Ntp, before_midnight + 5, 0), before_midnight * kUs, 0, kBuild);
    TEST_ASSERT_TRUE(r.outcome == SyncOutcome::Accepted);
    TEST_ASSERT_TRUE(r.correction_us == 5 * kUs);
    TEST_ASSERT_TRUE(r.new_utc_us == (kBuild + 86403) * kUs);
}

static void test_names() {
    TEST_ASSERT_EQUAL_STRING("none", source_name(TimeSource::None));
    TEST_ASSERT_EQUAL_STRING("rtc", source_name(TimeSource::Rtc));
    TEST_ASSERT_EQUAL_STRING("ntp", source_name(TimeSource::Ntp));
    TEST_ASSERT_EQUAL_STRING("http", source_name(TimeSource::Http));
}

void run_wall_clock_tests() {
    RUN_TEST(test_starts_not_valid);
    RUN_TEST(test_rtc_start_makes_valid_without_rtc_write);
    RUN_TEST(test_before_build_refused);
    RUN_TEST(test_ntp_correction_advanced_by_delay);
    RUN_TEST(test_http_ignored_while_ntp_fresh);
    RUN_TEST(test_forget_ntp_freshness_lets_http_in);
    RUN_TEST(test_every_network_sample_writes_rtc);
    RUN_TEST(test_invalidate_until_next_sample);
    RUN_TEST(test_stays_valid_for_days_without_samples);
    RUN_TEST(test_correction_across_midnight_is_one_step);
    RUN_TEST(test_names);
}
