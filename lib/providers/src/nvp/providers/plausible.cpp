#include "nvp/providers/plausible.h"

#include <cstdlib>

#include "nvp/calc/prayer_times.h"

namespace nvp::providers {

Plausibility check_plausible(const engine::DayRecord* records, size_t n, const calc::Coordinates& where,
                             int32_t utc_offset_s, int limit) {
    Plausibility p;
    if (n == 0) return p;
    p.checked = true;
    const size_t picks[3] = {0, n / 2, n - 1};
    for (size_t idx : picks) {
        const engine::DayRecord& r = records[idx];
        uint16_t calc[6];
        if (!calc::prayer_times_local(where, r.date, calc::turkey(), utc_offset_s, calc)) continue;
        for (int k = 0; k < 6; ++k) {
            const int d = static_cast<int>(r.minutes[k]) - static_cast<int>(calc[k]);
            if (std::abs(d) > limit) {
                p.ok = false;
                p.date = r.date;
                p.prayer = static_cast<int8_t>(k);
                p.diff = static_cast<int16_t>(d);
                return p;
            }
        }
    }
    return p;
}

bool batches_agree(const engine::DayRecord* a, size_t na, const engine::DayRecord* b, size_t nb) {
    size_t shared = 0;
    for (size_t i = 0; i < na; ++i) {
        for (size_t j = 0; j < nb; ++j) {
            if (!(a[i].date == b[j].date)) continue;
            ++shared;
            for (int k = 0; k < 6; ++k)
                if (a[i].minutes[k] != b[j].minutes[k]) return false;
        }
    }
    return shared > 0;
}

}  // namespace nvp::providers
