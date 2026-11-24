// Calendar arithmetic for the prayer-time engine (spec 002, research R1).
//
// A date becomes a day number (days since 1970-01-01); a moment becomes one signed 64-bit count
// of local seconds, day_number * 86400 + second_of_day. Midnight, month end and year end are then
// plain integer arithmetic. Proleptic Gregorian calendar, local civil time only: the engine never
// reads a clock or a time zone (FR-019).
#pragma once

#include <cstdint>

namespace nvp::engine {

struct Date {
    int16_t year = 1970;
    uint8_t month = 1;  // 1..12
    uint8_t day = 1;    // 1..31
};

inline bool operator==(const Date& a, const Date& b) {
    return a.year == b.year && a.month == b.month && a.day == b.day;
}
inline bool operator!=(const Date& a, const Date& b) { return !(a == b); }

using DayNumber = int32_t;     // days since 1970-01-01
using LocalSeconds = int64_t;  // seconds since 1970-01-01 00:00:00 local civil time

inline constexpr int32_t kSecondsPerDay = 86400;

// The current local time, given by the caller.
struct Moment {
    Date date;
    int32_t second_of_day = 0;  // 0..86399
};

bool is_leap_year(int32_t year);
uint8_t days_in_month(int32_t year, uint8_t month);  // 0 for an invalid month
bool valid(const Date& d);

DayNumber to_day_number(const Date& d);  // valid dates only
Date to_date(DayNumber n);

// 0 = Monday ... 6 = Sunday.
uint8_t weekday(DayNumber n);

// The local second at which minute `minutes` (0..1439) of day `n` begins.
inline LocalSeconds at(DayNumber n, uint16_t minutes) {
    return static_cast<LocalSeconds>(n) * kSecondsPerDay + static_cast<LocalSeconds>(minutes) * 60;
}

}  // namespace nvp::engine
