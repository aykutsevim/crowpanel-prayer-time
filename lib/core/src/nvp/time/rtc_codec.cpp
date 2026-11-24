#include "nvp/time/rtc_codec.h"

#include "nvp/engine/calendar.h"

namespace nvp::time {
namespace {

using engine::Date;

// BCD byte -> value, or -1 if a digit is above 9.
int bcd(uint8_t b) {
    const int hi = b >> 4;
    const int lo = b & 0x0F;
    return (hi > 9 || lo > 9) ? -1 : hi * 10 + lo;
}

uint8_t to_bcd(int v) { return static_cast<uint8_t>(((v / 10) << 4) | (v % 10)); }

int64_t floor_div(int64_t a, int64_t b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }

}  // namespace

RtcReading decode(const uint8_t raw[kRtcTimeBytes], int64_t build_utc) {
    RtcReading r;
    if ((raw[0] & 0x80) != 0) {
        r.reason = RtcInvalid::IntegrityLost;
        return r;
    }
    if ((raw[5] & 0x80) != 0) {
        r.reason = RtcInvalid::Century;
        return r;
    }
    // Masks from the register tables: unused bits are ignored.
    const int sec = bcd(raw[0] & 0x7F);
    const int min = bcd(raw[1] & 0x7F);
    const int hour = bcd(raw[2] & 0x3F);
    const int day = bcd(raw[3] & 0x3F);
    const int month = bcd(raw[5] & 0x1F);
    const int year = bcd(raw[6]);
    if (sec < 0 || sec > 59 || min < 0 || min > 59 || hour < 0 || hour > 23 || day < 1 || month < 1 ||
        month > 12 || year < 0) {
        r.reason = RtcInvalid::BadField;
        return r;
    }
    const Date d{static_cast<int16_t>(2000 + year), static_cast<uint8_t>(month), static_cast<uint8_t>(day)};
    if (!engine::valid(d)) {
        r.reason = RtcInvalid::BadField;
        return r;
    }
    const int64_t t = static_cast<int64_t>(engine::to_day_number(d)) * engine::kSecondsPerDay + hour * 3600 +
                      min * 60 + sec;
    if (t < build_utc) {
        r.reason = RtcInvalid::BeforeBuild;
        return r;
    }
    r.valid = true;
    r.reason = RtcInvalid::None;
    r.utc = t;
    return r;
}

bool encode(int64_t utc, uint8_t raw[kRtcTimeBytes]) {
    const int64_t days = floor_div(utc, engine::kSecondsPerDay);
    const int64_t sod = utc - days * engine::kSecondsPerDay;
    const Date d = engine::to_date(static_cast<engine::DayNumber>(days));
    if (d.year < 2000 || d.year > 2099) return false;
    raw[0] = to_bcd(static_cast<int>(sod % 60));             // VL = 0
    raw[1] = to_bcd(static_cast<int>((sod / 60) % 60));
    raw[2] = to_bcd(static_cast<int>(sod / 3600));
    raw[3] = to_bcd(d.day);
    raw[4] = static_cast<uint8_t>((engine::weekday(static_cast<engine::DayNumber>(days)) + 1) % 7);  // Sunday = 0
    raw[5] = to_bcd(d.month);                                // C = 0
    raw[6] = to_bcd(d.year - 2000);
    return true;
}

const char* reason_name(RtcInvalid r) {
    switch (r) {
        case RtcInvalid::None: return "valid";
        case RtcInvalid::NoAnswer: return "no answer";
        case RtcInvalid::IntegrityLost: return "integrity lost";
        case RtcInvalid::Century: return "century";
        case RtcInvalid::BadField: return "bad field";
        case RtcInvalid::BeforeBuild: return "before build";
    }
    return "?";
}

}  // namespace nvp::time
