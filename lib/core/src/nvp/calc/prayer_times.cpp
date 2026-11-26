// Port of adhan-js v4.4.6 PrayerTimes.ts and its parameter files (MIT, Copyright (c) 2016 Batoul
// Apps; LICENSE-adhan.txt). JavaScript Date arithmetic is reproduced in integer milliseconds:
// TimeComponents floors hours, minutes and seconds; adding seconds truncates to whole milliseconds
// (TimeClip); roundedMinute looks at the UTC seconds only.
#include "nvp/calc/prayer_times.h"

#include <cmath>

namespace nvp::calc {
namespace {

constexpr int64_t kDayMs = 86400LL * 1000;

// new TimeComponents(hours).utcDate(y, m, d); false for NaN.
bool utc_ms(double hours, engine::Date d, int64_t& out) {
    if (std::isnan(hours) || std::isinf(hours)) return false;
    const double h = std::floor(hours);
    const double m = std::floor((hours - h) * 60);
    const double s = std::floor((hours - (h + m / 60)) * 60 * 60);
    out = static_cast<int64_t>(engine::to_day_number(d)) * kDayMs + static_cast<int64_t>(h) * 3600000 +
          static_cast<int64_t>(m) * 60000 + static_cast<int64_t>(s) * 1000;
    return true;
}

int64_t add_seconds(int64_t ms, double seconds) { return ms + static_cast<int64_t>(std::trunc(seconds * 1000)); }

int64_t rounded_minute(int64_t ms, Rounding r) {
    int64_t secs = (ms / 1000) % 60;
    if (secs < 0) secs += 60;
    int64_t offset = secs >= 30 ? 60 - secs : -secs;
    if (r == Rounding::Up) offset = 60 - secs;
    if (r == Rounding::None) offset = 0;
    return ms + offset * 1000;
}

void night_portions(const Params& p, double& fajr, double& isha) {
    switch (p.high_latitude) {
        case HighLatitudeRule::MiddleOfTheNight: fajr = 0.5; isha = 0.5; return;
        case HighLatitudeRule::SeventhOfTheNight: fajr = 1.0 / 7; isha = 1.0 / 7; return;
        case HighLatitudeRule::TwilightAngle: fajr = p.fajr_angle / 60; isha = p.isha_angle / 60; return;
    }
}

Params make(double fajr, double isha, int interval = 0, double maghrib = 0) {
    Params p;
    p.fajr_angle = fajr;
    p.isha_angle = isha;
    p.isha_interval = interval;
    p.maghrib_angle = maghrib;
    return p;
}

}  // namespace

Params turkey() {
    Params p = make(18, 17);
    p.adjust[1] = -7;
    p.adjust[2] = 5;
    p.adjust[3] = 4;
    p.adjust[4] = 7;
    return p;
}
Params qatar() { return make(18, 0, 90); }
Params dubai() {
    Params p = make(18.2, 18.2);
    p.adjust[1] = -3;
    p.adjust[2] = 3;
    p.adjust[3] = 3;
    p.adjust[4] = 3;
    return p;
}
Params kuwait() { return make(18, 17.5); }
Params umm_al_qura() { return make(18.5, 0, 90); }
Params singapore() {
    Params p = make(20, 18);
    p.adjust[2] = 1;
    p.rounding = Rounding::Up;
    return p;
}
Params tehran() { return make(17.7, 14, 0, 4.5); }

bool prayer_times_utc_ms(const Coordinates& c, engine::Date date, const Params& p, int64_t out[6]) {
    if (!engine::valid(date)) return false;
    const SolarTime solar(date.year, date.month, date.day, c);
    const engine::Date tomorrow = engine::to_date(engine::to_day_number(date) + 1);
    const SolarTime tomorrow_solar(tomorrow.year, tomorrow.month, tomorrow.day, c);

    int64_t dhuhr, sunrise, sunset, asr, tomorrow_sunrise;
    if (!utc_ms(solar.transit, date, dhuhr) || !utc_ms(solar.sunrise, date, sunrise) ||
        !utc_ms(solar.sunset, date, sunset) || !utc_ms(solar.afternoon(p.shadow_length), date, asr) ||
        !utc_ms(tomorrow_solar.sunrise, tomorrow, tomorrow_sunrise))
        return false;
    const double night = static_cast<double>(tomorrow_sunrise - sunset) / 1000;

    double fajr_portion = 0, isha_portion = 0;
    night_portions(p, fajr_portion, isha_portion);

    int64_t fajr = 0;
    const bool fajr_ok = utc_ms(solar.hour_angle(-1 * p.fajr_angle, false), date, fajr);
    const int64_t safe_fajr = add_seconds(sunrise, -(fajr_portion * night));
    if (!fajr_ok || safe_fajr > fajr) fajr = safe_fajr;

    int64_t isha = 0;
    if (p.isha_interval > 0) {
        isha = add_seconds(sunset, p.isha_interval * 60.0);
    } else {
        const bool isha_ok = utc_ms(solar.hour_angle(-1 * p.isha_angle, true), date, isha);
        const int64_t safe_isha = add_seconds(sunset, isha_portion * night);
        if (!isha_ok || safe_isha < isha) isha = safe_isha;
    }

    int64_t maghrib = sunset;
    if (p.maghrib_angle > 0) {
        int64_t angle_based = 0;
        if (utc_ms(solar.hour_angle(-1 * p.maghrib_angle, true), date, angle_based) && sunset < angle_based &&
            isha > angle_based)
            maghrib = angle_based;
    }

    const int64_t raw[6] = {fajr, sunrise, dhuhr, asr, maghrib, isha};
    for (int i = 0; i < 6; ++i) out[i] = rounded_minute(add_seconds(raw[i], p.adjust[i] * 60.0), p.rounding);
    return true;
}

bool prayer_times_local(const Coordinates& c, engine::Date date, const Params& p, int32_t utc_offset_s,
                        uint16_t out[6]) {
    int64_t ms[6];
    if (!prayer_times_utc_ms(c, date, p, ms)) return false;
    const int64_t day_start = static_cast<int64_t>(engine::to_day_number(date)) * kDayMs;
    for (int i = 0; i < 6; ++i) {
        const int64_t local = ms[i] + static_cast<int64_t>(utc_offset_s) * 1000 - day_start;
        if (local < 0 || local >= kDayMs) return false;
        out[i] = static_cast<uint16_t>(local / 60000);
    }
    return true;
}

}  // namespace nvp::calc
