// POSIX TZ strings: the stored time-zone setting (TIME-04; spec 005 FR-012, research R5).
// "std offset [dst [offset] [,start[/time],end[/time]]]", e.g. "<+03>-3" (Türkiye),
// "CET-1CEST,M3.5.0,M10.5.0/3", "EST5EDT,M3.2.0,M11.1.0". Pure C++; the same conversion runs on
// the device and in the host tests (newlib's tzset is not used).
#pragma once

#include <cstdint>

namespace nvp::time {

inline constexpr const char* kDefaultZone = "<+03>-3";

struct ZoneRule {
    enum class Kind : uint8_t { MonthWeekDay, Julian1, Julian0 };  // "Mm.w.d", "Jn" (no Feb 29), "n"
    Kind kind = Kind::MonthWeekDay;
    uint8_t month = 0, week = 0, weekday = 0;  // weekday 0 = Sunday (POSIX)
    uint16_t day = 0;
    int32_t time_s = 7200;  // local time of the change, default 02:00
};

struct Zone {
    int32_t std_offset_s = 3 * 3600;  // seconds east of UTC (POSIX writes the opposite sign)
    bool has_dst = false;
    int32_t dst_offset_s = 0;
    ZoneRule start, end;
};

// True and `out` filled for a valid string; false and `out` untouched otherwise. A dst part needs
// both rules (zones without rules are refused rather than guessed).
bool parse_zone(const char* s, Zone& out);

Zone default_zone();  // kDefaultZone

int64_t utc_to_local(const Zone& z, int64_t utc_s);
// The UTC second of a local second (for local times in a DST gap or overlap, the standard-time reading).
int64_t local_to_utc(const Zone& z, int64_t local_s);

}  // namespace nvp::time
