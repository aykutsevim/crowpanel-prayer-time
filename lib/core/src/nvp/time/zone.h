// Local time for Türkiye (TIME-04 default zone "<+03>-3"; spec 003 FR-027, research R2).
// A fixed offset: Türkiye has no daylight saving. Parsing a general POSIX TZ string is Slice 5.
#pragma once

#include <cstdint>

namespace nvp::time {

inline constexpr int16_t kDefaultUtcOffsetMinutes = 180;

inline int64_t utc_to_local(int64_t utc) { return utc + kDefaultUtcOffsetMinutes * 60; }
inline int64_t local_to_utc(int64_t local) { return local - kDefaultUtcOffsetMinutes * 60; }

}  // namespace nvp::time
