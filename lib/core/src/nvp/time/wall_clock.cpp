#include "nvp/time/wall_clock.h"

namespace nvp::time {

SyncResult WallClock::accept(const TimeSample& s, int64_t system_utc_us, int64_t now_uptime_us, int64_t build_utc_s) {
    SyncResult r;
    const int64_t sample_now = s.utc_us + (now_uptime_us - s.at_uptime_us);
    if (sample_now < build_utc_s * 1000000) {
        r.outcome = SyncOutcome::RefusedBeforeBuild;
        return r;
    }
    if (s.source == TimeSource::Http && valid_ && source_ == TimeSource::Ntp && ntp_fresh_allowed_ &&
        now_uptime_us - last_sync_uptime_us_ < kNtpFreshUs) {
        r.outcome = SyncOutcome::IgnoredNtpFresh;
        return r;
    }
    r.outcome = SyncOutcome::Accepted;
    r.new_utc_us = sample_now;
    r.correction_us = sample_now - system_utc_us;
    r.write_rtc = s.source == TimeSource::Ntp || s.source == TimeSource::Http;
    valid_ = true;
    source_ = s.source;
    last_sync_uptime_us_ = now_uptime_us;
    last_correction_us_ = r.correction_us;
    if (s.source == TimeSource::Ntp) ntp_fresh_allowed_ = true;
    return r;
}

void WallClock::invalidate() {
    valid_ = false;
    source_ = TimeSource::None;
}

void WallClock::forget_ntp_freshness() { ntp_fresh_allowed_ = false; }

const char* source_name(TimeSource s) {
    switch (s) {
        case TimeSource::None: return "none";
        case TimeSource::Rtc: return "rtc";
        case TimeSource::Ntp: return "ntp";
        case TimeSource::Http: return "http";
    }
    return "none";
}

}  // namespace nvp::time
