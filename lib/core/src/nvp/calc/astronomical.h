// Solar position and transit/hour-angle maths: a C++17 port of adhan-js v4.4.6
// (`src/Astronomical.ts`, `SolarCoordinates.ts`, `SolarTime.ts`, `MathUtils.ts`; MIT, Batoul Apps,
// see LICENSE-adhan.txt; spec 007 research R3). Equations from Meeus, Astronomical Algorithms.
// Pure C++, no heap, no exceptions. Angles in degrees; times in hours of the UTC day.
#pragma once
#include <cstdint>

namespace nvp::calc {

struct Coordinates {
    double latitude = 0;
    double longitude = 0;
};

namespace astro {

double unwind_angle(double angle);
double quadrant_shift_angle(double angle);
double julian_day(int year, int month, int day, double hours = 0);
double julian_century(double julian_day);
double interpolate(double y2, double y1, double y3, double n);
double interpolate_angles(double y2, double y1, double y3, double n);
double approximate_transit(double longitude, double sidereal_time, double right_ascension);
double corrected_transit(double m0, double longitude, double sidereal_time, double ra, double prev_ra, double next_ra);
double corrected_hour_angle(double m0, double angle, const Coordinates& c, bool after_transit, double sidereal_time,
                            double ra, double prev_ra, double next_ra, double decl, double prev_decl, double next_decl);

}  // namespace astro

struct SolarCoordinates {
    double declination = 0;
    double right_ascension = 0;
    double apparent_sidereal_time = 0;
    explicit SolarCoordinates(double julian_day);
};

// SolarTime of adhan-js: transit, sunrise and sunset in hours of the UTC day (NaN when the sun
// does not reach the altitude).
class SolarTime {
public:
    SolarTime(int year, int month, int day, const Coordinates& c);
    double transit = 0;
    double sunrise = 0;
    double sunset = 0;
    double hour_angle(double angle, bool after_transit) const;
    double afternoon(double shadow_length) const;

private:
    Coordinates observer_;
    SolarCoordinates solar_;
    SolarCoordinates prev_;
    SolarCoordinates next_;
    double approx_transit_ = 0;
};

}  // namespace nvp::calc
