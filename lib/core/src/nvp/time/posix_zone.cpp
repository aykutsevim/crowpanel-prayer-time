#include "nvp/time/posix_zone.h"

#include "nvp/engine/calendar.h"

namespace nvp::time {
namespace {

using engine::DayNumber;

bool is_alpha(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
bool is_digit(char c) { return c >= '0' && c <= '9'; }

// std/dst name: three or more letters, or <...> with letters, digits, '+' and '-'.
bool name(const char*& p) {
    if (*p == '<') {
        const char* q = p + 1;
        int n = 0;
        while (*q != '\0' && *q != '>') {
            if (!is_alpha(*q) && !is_digit(*q) && *q != '+' && *q != '-') return false;
            ++q;
            ++n;
        }
        if (*q != '>' || n < 3) return false;
        p = q + 1;
        return true;
    }
    int n = 0;
    while (is_alpha(p[n])) ++n;
    if (n < 3) return false;
    p += n;
    return true;
}

bool number(const char*& p, int max_digits, int* out) {
    int v = 0, n = 0;
    while (is_digit(*p) && n < max_digits) {
        v = v * 10 + (*p - '0');
        ++p;
        ++n;
    }
    if (n == 0) return false;
    *out = v;
    return true;
}

// [+-]hh[:mm[:ss]], hours 0..24 (a rule time may go to 167). Returns seconds.
bool hms(const char*& p, int max_hours, int32_t* out, bool allow_sign) {
    int sign = 1;
    if (allow_sign && (*p == '+' || *p == '-')) {
        sign = *p == '-' ? -1 : 1;
        ++p;
    }
    int h = 0, m = 0, s = 0;
    if (!number(p, 3, &h) || h > max_hours) return false;
    if (*p == ':') {
        ++p;
        if (!number(p, 2, &m) || m > 59) return false;
        if (*p == ':') {
            ++p;
            if (!number(p, 2, &s) || s > 59) return false;
        }
    }
    *out = sign * (h * 3600 + m * 60 + s);
    return true;
}

bool rule(const char*& p, ZoneRule& r) {
    int a = 0, b = 0, c = 0;
    if (*p == 'M') {
        ++p;
        if (!number(p, 2, &a) || *p++ != '.' || !number(p, 1, &b) || *p++ != '.' || !number(p, 1, &c)) return false;
        if (a < 1 || a > 12 || b < 1 || b > 5 || c > 6) return false;
        r.kind = ZoneRule::Kind::MonthWeekDay;
        r.month = static_cast<uint8_t>(a);
        r.week = static_cast<uint8_t>(b);
        r.weekday = static_cast<uint8_t>(c);
    } else if (*p == 'J') {
        ++p;
        if (!number(p, 3, &a) || a < 1 || a > 365) return false;
        r.kind = ZoneRule::Kind::Julian1;
        r.day = static_cast<uint16_t>(a);
    } else {
        if (!number(p, 3, &a) || a > 365) return false;
        r.kind = ZoneRule::Kind::Julian0;
        r.day = static_cast<uint16_t>(a);
    }
    r.time_s = 7200;
    if (*p == '/') {
        ++p;
        if (!hms(p, 167, &r.time_s, true)) return false;
    }
    return true;
}

// Day number of the rule's date in `year`.
DayNumber rule_day(const ZoneRule& r, int32_t year) {
    const DayNumber jan1 = engine::to_day_number(engine::Date{static_cast<int16_t>(year), 1, 1});
    switch (r.kind) {
        case ZoneRule::Kind::Julian1: {
            // 1..365, February 29 never counted: after February in a leap year, add one.
            int d = r.day;
            if (engine::is_leap_year(year) && d >= 60) ++d;
            return jan1 + d - 1;
        }
        case ZoneRule::Kind::Julian0:
            return jan1 + r.day;
        case ZoneRule::Kind::MonthWeekDay: {
            const DayNumber first = engine::to_day_number(engine::Date{static_cast<int16_t>(year), r.month, 1});
            const int first_wd = (engine::weekday(first) + 1) % 7;  // engine: 0 = Monday; POSIX: 0 = Sunday
            int day = 1 + (r.weekday - first_wd + 7) % 7 + (r.week - 1) * 7;
            const int dim = engine::days_in_month(year, r.month);
            while (day > dim) day -= 7;  // week 5: the last such weekday
            return first + day - 1;
        }
    }
    return jan1;
}

int32_t year_of(int64_t seconds) {
    const int64_t days = seconds >= 0 ? seconds / 86400 : -((-seconds + 86399) / 86400);
    return engine::to_date(static_cast<DayNumber>(days)).year;
}

bool in_dst(const Zone& z, int64_t utc_s) {
    const int32_t year = year_of(utc_s + z.std_offset_s);
    // Start is given in standard local time, end in daylight local time.
    const int64_t start = static_cast<int64_t>(rule_day(z.start, year)) * 86400 + z.start.time_s - z.std_offset_s;
    const int64_t end = static_cast<int64_t>(rule_day(z.end, year)) * 86400 + z.end.time_s - z.dst_offset_s;
    if (start < end) return utc_s >= start && utc_s < end;  // northern hemisphere
    return utc_s >= start || utc_s < end;                   // southern hemisphere
}

}  // namespace

bool parse_zone(const char* s, Zone& out) {
    if (s == nullptr) return false;
    Zone z;
    const char* p = s;
    int32_t off = 0;
    if (!name(p) || !hms(p, 24, &off, true)) return false;
    z.std_offset_s = -off;
    z.has_dst = false;
    if (*p != '\0') {
        if (!name(p)) return false;
        z.has_dst = true;
        z.dst_offset_s = z.std_offset_s + 3600;
        if (*p != ',' && *p != '\0') {
            if (!hms(p, 24, &off, true)) return false;
            z.dst_offset_s = -off;
        }
        if (*p++ != ',' || !rule(p, z.start) || *p++ != ',' || !rule(p, z.end) || *p != '\0') return false;
    }
    out = z;
    return true;
}

Zone default_zone() {
    Zone z;
    parse_zone(kDefaultZone, z);
    return z;
}

int64_t utc_to_local(const Zone& z, int64_t utc_s) {
    if (z.has_dst && in_dst(z, utc_s)) return utc_s + z.dst_offset_s;
    return utc_s + z.std_offset_s;
}

int64_t local_to_utc(const Zone& z, int64_t local_s) {
    if (z.has_dst) {
        const int64_t as_dst = local_s - z.dst_offset_s;
        if (in_dst(z, as_dst)) return as_dst;
    }
    return local_s - z.std_offset_s;
}

}  // namespace nvp::time
