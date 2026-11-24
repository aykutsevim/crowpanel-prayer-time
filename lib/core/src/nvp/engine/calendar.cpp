#include "nvp/engine/calendar.h"

namespace nvp::engine {

bool is_leap_year(int32_t y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

uint8_t days_in_month(int32_t year, uint8_t month) {
    static const uint8_t kDays[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 0;
    if (month == 2 && is_leap_year(year)) return 29;
    return kDays[month];
}

bool valid(const Date& d) { return d.day >= 1 && d.day <= days_in_month(d.year, d.month); }

// days_from_civil and civil_from_days: Howard Hinnant, "chrono-Compatible Low-Level Date
// Algorithms" (public domain), howardhinnant.github.io/date_algorithms.html. Integer only, exact
// for the proleptic Gregorian calendar.
DayNumber to_day_number(const Date& date) {
    const int32_t m = date.month;
    const int32_t d = date.day;
    const int32_t y = static_cast<int32_t>(date.year) - (m <= 2 ? 1 : 0);
    const int32_t era = (y >= 0 ? y : y - 399) / 400;
    const int32_t yoe = y - era * 400;                                    // [0, 399]
    const int32_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;  // [0, 365]
    const int32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;            // [0, 146096]
    return era * 146097 + doe - 719468;
}

Date to_date(DayNumber n) {
    const int32_t z = n + 719468;
    const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
    const int32_t doe = z - era * 146097;                                   // [0, 146096]
    const int32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;  // [0, 399]
    const int32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);            // [0, 365]
    const int32_t mp = (5 * doy + 2) / 153;                                 // [0, 11]
    const int32_t d = doy - (153 * mp + 2) / 5 + 1;                         // [1, 31]
    const int32_t m = mp < 10 ? mp + 3 : mp - 9;                            // [1, 12]
    const int32_t y = yoe + era * 400 + (m <= 2 ? 1 : 0);
    Date out;
    out.year = static_cast<int16_t>(y);
    out.month = static_cast<uint8_t>(m);
    out.day = static_cast<uint8_t>(d);
    return out;
}

uint8_t weekday(DayNumber n) {
    // 1970-01-01 (day 0) is a Thursday = 3 when Monday = 0.
    const int32_t r = (n + 3) % 7;
    return static_cast<uint8_t>(r < 0 ? r + 7 : r);
}

}  // namespace nvp::engine
