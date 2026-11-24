// Hotspot password: 8 decimal digits from a random source (NET-02, research R11). Generated once,
// stored on the device, shown only on the device screen (FR-020, FR-024).
#pragma once

#include <cstdint>

namespace nvp::diag {

using RandomWord = uint32_t (*)();

// Writes 8 digits and a NUL into out[9]. Leading zeros are kept. Uniform: draws again when the
// random word falls in the incomplete top block (spec 004 research R10).
void make_ap_password(RandomWord random, char out[9]);

// The settings PIN (03 §8 settings table, SET-02; spec 004 FR-022): 6 digits and a NUL into
// out[7], uniform in the same way.
void make_settings_pin(RandomWord random, char out[7]);

}  // namespace nvp::diag
