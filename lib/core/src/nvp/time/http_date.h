// The `Date` header of an HTTP answer, for the time fallback (spec 005 FR-002, TIME-02, research R2).
#pragma once

#include <cstdint>

namespace nvp::time {

// Parses the IMF-fixdate form of RFC 9110 §5.6.7 only ("Tue, 24 Nov 2026 11:41:17 GMT"): English
// names, the weekday must match the date, "GMT" required, nothing after it. On success writes UTC
// seconds since 1970 to `utc_s`; otherwise leaves it unchanged.
bool parse_http_date(const char* s, int64_t* utc_s);

}  // namespace nvp::time
