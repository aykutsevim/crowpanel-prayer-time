// The time fallback (TIME-02; spec 005 FR-002, research R2, owner decision OD5-1): read the `Date`
// header of a plain-HTTP answer from a host the device already contacts. Plain HTTP because
// certificate dates are checked (OD5-1) and the clock is what this fallback is for; nothing but the
// header is used. tools/check_tls.py allows a client without a CA in this file only, and only with
// http:// URLs. Call from the network worker only (blocking, at most 5 s).
#pragma once

#include <cstdint>

namespace platform::http_date {

struct Result {
    bool ok = false;
    int64_t utc_us = 0;      // the header's time + 0.5 s + half the round trip
    int64_t at_uptime_us = 0;
    uint32_t rtt_ms = 0;
    const char* why = "";    // short, static text when not ok
};

// `index` 0 or 1 chooses the host (http://ezanvakti.imsakiyem.com/, http://ezanvakti.emushaf.net/).
Result fetch(int index);
const char* host(int index);

}  // namespace platform::http_date
