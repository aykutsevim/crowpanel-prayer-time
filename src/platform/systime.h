// The system clock (newlib time; also what mbedTLS reads for certificate dates, spec 005 R3, R4).
#pragma once

#include <cstdint>

namespace platform::systime {

int64_t utc_us();                // gettimeofday
void set_utc_us(int64_t utc_us);  // settimeofday

}  // namespace platform::systime
