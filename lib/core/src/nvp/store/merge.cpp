#include "nvp/store/merge.h"

#include <cstring>

#include "nvp/engine/calendar.h"

namespace nvp::store {
namespace {

bool same_times(const engine::DayRecord& a, const engine::DayRecord& b) {
    return std::memcmp(a.minutes, b.minutes, sizeof a.minutes) == 0;
}

// Index of the first record dated on or after n (binary search; the store is sorted).
uint16_t lower_bound(const Store& s, engine::DayNumber n) {
    uint16_t lo = 0, hi = s.count;
    while (lo < hi) {
        uint16_t mid = static_cast<uint16_t>((lo + hi) / 2);
        if (engine::to_day_number(s.records[mid].date) < n) lo = static_cast<uint16_t>(mid + 1); else hi = mid;
    }
    return lo;
}

}  // namespace

MergeStats merge(Store& s, const engine::DayRecord* incoming, size_t n, int64_t fetch_utc, engine::Date today) {
    MergeStats st;
    const engine::DayNumber keep_from = engine::to_day_number(today) - 1;  // yesterday

    // Prune stored records before yesterday.
    uint16_t first = lower_bound(s, keep_from);
    if (first > 0) {
        std::memmove(s.records, s.records + first, (s.count - first) * sizeof s.records[0]);
        s.count = static_cast<uint16_t>(s.count - first);
        st.pruned = static_cast<uint16_t>(st.pruned + first);
    }

    const bool later_fetch = fetch_utc >= s.last_refresh_utc;
    for (size_t i = 0; i < n; ++i) {
        const engine::DayRecord& in = incoming[i];
        const engine::DayNumber d = engine::to_day_number(in.date);
        if (d < keep_from) {
            ++st.pruned;
            continue;
        }
        uint16_t at = lower_bound(s, d);
        if (at < s.count && engine::to_day_number(s.records[at].date) == d) {
            engine::DayRecord& old = s.records[at];
            const uint8_t r_in = engine::rank(in.source);
            const uint8_t r_old = engine::rank(old.source);
            if (r_in == 40 && r_old == 40 && in.source != old.source && !same_times(in, old)) {
                if (!st.has_mismatch) {
                    st.has_mismatch = true;
                    st.mismatch_stored = old;
                    st.mismatch_incoming = in;
                }
                ++st.mismatches;
            }
            if (r_in > r_old || (r_in == r_old && later_fetch)) {
                old = in;
                ++st.replaced;
            } else {
                ++st.kept;
            }
            continue;
        }
        // A new date. When full, the oldest record goes (or the new one, when it is the oldest).
        if (s.count == kMaxRecords) {
            if (at == 0) {
                ++st.dropped;
                continue;
            }
            std::memmove(s.records, s.records + 1, (kMaxRecords - 1) * sizeof s.records[0]);
            --s.count;
            --at;
            ++st.dropped;
        }
        std::memmove(s.records + at + 1, s.records + at, (s.count - at) * sizeof s.records[0]);
        s.records[at] = in;
        ++s.count;
        ++st.added;
    }
    if (later_fetch) s.last_refresh_utc = fetch_utc;
    return st;
}

}  // namespace nvp::store
