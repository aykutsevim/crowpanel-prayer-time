// When to refresh (spec 007 FR-003..FR-006, FR-012, research R5, 04-data-sources §5.1, §5.3).
// A pure, deterministic state machine: the caller gives it events and, once a minute, a view of the
// time and the store; it answers "start a refresh now, because ..." or nothing. It knows nothing of
// Wi-Fi, tasks or HTTP. The persisted part of its state is the store::RefreshState it is given.
//
// Triggers, checked only with a valid clock, a location and the link up (FR-006):
//   setup done or location changed: at once;
//   manual: at once, at most once per minute;
//   boot: once, if today has no Diyanet-derived record, fewer than 30 days ahead are stored, or the
//         last success is more than 7 days old (or there was none);
//   reconnection after an outage: if a boot condition holds or a retry is pending, at most once per
//         10 minutes (this is the "one extra attempt" of FR-012);
//   retry: at next_retry_utc (5 min, 15 min, 1 h, 3 h, then every 6 h; hourly while today is
//         calculated, and then `lite`);
//   night: once per local date in the hour starting at the night minute (03:00..04:59, from the MAC):
//         weekly when the last success is 7 days old, else when fewer than 30 days ahead are stored.
#pragma once
#include <cstddef>
#include <cstdint>

#include "nvp/engine/calendar.h"
#include "nvp/store/refresh_state.h"

namespace nvp::providers {

inline constexpr int64_t kWeek = 7 * 86400;
inline constexpr int64_t kReconnectGap = 10 * 60;
inline constexpr int64_t kManualGap = 60;
inline constexpr int64_t kCalculatedRetry = 3600;
inline constexpr uint16_t kNightStart = 180;  // 03:00
inline constexpr uint16_t kNightSpan = 120;   // .. 04:59
inline constexpr uint16_t kNightWindow = 60;  // minutes after the night minute a missed tick may still fire

// Retry delay of a step 1..5 (FR-012), in seconds.
int64_t retry_delay(uint8_t step);

// 180 + FNV-1a-32(mac) mod 120 (FR-005).
uint16_t night_minute_of(const uint8_t mac[6]);

enum class Trigger : uint8_t { None, Setup, LocationChanged, Manual, Boot, Reconnect, Retry, Weekly, LowCoverage };
const char* trigger_text(Trigger t);  // "boot", ...

struct PlannerView {
    int64_t now_utc = 0;
    int32_t utc_offset_s = 0;       // local = UTC + offset (the time zone of Slice 5)
    bool clock_valid = false;
    bool link_up = false;
    bool has_location = false;
    bool today_diyanet = false;     // today has a Diyanet-derived record
    int32_t days_ahead = -1;        // runner::diyanet_days_ahead
    bool today_calculated = false;  // today's record on screen is calculated (rank below 40)
};

struct Decision {
    bool start = false;
    Trigger trigger = Trigger::None;
    bool lite = false;              // the hourly retry while calculated (analysis I2)
    const char* why = "";           // for the log, for example "under 30 days ahead"
};

class Planner {
public:
    explicit Planner(store::RefreshState& state) : st_(state) {}

    void set_night_minute(uint16_t m) { night_ = m; }
    uint16_t night_minute() const { return night_; }

    void on_boot();
    void on_link_up();
    void on_link_down();
    void on_setup_done();
    void on_location_changed();  // also clears calibration, coords_suspect and the retry
    bool on_manual(int64_t now_utc);  // false: refused, once per minute

    // Call once a minute (and after an event). A returned start must be followed by on_finished().
    Decision decide(const PlannerView& v);
    void on_finished(store::RefreshOutcome outcome, bool diyanet_stored, int64_t now_utc, bool today_calculated);

    // The next moment a refresh is planned (UTC s), for `data plan`; 0 when none.
    int64_t next_scheduled_utc(const PlannerView& v) const;
    int64_t next_weekly_utc(const PlannerView& v) const;

    const store::RefreshState& state() const { return st_; }

private:
    Decision decide_events(const PlannerView& v);  // all triggers but the night window
    bool boot_condition(const PlannerView& v, const char** why) const;
    int64_t next_night_after(int64_t utc, int32_t offset) const;

    store::RefreshState& st_;
    uint16_t night_ = kNightStart;
    bool boot_pending_ = false;
    bool immediate_pending_ = false;
    Trigger immediate_ = Trigger::None;
    bool manual_pending_ = false;
    bool reconnect_pending_ = false;
    bool seen_link_ = false;
    bool link_was_down_ = false;
    int64_t last_reconnect_ = 0;
    bool reconnect_used_ = false;
    int64_t last_manual_ = 0;
    bool manual_used_ = false;
    engine::DayNumber last_night_day_ = -1;
};

}  // namespace nvp::providers
