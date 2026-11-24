// Encoding of the board's real-time clock, BM8563 (U4 of the V1.4 schematic; PCF8563-compatible),
// registers 02h..08h: VL_seconds, Minutes, Hours, Days, Weekdays, Century_months, Years, in BCD
// (NXP PCF8563 data sheet Rev. 11.1, Tables 4 and 8 to 17). Spec 003 FR-026, FR-027, research R1.
//
// The clock holds UTC, 2000..2099 (century bit C = 0). A reading is valid only if the
// "clock integrity is not guaranteed" bit VL is clear, C is clear, every BCD digit and field is in
// range, and the time is not earlier than the firmware's build time (TIME-03).
#pragma once

#include <cstdint>

namespace nvp::time {

enum class RtcInvalid : uint8_t { None, NoAnswer, IntegrityLost, Century, BadField, BeforeBuild };

struct RtcReading {
    bool valid = false;
    RtcInvalid reason = RtcInvalid::NoAnswer;
    int64_t utc = 0;  // seconds since 1970-01-01 00:00:00 UTC; meaningful only when valid
};

inline constexpr uint8_t kRtcAddress = 0x51;    // data sheet: write A2h, read A3h
inline constexpr uint8_t kRtcFirstRegister = 0x02;
inline constexpr uint8_t kRtcTimeBytes = 7;     // 02h..08h, read or written in one access (§8.5)

RtcReading decode(const uint8_t raw[kRtcTimeBytes], int64_t build_utc);

// Writes the seven registers for `utc` (VL = 0, C = 0, weekday from the date, Sunday = 0 as in
// Table 14). Returns false for times outside 2000-01-01 .. 2099-12-31.
bool encode(int64_t utc, uint8_t raw[kRtcTimeBytes]);

const char* reason_name(RtcInvalid r);  // "integrity lost", "no answer", ... for the log and report

}  // namespace nvp::time
