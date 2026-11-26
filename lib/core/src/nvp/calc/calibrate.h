// Self-calibration, the computed Hijri date and the device's own calculated days
// (spec 007 FR-017, FR-018, research R8, 04-data-sources §3.5, 02-ui-spec §8). Pure C++, no heap.
//
// Calibration: when at least 14 Diyanet-derived records are available, the offset of each of the six
// times is the median of (published - calculated) over up to 30 such records, the earliest first
// (yesterday on). An offset beyond ±10 minutes rejects the whole set: it means wrong coordinates.
//
// Hijri date for calculated days: the arithmetic (tabular, civil epoch) Islamic calendar, shifted
// by the days it differs from the latest Diyanet-derived record. Shown only in the calculated state.
#pragma once
#include <cstddef>
#include <cstdint>

#include "nvp/calc/astronomical.h"
#include "nvp/engine/calendar.h"
#include "nvp/engine/day_record.h"
#include "nvp/store/refresh_state.h"
#include "nvp/store/store.h"

namespace nvp::calc {

inline constexpr size_t kCalibrationMinDays = 14;
inline constexpr size_t kCalibrationMaxDays = 30;
inline constexpr int kCalibrationLimit = 10;  // minutes
inline constexpr int32_t kTurkeyOffset = 3 * 3600;

struct CalibrationResult {
    enum class Status : uint8_t { Ok, TooFewDays, OffsetTooLarge, NoCalculation };
    Status status = Status::TooFewDays;
    int8_t offsets[6] = {};
    uint8_t days = 0;
    int8_t bad_prayer = -1;  // OffsetTooLarge: 0..5 (imsak..yatsi)
    int16_t bad_offset = 0;
    bool hijri_ok = false;
    int8_t hijri_offset = 0;
};

// From the Diyanet-derived records in `records` (sorted by date).
CalibrationResult calibrate(const engine::DayRecord* records, size_t n, const Coordinates& where,
                            int32_t utc_offset_s = kTurkeyOffset);

// Tabular Islamic date of `d` (no offset).
engine::HijriDate tabular_hijri(engine::Date d);
// Days to add to the tabular calendar so that `d` gets `published` (Diyanet's Hijri date of d).
int8_t hijri_offset_for(engine::Date d, const engine::HijriDate& published);
// The Hijri date for a calculated day.
engine::HijriDate computed_hijri(engine::Date d, int8_t hijri_offset);

// Calculated times for `d` with the calibration applied (when valid for the district). False when
// the times cannot be computed or break ENG-01.
bool local_record(engine::Date d, const Coordinates& where, const store::Calibration* cal, uint32_t district,
                  engine::DayRecord& out, int32_t utc_offset_s = kTurkeyOffset);

// Adds rank-10 (Local) records for today .. today + days - 1 where the store has no record at all.
// Never replaces a record. Returns the number added. The store's last refresh is unchanged.
size_t fill_local(store::Store& s, engine::Date today, const Coordinates& where, const store::Calibration* cal,
                  size_t days = 8, int32_t utc_offset_s = kTurkeyOffset);

}  // namespace nvp::calc
