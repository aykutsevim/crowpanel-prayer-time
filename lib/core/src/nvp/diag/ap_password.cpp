#include "nvp/diag/ap_password.h"

#include <cstdio>

namespace nvp::diag {

void make_ap_password(RandomWord random, char out[9]) {
    // 2^32 / 10^8 leaves a bias below 4 % between digit strings; irrelevant for a local hotspot
    // password that is shown on the screen.
    const unsigned long value = static_cast<unsigned long>(random() % 100000000u);
    std::snprintf(out, 9, "%08lu", value);
}

}  // namespace nvp::diag
