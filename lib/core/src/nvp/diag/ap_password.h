// Hotspot password: 8 decimal digits from a random source (NET-02, research R11). Generated once,
// stored on the device, shown only on the device screen (FR-020, FR-024).
#pragma once

#include <cstdint>

namespace nvp::diag {

using RandomWord = uint32_t (*)();

// Writes 8 digits and a NUL into out[9]. Leading zeros are kept.
void make_ap_password(RandomWord random, char out[9]);

}  // namespace nvp::diag
