// Merging a fetched batch into the store (spec 006 FR-008, research R6,
// docs/specification/04-data-sources.md §4). Pure C++, no heap.
//
//   - one record per date;
//   - a higher rank replaces; a lower rank never does;
//   - an equal rank replaces when the batch is not older than the store's last refresh (the store
//     keeps one fetch time, analysis I1);
//   - two rank-40 sources that disagree on a date count one mismatch, and the first such pair is
//     kept for the log;
//   - records before yesterday are pruned; at most kMaxRecords are kept (the oldest dropped).
#pragma once

#include <cstddef>
#include <cstdint>

#include "nvp/engine/day_record.h"
#include "nvp/store/store.h"

namespace nvp::store {

struct MergeStats {
    uint16_t added = 0;
    uint16_t replaced = 0;
    uint16_t kept = 0;        // incoming records that did not replace a stored one
    uint16_t mismatches = 0;  // dates where two rank-40 sources disagreed
    uint16_t pruned = 0;      // stored or incoming records before yesterday
    uint16_t dropped = 0;     // records dropped to stay within kMaxRecords
    bool has_mismatch = false;
    engine::DayRecord mismatch_stored;    // the first mismatch: both time sets (R6)
    engine::DayRecord mismatch_incoming;
};

MergeStats merge(Store& s, const engine::DayRecord* incoming, size_t n, int64_t fetch_utc, engine::Date today);

}  // namespace nvp::store
