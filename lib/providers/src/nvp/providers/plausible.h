// Plausibility of a fetched Diyanet-derived batch (spec 007 FR-016, US7, 04-data-sources §7).
// Pure C++, no heap.
//
// With coordinates and a valid clock: for the first, middle and last record of the batch, each of
// the six times must lie within 20 minutes of the uncalibrated calculation for that date. This
// catches another place, a shifted hour and shifted days. The comparison uses the uncalibrated
// calculation on purpose: calibration learns from published data and must not excuse it.
#pragma once
#include <cstddef>
#include <cstdint>

#include "nvp/calc/astronomical.h"
#include "nvp/engine/calendar.h"
#include "nvp/engine/day_record.h"

namespace nvp::providers {

inline constexpr int kPlausibleMinutes = 20;

struct Plausibility {
    bool checked = false;
    bool ok = true;
    engine::Date date;  // the first failing record
    int8_t prayer = -1;  // 0..5 (imsak..yatsi)
    int16_t diff = 0;    // published minus calculated, minutes
};

Plausibility check_plausible(const engine::DayRecord* records, size_t n, const calc::Coordinates& where,
                             int32_t utc_offset_s, int limit = kPlausibleMinutes);

// True when the two batches share at least one date and agree on all six times of every shared date.
bool batches_agree(const engine::DayRecord* a, size_t na, const engine::DayRecord* b, size_t nb);

}  // namespace nvp::providers
