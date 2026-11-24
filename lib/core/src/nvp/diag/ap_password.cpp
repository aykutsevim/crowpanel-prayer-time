#include "nvp/diag/ap_password.h"

#include <cstdio>

namespace nvp::diag {

namespace {

// A uniform value below `range`: words in the top block that does not hold a whole `range` are
// drawn again (at most 2.4 % of the words for 10^8, 0.02 % for 10^6).
unsigned long uniform_below(RandomWord random, uint32_t range) {
    const uint32_t limit = static_cast<uint32_t>((0x100000000ull / range) * range);  // excluded from here
    for (;;) {
        const uint32_t w = random();
        if (w < limit) return static_cast<unsigned long>(w % range);
    }
}

}  // namespace

void make_ap_password(RandomWord random, char out[9]) {
    std::snprintf(out, 9, "%08lu", uniform_below(random, 100000000u));
}

void make_settings_pin(RandomWord random, char out[7]) {
    std::snprintf(out, 7, "%06lu", uniform_below(random, 1000000u));
}

}  // namespace nvp::diag
