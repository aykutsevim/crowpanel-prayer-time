// Port of adhan-js v4.4.6 Astronomical.ts, SolarCoordinates.ts, SolarTime.ts, MathUtils.ts
// (MIT, Copyright (c) 2016 Batoul Apps; LICENSE-adhan.txt). Kept line for line with the original
// so that it can be compared; only the language changed.
#include "nvp/calc/astronomical.h"

#include <cmath>

namespace nvp::calc {
namespace {

constexpr double kPi = 3.14159265358979323846;
double rad(double d) { return d * kPi / 180.0; }
double deg(double r) { return r * 180.0 / kPi; }
double normalize_to_scale(double num, double max) { return num - max * std::floor(num / max); }

double mean_solar_longitude(double T) { return astro::unwind_angle(280.4664567 + 36000.76983 * T + 0.0003032 * T * T); }
double mean_lunar_longitude(double T) { return astro::unwind_angle(218.3165 + 481267.8813 * T); }
double ascending_lunar_node_longitude(double T) {
    return astro::unwind_angle(125.04452 - 1934.136261 * T + 0.0020708 * T * T + T * T * T / 450000);
}
double mean_solar_anomaly(double T) { return astro::unwind_angle(357.52911 + 35999.05029 * T - 0.0001537 * T * T); }
double solar_equation_of_the_center(double T, double M) {
    const double m = rad(M);
    return (1.914602 - 0.004817 * T - 0.000014 * T * T) * std::sin(m) + (0.019993 - 0.000101 * T) * std::sin(2 * m) +
           0.000289 * std::sin(3 * m);
}
double apparent_solar_longitude(double T, double L0) {
    const double longitude = L0 + solar_equation_of_the_center(T, mean_solar_anomaly(T));
    const double omega = 125.04 - 1934.136 * T;
    return astro::unwind_angle(longitude - 0.00569 - 0.00478 * std::sin(rad(omega)));
}
double mean_obliquity_of_the_ecliptic(double T) {
    return 23.439291 - 0.013004167 * T - 0.0000001639 * T * T + 0.0000005036 * T * T * T;
}
double apparent_obliquity_of_the_ecliptic(double T, double eps0) {
    return eps0 + 0.00256 * std::cos(rad(125.04 - 1934.136 * T));
}
double mean_sidereal_time(double T) {
    const double jd = T * 36525 + 2451545.0;
    return astro::unwind_angle(280.46061837 + 360.98564736629 * (jd - 2451545) + 0.000387933 * T * T -
                               T * T * T / 38710000);
}
double nutation_in_longitude(double L0, double Lp, double omega) {
    return (-17.2 / 3600) * std::sin(rad(omega)) - (1.32 / 3600) * std::sin(2 * rad(L0)) -
           (0.23 / 3600) * std::sin(2 * rad(Lp)) + (0.21 / 3600) * std::sin(2 * rad(omega));
}
double nutation_in_obliquity(double L0, double Lp, double omega) {
    return (9.2 / 3600) * std::cos(rad(omega)) + (0.57 / 3600) * std::cos(2 * rad(L0)) +
           (0.1 / 3600) * std::cos(2 * rad(Lp)) - (0.09 / 3600) * std::cos(2 * rad(omega));
}
double altitude_of_celestial_body(double phi, double delta, double H) {
    return deg(std::asin(std::sin(rad(phi)) * std::sin(rad(delta)) +
                         std::cos(rad(phi)) * std::cos(rad(delta)) * std::cos(rad(H))));
}

}  // namespace

namespace astro {

double unwind_angle(double angle) { return normalize_to_scale(angle, 360.0); }

double quadrant_shift_angle(double angle) {
    if (angle >= -180 && angle <= 180) return angle;
    return angle - 360 * std::round(angle / 360);
}

double julian_day(int year, int month, int day, double hours) {
    const int Y = month > 2 ? year : year - 1;
    const int M = month > 2 ? month : month + 12;
    const double D = day + hours / 24;
    const int A = Y / 100;
    const int B = 2 - A + A / 4;
    const int i0 = static_cast<int>(365.25 * (Y + 4716));
    const int i1 = static_cast<int>(30.6001 * (M + 1));
    return i0 + i1 + D + B - 1524.5;
}

double julian_century(double jd) { return (jd - 2451545.0) / 36525; }

double interpolate(double y2, double y1, double y3, double n) {
    const double a = y2 - y1, b = y3 - y2, c = b - a;
    return y2 + (n / 2) * (a + b + n * c);
}

double interpolate_angles(double y2, double y1, double y3, double n) {
    const double a = unwind_angle(y2 - y1), b = unwind_angle(y3 - y2), c = b - a;
    return y2 + (n / 2) * (a + b + n * c);
}

double approximate_transit(double L, double theta0, double a2) {
    const double Lw = L * -1;
    const double m0 = normalize_to_scale((a2 + Lw - theta0) / 360, 1);
    const double expected = normalize_to_scale((12.0 - L / 15.0) / 24.0, 1);
    if (m0 - expected > 0.5) return m0 - 1.0;
    if (expected - m0 > 0.5) return m0 + 1.0;
    return m0;
}

double corrected_transit(double m0, double L, double theta0, double a2, double a1, double a3) {
    const double Lw = L * -1;
    const double theta = unwind_angle(theta0 + 360.985647 * m0);
    const double a = unwind_angle(interpolate_angles(a2, a1, a3, m0));
    const double H = quadrant_shift_angle(theta - Lw - a);
    const double dm = H / -360;
    return (m0 + dm) * 24;
}

double corrected_hour_angle(double m0, double h0, const Coordinates& c, bool after_transit, double theta0, double a2,
                            double a1, double a3, double d2, double d1, double d3) {
    const double Lw = c.longitude * -1;
    const double term1 = std::sin(rad(h0)) - std::sin(rad(c.latitude)) * std::sin(rad(d2));
    const double term2 = std::cos(rad(c.latitude)) * std::cos(rad(d2));
    const double H0 = deg(std::acos(term1 / term2));
    const double m = after_transit ? m0 + H0 / 360 : m0 - H0 / 360;
    const double theta = unwind_angle(theta0 + 360.985647 * m);
    const double a = unwind_angle(interpolate_angles(a2, a1, a3, m));
    const double delta = interpolate(d2, d1, d3, m);
    const double H = theta - Lw - a;
    const double h = altitude_of_celestial_body(c.latitude, delta, H);
    const double term3 = h - h0;
    const double term4 = 360 * std::cos(rad(delta)) * std::cos(rad(c.latitude)) * std::sin(rad(H));
    const double dm = term3 / term4;
    return (m + dm) * 24;
}

}  // namespace astro

SolarCoordinates::SolarCoordinates(double julian_day) {
    const double T = astro::julian_century(julian_day);
    const double L0 = mean_solar_longitude(T);
    const double Lp = mean_lunar_longitude(T);
    const double omega = ascending_lunar_node_longitude(T);
    const double lambda = rad(apparent_solar_longitude(T, L0));
    const double theta0 = mean_sidereal_time(T);
    const double dpsi = nutation_in_longitude(L0, Lp, omega);
    const double deps = nutation_in_obliquity(L0, Lp, omega);
    const double eps0 = mean_obliquity_of_the_ecliptic(T);
    const double eps_app = rad(apparent_obliquity_of_the_ecliptic(T, eps0));
    declination = deg(std::asin(std::sin(eps_app) * std::sin(lambda)));
    right_ascension = astro::unwind_angle(deg(std::atan2(std::cos(eps_app) * std::sin(lambda), std::cos(lambda))));
    apparent_sidereal_time = theta0 + (dpsi * 3600 * std::cos(rad(eps0 + deps))) / 3600;
}

SolarTime::SolarTime(int year, int month, int day, const Coordinates& c)
    : observer_(c),
      solar_(astro::julian_day(year, month, day, 0)),
      prev_(astro::julian_day(year, month, day, 0) - 1),
      next_(astro::julian_day(year, month, day, 0) + 1) {
    const double m0 = astro::approximate_transit(c.longitude, solar_.apparent_sidereal_time, solar_.right_ascension);
    const double solar_altitude = -50.0 / 60.0;
    approx_transit_ = m0;
    transit = astro::corrected_transit(m0, c.longitude, solar_.apparent_sidereal_time, solar_.right_ascension,
                                       prev_.right_ascension, next_.right_ascension);
    sunrise = hour_angle(solar_altitude, false);
    sunset = hour_angle(solar_altitude, true);
}

double SolarTime::hour_angle(double angle, bool after_transit) const {
    return astro::corrected_hour_angle(approx_transit_, angle, observer_, after_transit, solar_.apparent_sidereal_time,
                                       solar_.right_ascension, prev_.right_ascension, next_.right_ascension,
                                       solar_.declination, prev_.declination, next_.declination);
}

double SolarTime::afternoon(double shadow_length) const {
    const double tangent = std::fabs(observer_.latitude - solar_.declination);
    const double inverse = shadow_length + std::tan(rad(tangent));
    const double angle = deg(std::atan(1.0 / inverse));
    return hour_angle(angle, true);
}

}  // namespace nvp::calc
