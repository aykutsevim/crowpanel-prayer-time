// The six prayer times for a date and a place: a C++17 port of adhan-js v4.4.6 `PrayerTimes.ts`
// with `CalculationParameters.ts`, `CalculationMethod.ts`, `Madhab.ts`, `HighLatitudeRule.ts`,
// `TimeComponents.ts`, `DateUtils.ts` and `Rounding.ts` (MIT, Batoul Apps; LICENSE-adhan.txt;
// spec 007 research R3, FR-017, 04-data-sources §3.5).
//
// Not ported: the Moonsighting Committee special cases, polar-circle resolution, Qibla, Sunnah
// times. Pure C++, no heap, no exceptions. Times come out as UTC milliseconds since 1970, like the
// JavaScript Date values of the original, and as minutes after local midnight for a UTC offset.
#pragma once
#include <cstdint>

#include "nvp/calc/astronomical.h"
#include "nvp/engine/calendar.h"

namespace nvp::calc {

enum class HighLatitudeRule : uint8_t { MiddleOfTheNight, SeventhOfTheNight, TwilightAngle };
enum class Rounding : uint8_t { Nearest, Up, None };

struct Params {
    double fajr_angle = 0;
    double isha_angle = 0;
    int isha_interval = 0;     // minutes after Maghrib; replaces the Isha angle when > 0
    double maghrib_angle = 0;  // Tehran only
    double shadow_length = 1;  // 1 = Shafi (standard Asr), 2 = Hanafi
    HighLatitudeRule high_latitude = HighLatitudeRule::MiddleOfTheNight;
    Rounding rounding = Rounding::Nearest;
    // Method adjustments in minutes: fajr, sunrise, dhuhr, asr, maghrib, isha.
    int adjust[6] = {};
};

// Diyanet ("Turkey" in adhan): Fajr 18°, Isha 17°, sunrise -7, Dhuhr +5, Asr +4, Maghrib +7.
Params turkey();
// The other methods of the shared adhan test data (used by the tests only).
Params qatar();
Params dubai();
Params kuwait();
Params umm_al_qura();
Params singapore();
Params tehran();

// Fajr, sunrise, Dhuhr, Asr, Maghrib, Isha as UTC milliseconds. False when a time does not exist.
bool prayer_times_utc_ms(const Coordinates& c, engine::Date date, const Params& p, int64_t out[6]);

// The same as minutes after local midnight of `date` for a UTC offset in seconds (Türkiye: 10800).
// False when a time does not exist or falls outside the day.
bool prayer_times_local(const Coordinates& c, engine::Date date, const Params& p, int32_t utc_offset_s,
                        uint16_t out[6]);

}  // namespace nvp::calc
