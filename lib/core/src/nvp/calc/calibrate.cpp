#include "nvp/calc/calibrate.h"

#include <algorithm>
#include <cmath>

#include "nvp/calc/prayer_times.h"

namespace nvp::calc {
namespace {

constexpr int32_t kJdnOf1970 = 2440588;  // Julian Day Number of 1970-01-01

int32_t jdn(engine::Date d) { return engine::to_day_number(d) + kJdnOf1970; }

// Tabular Islamic calendar, civil epoch (16 July 622 Julian).
engine::HijriDate from_jdn(int32_t jd) {
    int32_t l = jd - 1948440 + 10632;
    const int32_t n = (l - 1) / 10631;
    l = l - 10631 * n + 354;
    const int32_t j = ((10985 - l) / 5316) * ((50 * l) / 17719) + (l / 5670) * ((43 * l) / 15238);
    l = l - ((30 - j) / 15) * ((17719 * j) / 50) - (j / 16) * ((15238 * j) / 43) + 29;
    const int32_t m = (24 * l) / 709;
    const int32_t d = l - (709 * m) / 24;
    const int32_t y = 30 * n + j - 30;
    engine::HijriDate h;
    h.day = static_cast<uint8_t>(d);
    h.month = static_cast<uint8_t>(m);
    h.year = static_cast<uint16_t>(y);
    h.present = true;
    return h;
}

int32_t to_jdn(const engine::HijriDate& h) {
    const int32_t y = h.year, m = h.month, d = h.day;
    return d + static_cast<int32_t>(std::ceil(29.5 * (m - 1))) + (y - 1) * 354 + (3 + 11 * y) / 30 + 1948439;
}

int median(int* v, size_t n) {
    std::sort(v, v + n);
    if (n % 2 == 1) return v[n / 2];
    return static_cast<int>(std::lround((v[n / 2 - 1] + v[n / 2]) / 2.0));
}

}  // namespace

engine::HijriDate tabular_hijri(engine::Date d) { return from_jdn(jdn(d)); }

int8_t hijri_offset_for(engine::Date d, const engine::HijriDate& published) {
    const int32_t diff = to_jdn(published) - jdn(d);
    return static_cast<int8_t>(std::max<int32_t>(-3, std::min<int32_t>(3, diff)));
}

engine::HijriDate computed_hijri(engine::Date d, int8_t hijri_offset) { return from_jdn(jdn(d) + hijri_offset); }

CalibrationResult calibrate(const engine::DayRecord* records, size_t n, const Coordinates& where,
                            int32_t utc_offset_s) {
    CalibrationResult r;
    int diffs[6][kCalibrationMaxDays];
    size_t used = 0;
    for (size_t i = 0; i < n && used < kCalibrationMaxDays; ++i) {
        const engine::DayRecord& rec = records[i];
        if (engine::rank(rec.source) < 40) continue;
        uint16_t calc[6];
        if (!prayer_times_local(where, rec.date, turkey(), utc_offset_s, calc)) continue;
        for (int k = 0; k < 6; ++k) diffs[k][used] = static_cast<int>(rec.minutes[k]) - static_cast<int>(calc[k]);
        ++used;
    }
    // The Hijri offset from the latest Diyanet-derived record that carries a Hijri date.
    for (size_t i = n; i > 0; --i) {
        const engine::DayRecord& rec = records[i - 1];
        if (engine::rank(rec.source) >= 40 && rec.hijri.present) {
            r.hijri_offset = hijri_offset_for(rec.date, rec.hijri);
            r.hijri_ok = true;
            break;
        }
    }
    r.days = static_cast<uint8_t>(used);
    if (used < kCalibrationMinDays) {
        r.status = CalibrationResult::Status::TooFewDays;
        return r;
    }
    for (int k = 0; k < 6; ++k) {
        const int m = median(diffs[k], used);
        if (std::abs(m) > kCalibrationLimit) {
            r.status = CalibrationResult::Status::OffsetTooLarge;
            r.bad_prayer = static_cast<int8_t>(k);
            r.bad_offset = static_cast<int16_t>(m);
            return r;
        }
        r.offsets[k] = static_cast<int8_t>(m);
    }
    r.status = CalibrationResult::Status::Ok;
    return r;
}

bool local_record(engine::Date d, const Coordinates& where, const store::Calibration* cal, uint32_t district,
                  engine::DayRecord& out, int32_t utc_offset_s) {
    uint16_t t[6];
    if (!prayer_times_local(where, d, turkey(), utc_offset_s, t)) return false;
    const bool use = cal != nullptr && cal->valid && cal->district == district;
    out = engine::DayRecord{};
    out.date = d;
    for (int k = 0; k < 6; ++k) {
        const int v = static_cast<int>(t[k]) + (use ? cal->offsets[k] : 0);
        if (v < 0 || v >= 1440) return false;
        out.minutes[k] = static_cast<uint16_t>(v);
    }
    out.hijri = computed_hijri(d, cal != nullptr && cal->district == district ? cal->hijri_offset : 0);
    out.source = engine::SourceId::Local;
    return engine::valid(out);
}

size_t fill_local(store::Store& s, engine::Date today, const Coordinates& where, const store::Calibration* cal,
                  size_t days, int32_t utc_offset_s) {
    size_t added = 0;
    for (size_t i = 0; i < days; ++i) {
        const engine::Date d = engine::to_date(engine::to_day_number(today) + static_cast<int32_t>(i));
        if (store::find(s, d) != nullptr || s.count >= store::kMaxRecords) continue;
        engine::DayRecord rec;
        if (!local_record(d, where, cal, s.district_id, rec, utc_offset_s)) continue;
        // Insert in date order.
        const engine::DayNumber n = engine::to_day_number(d);
        size_t at = s.count;
        while (at > 0 && engine::to_day_number(s.records[at - 1].date) > n) {
            s.records[at] = s.records[at - 1];
            --at;
        }
        s.records[at] = rec;
        ++s.count;
        ++added;
    }
    return added;
}

}  // namespace nvp::calc
