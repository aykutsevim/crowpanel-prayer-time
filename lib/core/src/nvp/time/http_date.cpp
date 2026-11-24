#include "nvp/time/http_date.h"

#include <cstring>

#include "nvp/engine/calendar.h"

namespace nvp::time {
namespace {

int two(const char* p) {
    if (p[0] < '0' || p[0] > '9' || p[1] < '0' || p[1] > '9') return -1;
    return (p[0] - '0') * 10 + (p[1] - '0');
}

}  // namespace

bool parse_http_date(const char* s, int64_t* utc_s) {
    // "Tue, 24 Nov 2026 11:41:17 GMT" is exactly 29 characters.
    if (s == nullptr || utc_s == nullptr || std::strlen(s) != 29) return false;
    static const char* kDays[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};  // engine weekday order
    static const char* kMonths[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    if (s[3] != ',' || s[4] != ' ' || s[7] != ' ' || s[11] != ' ' || s[16] != ' ' || s[19] != ':' ||
        s[22] != ':' || s[25] != ' ' || std::strcmp(s + 26, "GMT") != 0) {
        return false;
    }
    int month = 0;
    for (int i = 0; i < 12; ++i) {
        if (std::strncmp(s + 8, kMonths[i], 3) == 0) month = i + 1;
    }
    const int day = two(s + 5);
    const int y_hi = two(s + 12), y_lo = two(s + 14);
    const int hour = two(s + 17), minute = two(s + 20), second = two(s + 23);
    if (month == 0 || day < 0 || y_hi < 0 || y_lo < 0 || hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
        second < 0 || second > 60) {
        return false;
    }
    const engine::Date d{static_cast<int16_t>(y_hi * 100 + y_lo), static_cast<uint8_t>(month),
                         static_cast<uint8_t>(day)};
    if (d.year < 1970 || !engine::valid(d)) return false;
    const engine::DayNumber n = engine::to_day_number(d);
    if (std::strncmp(s, kDays[engine::weekday(n)], 3) != 0) return false;
    *utc_s = static_cast<int64_t>(n) * 86400 + hour * 3600 + minute * 60 + (second == 60 ? 59 : second);
    return true;
}

}  // namespace nvp::time
