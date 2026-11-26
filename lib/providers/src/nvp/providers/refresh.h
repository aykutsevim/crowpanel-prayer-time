// İmsakiyem request URLs (spec 006 research R1). Spec 006's refresh_with lived here; since spec 007
// the refresh is nvp::providers::refresh_run (runner.h) over HttpSource (http_source.h), and only the
// URL builders remain (T046). Pure C++.
#pragma once
#include <cstddef>
#include <cstdint>

#include "nvp/engine/calendar.h"

namespace nvp::providers {

inline constexpr int32_t kV2Days = 365;  // the v2 limit is 366 days, from .. from + 365 (R1)
inline constexpr int32_t kV1Days = 30;   // one v1 page (R1)

// Writes the request URL for `from` onward. Returns false when cap is too small.
bool v2_url(uint32_t district, engine::Date from, char* out, size_t cap);
bool v1_url(uint32_t district, engine::Date from, char* out, size_t cap);

}  // namespace nvp::providers
