// BM8563 / PCF8563 time registers 02h..08h (spec 003 FR-026, FR-027, research R1).
// Register facts: NXP PCF8563 data sheet Rev. 11.1, Tables 8 to 17. Written before the code.
#include <unity.h>

#include "nvp/engine/calendar.h"
#include "nvp/time/rtc_codec.h"

using namespace nvp::time;
using nvp::engine::Date;
using nvp::engine::to_day_number;

static int64_t utc(int y, int mo, int d, int h, int mi, int s) {
    return static_cast<int64_t>(to_day_number(Date{static_cast<int16_t>(y), static_cast<uint8_t>(mo),
                                                   static_cast<uint8_t>(d)})) * 86400 + h * 3600 + mi * 60 + s;
}

static const int64_t kBuild = utc(2026, 11, 24, 0, 0, 0);

static void test_decode_valid() {
    // 2026-11-24 09:30:05 UTC, a Tuesday (weekday register 2, not used): BCD.
    const uint8_t raw[7] = {0x05, 0x30, 0x09, 0x24, 0x02, 0x11, 0x26};
    const RtcReading r = decode(raw, kBuild);
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(RtcInvalid::None), static_cast<int>(r.reason));
    TEST_ASSERT_TRUE(r.utc == utc(2026, 11, 24, 9, 30, 5));
}

static void expect_reason(const uint8_t raw[7], RtcInvalid reason, int64_t build = kBuild) {
    const RtcReading r = decode(raw, build);
    TEST_ASSERT_FALSE(r.valid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(reason), static_cast<int>(r.reason));
}

static void test_integrity_lost() {  // VL = bit 7 of 02h (Table 8)
    const uint8_t raw[7] = {0x85, 0x30, 0x09, 0x24, 0x02, 0x11, 0x26};
    expect_reason(raw, RtcInvalid::IntegrityLost);
}

static void test_century_bit() {  // C = bit 7 of 07h (Table 15); 20xx only
    const uint8_t raw[7] = {0x05, 0x30, 0x09, 0x24, 0x02, 0x91, 0x26};
    expect_reason(raw, RtcInvalid::Century);
}

static void test_bad_fields() {
    const uint8_t digit[7] = {0x0A, 0x30, 0x09, 0x24, 0x02, 0x11, 0x26};    // BCD digit > 9
    const uint8_t month[7] = {0x05, 0x30, 0x09, 0x24, 0x02, 0x13, 0x26};    // month 13
    const uint8_t day[7] = {0x05, 0x30, 0x09, 0x32, 0x02, 0x11, 0x26};      // day 32
    const uint8_t feb29[7] = {0x05, 0x30, 0x09, 0x29, 0x02, 0x02, 0x26};    // 2026-02-29
    const uint8_t hour[7] = {0x05, 0x30, 0x24, 0x24, 0x02, 0x11, 0x26};     // hour 24
    const uint8_t minute[7] = {0x05, 0x60, 0x09, 0x24, 0x02, 0x11, 0x26};   // minute 60
    expect_reason(digit, RtcInvalid::BadField);
    expect_reason(month, RtcInvalid::BadField);
    expect_reason(day, RtcInvalid::BadField);
    expect_reason(feb29, RtcInvalid::BadField);
    expect_reason(hour, RtcInvalid::BadField);
    expect_reason(minute, RtcInvalid::BadField);
}

static void test_unused_bits_are_masked() {
    // Unused bits (minutes bit 7, hours bits 7-6, days bits 7-6, months bits 6-5) do not matter.
    const uint8_t raw[7] = {0x05, 0xB0, 0xC9, 0xE4, 0xFA, 0x71, 0x26};
    const RtcReading r = decode(raw, kBuild);
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_TRUE(r.utc == utc(2026, 11, 24, 9, 30, 5));
}

static void test_before_build() {  // TIME-03
    const uint8_t raw[7] = {0x59, 0x59, 0x23, 0x23, 0x01, 0x11, 0x26};  // 2026-11-23 23:59:59
    expect_reason(raw, RtcInvalid::BeforeBuild);
}

static void test_round_trip_every_day() {
    uint8_t raw[7];
    for (int64_t day = utc(2000, 1, 1, 12, 0, 0); day <= utc(2099, 12, 31, 12, 0, 0); day += 86400) {
        encode(day + 3725, raw);
        const RtcReading r = decode(raw, 0);
        if (!r.valid || r.utc != day + 3725) TEST_FAIL_MESSAGE("round trip failed");
    }
}

static void test_encode_bits() {
    uint8_t raw[7];
    encode(utc(2026, 11, 24, 9, 30, 5), raw);
    TEST_ASSERT_EQUAL_HEX8(0x05, raw[0]);  // VL cleared
    TEST_ASSERT_EQUAL_HEX8(0x30, raw[1]);
    TEST_ASSERT_EQUAL_HEX8(0x09, raw[2]);
    TEST_ASSERT_EQUAL_HEX8(0x24, raw[3]);
    TEST_ASSERT_EQUAL_HEX8(0x02, raw[4]);  // Tuesday: weekday 0 = Sunday (Table 14)
    TEST_ASSERT_EQUAL_HEX8(0x11, raw[5]);  // C cleared
    TEST_ASSERT_EQUAL_HEX8(0x26, raw[6]);
    TEST_ASSERT_FALSE(encode(utc(2100, 1, 1, 0, 0, 0), raw));
    TEST_ASSERT_FALSE(encode(utc(1999, 12, 31, 0, 0, 0), raw));
}

static void test_reason_names() {
    TEST_ASSERT_EQUAL_STRING("integrity lost", reason_name(RtcInvalid::IntegrityLost));
    TEST_ASSERT_EQUAL_STRING("no answer", reason_name(RtcInvalid::NoAnswer));
    TEST_ASSERT_EQUAL_STRING("before build", reason_name(RtcInvalid::BeforeBuild));
}

void run_rtc_codec_tests() {
    RUN_TEST(test_decode_valid);
    RUN_TEST(test_integrity_lost);
    RUN_TEST(test_century_bit);
    RUN_TEST(test_bad_fields);
    RUN_TEST(test_unused_bits_are_masked);
    RUN_TEST(test_before_build);
    RUN_TEST(test_round_trip_every_day);
    RUN_TEST(test_encode_bits);
    RUN_TEST(test_reason_names);
}
