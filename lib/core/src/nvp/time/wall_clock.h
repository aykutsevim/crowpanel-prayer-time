// The device's knowledge of the time: is it valid, where did it come from, what did the last sync
// change (spec 005 FR-003..FR-006, TIME-03, TIME-05, TIME-08; research R4, data-model.md).
// Pure: the caller passes the system clock and the uptime; src/app applies the result.
#pragma once

#include <cstdint>

namespace nvp::time {

enum class TimeSource : uint8_t { None, Rtc, Ntp, Http };

struct TimeSample {
    TimeSource source = TimeSource::None;
    int64_t utc_us = 0;        // what the source said, UTC microseconds since 1970
    int64_t at_uptime_us = 0;  // device uptime when it was taken
    const char* server = "";   // for the log only
};

enum class SyncOutcome : uint8_t { Accepted, RefusedBeforeBuild, IgnoredNtpFresh };

struct SyncResult {
    SyncOutcome outcome = SyncOutcome::RefusedBeforeBuild;
    int64_t new_utc_us = 0;     // the sample advanced to `now_uptime_us`; set the system clock to it
    int64_t correction_us = 0;  // new_utc_us - system clock (signed)
    bool write_rtc = false;     // every accepted network sample (FR-004)
};

class WallClock {
public:
    static constexpr int64_t kNtpFreshUs = 6LL * 3600 * 1000000;  // an Http sample waits this long

    // `system_utc_us`: the system clock now; `now_uptime_us`: the uptime now; `build_utc_s`: the
    // firmware's build time (TIME-03). A refused or ignored sample changes nothing.
    SyncResult accept(const TimeSample& s, int64_t system_utc_us, int64_t now_uptime_us, int64_t build_utc_s);

    void invalidate();            // `clock invalid`: not valid until the next accepted sample
    void forget_ntp_freshness();  // `ntp off`: the next Http sample is accepted

    bool valid() const { return valid_; }
    TimeSource source() const { return source_; }
    int64_t last_sync_uptime_us() const { return last_sync_uptime_us_; }
    int64_t last_correction_us() const { return last_correction_us_; }

private:
    bool valid_ = false;
    TimeSource source_ = TimeSource::None;
    bool ntp_fresh_allowed_ = true;
    int64_t last_sync_uptime_us_ = 0;
    int64_t last_correction_us_ = 0;
};

const char* source_name(TimeSource s);  // "none", "rtc", "ntp", "http"

}  // namespace nvp::time
