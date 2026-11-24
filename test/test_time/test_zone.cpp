// POSIX time zones (spec 005 FR-012, TIME-04, research R5). Written before the code.
#include <unity.h>

#include <cstdint>

#include "nvp/time/posix_zone.h"

using namespace nvp::time;

namespace {
constexpr int64_t kNow = 1795520477;          // 2026-11-24 11:41:17 UTC
constexpr int64_t kCetStart = 1774746000;     // 2026-03-29 01:00:00 UTC
constexpr int64_t kCetEnd = 1792890000;       // 2026-10-25 01:00:00 UTC
constexpr int64_t kEstStart = 1772953200;     // 2026-03-08 07:00:00 UTC
constexpr int64_t kEstEnd = 1793512800;       // 2026-11-01 06:00:00 UTC
constexpr int64_t kH = 3600;

Zone parse(const char* s) {
    Zone z;
    TEST_ASSERT_TRUE_MESSAGE(parse_zone(s, z), s);
    return z;
}
}  // namespace

static void test_turkiye_default() {
    TEST_ASSERT_EQUAL_STRING("<+03>-3", kDefaultZone);
    const Zone z = parse("<+03>-3");
    TEST_ASSERT_FALSE(z.has_dst);
    TEST_ASSERT_EQUAL_INT32(3 * kH, z.std_offset_s);
    TEST_ASSERT_TRUE(utc_to_local(z, kNow) == kNow + 3 * kH);
    TEST_ASSERT_TRUE(local_to_utc(z, kNow + 3 * kH) == kNow);
    TEST_ASSERT_TRUE(utc_to_local(default_zone(), kNow) == kNow + 3 * kH);
}

static void test_utc_and_offsets_with_minutes() {
    TEST_ASSERT_TRUE(utc_to_local(parse("UTC0"), kNow) == kNow);
    TEST_ASSERT_TRUE(utc_to_local(parse("<+0530>-5:30"), kNow) == kNow + 5 * kH + 1800);
    TEST_ASSERT_TRUE(utc_to_local(parse("NST3:30"), kNow) == kNow - 3 * kH - 1800);
}

static void test_cet_transitions() {
    const Zone z = parse("CET-1CEST,M3.5.0,M10.5.0/3");
    TEST_ASSERT_TRUE(z.has_dst);
    TEST_ASSERT_TRUE(utc_to_local(z, kCetStart - 1) == kCetStart - 1 + kH);
    TEST_ASSERT_TRUE(utc_to_local(z, kCetStart) == kCetStart + 2 * kH);
    TEST_ASSERT_TRUE(utc_to_local(z, kCetEnd - 1) == kCetEnd - 1 + 2 * kH);
    TEST_ASSERT_TRUE(utc_to_local(z, kCetEnd) == kCetEnd + kH);
}

static void test_est_transitions() {
    const Zone z = parse("EST5EDT,M3.2.0,M11.1.0");
    TEST_ASSERT_TRUE(utc_to_local(z, kEstStart - 1) == kEstStart - 1 - 5 * kH);
    TEST_ASSERT_TRUE(utc_to_local(z, kEstStart) == kEstStart - 4 * kH);
    TEST_ASSERT_TRUE(utc_to_local(z, kEstEnd - 1) == kEstEnd - 1 - 4 * kH);
    TEST_ASSERT_TRUE(utc_to_local(z, kEstEnd) == kEstEnd - 5 * kH);
    // local_to_utc in summer and winter
    TEST_ASSERT_TRUE(local_to_utc(z, kEstStart + 3600 - 4 * kH) == kEstStart + 3600);
}

static void test_julian_rules() {
    // Same instants as CET, written with day numbers: J88 = 29 March (non-leap), n87 = 29 March (0-based).
    const Zone j = parse("CET-1CEST,J88/2,J298/3");
    TEST_ASSERT_TRUE(utc_to_local(j, kCetStart) == kCetStart + 2 * kH);
    TEST_ASSERT_TRUE(utc_to_local(j, kCetEnd) == kCetEnd + kH);
    const Zone n = parse("CET-1CEST,87/2,297/3");
    TEST_ASSERT_TRUE(utc_to_local(n, kCetStart) == kCetStart + 2 * kH);
    TEST_ASSERT_TRUE(utc_to_local(n, kCetStart - 1) == kCetStart - 1 + kH);
}

static void test_invalid_refused_and_unchanged() {
    Zone z = parse("<+03>-3");
    const char* bad[] = {"", "+3", "<+03", "CET-1CEST,M13.5.0,M10.5.0", "ABC25", "CET", "CET-1CEST,M3.5.0",
                         "AB-1"};
    for (const char* s : bad) {
        TEST_ASSERT_FALSE_MESSAGE(parse_zone(s, z), s);
        TEST_ASSERT_EQUAL_INT32(3 * kH, z.std_offset_s);
        TEST_ASSERT_FALSE(z.has_dst);
    }
    TEST_ASSERT_FALSE(parse_zone(nullptr, z));
}

void run_zone_tests() {
    RUN_TEST(test_turkiye_default);
    RUN_TEST(test_utc_and_offsets_with_minutes);
    RUN_TEST(test_cet_transitions);
    RUN_TEST(test_est_transitions);
    RUN_TEST(test_julian_rules);
    RUN_TEST(test_invalid_refused_and_unchanged);
}
