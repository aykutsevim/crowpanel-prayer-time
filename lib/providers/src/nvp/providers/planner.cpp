#include "nvp/providers/planner.h"

#include "nvp/providers/runner.h"

namespace nvp::providers {
namespace {

int64_t floor_div(int64_t a, int64_t b) {
    int64_t q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) --q;
    return q;
}

}  // namespace

int64_t retry_delay(uint8_t step) {
    switch (step) {
        case 0: return 0;
        case 1: return 5 * 60;
        case 2: return 15 * 60;
        case 3: return 3600;
        case 4: return 3 * 3600;
        default: return 6 * 3600;
    }
}

uint16_t night_minute_of(const uint8_t mac[6]) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < 6; ++i) {
        h ^= mac[i];
        h *= 16777619u;
    }
    return static_cast<uint16_t>(kNightStart + h % kNightSpan);
}

const char* trigger_text(Trigger t) {
    switch (t) {
        case Trigger::None: return "none";
        case Trigger::Setup: return "setup";
        case Trigger::LocationChanged: return "location changed";
        case Trigger::Manual: return "manual";
        case Trigger::Boot: return "boot";
        case Trigger::Reconnect: return "reconnection";
        case Trigger::Retry: return "retry";
        case Trigger::Weekly: return "weekly";
        case Trigger::LowCoverage: return "low coverage";
    }
    return "?";
}

void Planner::on_boot() { boot_pending_ = true; }

void Planner::on_link_up() {
    if (seen_link_ && link_was_down_) reconnect_pending_ = true;
    seen_link_ = true;
    link_was_down_ = false;
}

void Planner::on_link_down() {
    if (seen_link_) link_was_down_ = true;
}

void Planner::on_setup_done() {
    immediate_pending_ = true;
    immediate_ = Trigger::Setup;
}

void Planner::on_location_changed() {
    immediate_pending_ = true;
    immediate_ = Trigger::LocationChanged;
    st_.retry_step = 0;
    st_.next_retry_utc = 0;
    st_.calibration = store::Calibration{};
    st_.coords_suspect = false;
}

bool Planner::on_manual(int64_t now) {
    if (manual_used_ && now - last_manual_ < kManualGap) return false;
    manual_used_ = true;
    last_manual_ = now;
    manual_pending_ = true;
    return true;
}

bool Planner::boot_condition(const PlannerView& v, const char** why) const {
    if (!v.today_diyanet) {
        *why = "no Diyanet record for today";
    } else if (v.days_ahead < kGoalDaysAhead) {
        *why = "under 30 days ahead";
    } else if (st_.last_success_utc == 0) {
        *why = "never refreshed";
    } else if (v.now_utc - st_.last_success_utc > kWeek) {
        *why = "last success over 7 days ago";
    } else {
        return false;
    }
    return true;
}

Decision Planner::decide_events(const PlannerView& v) {
    Decision d;
    const char* why = "";
    if (immediate_pending_) {
        immediate_pending_ = false;
        boot_pending_ = false;
        d.start = true;
        d.trigger = immediate_;
        d.why = immediate_ == Trigger::Setup ? "setup completed" : "location changed";
        return d;
    }
    if (manual_pending_) {
        manual_pending_ = false;
        d.start = true;
        d.trigger = Trigger::Manual;
        d.why = "development command";
        return d;
    }
    if (boot_pending_) {
        boot_pending_ = false;
        reconnect_pending_ = false;  // the first connection after a start is the boot's
        if (boot_condition(v, &why)) {
            d.start = true;
            d.trigger = Trigger::Boot;
            d.why = why;
            return d;
        }
    }
    if (reconnect_pending_) {
        reconnect_pending_ = false;
        const bool due = boot_condition(v, &why) || st_.retry_step > 0;
        if (due && (!reconnect_used_ || v.now_utc - last_reconnect_ >= kReconnectGap)) {
            reconnect_used_ = true;
            last_reconnect_ = v.now_utc;
            d.start = true;
            d.trigger = Trigger::Reconnect;
            d.why = st_.retry_step > 0 && !boot_condition(v, &why) ? "retry pending" : why;
            return d;
        }
    }
    if (st_.next_retry_utc != 0 && v.now_utc >= st_.next_retry_utc) {
        d.start = true;
        d.trigger = Trigger::Retry;
        d.lite = v.today_calculated;
        d.why = v.today_calculated ? "hourly while calculated" : "last refresh did not reach its goal";
        return d;
    }
    return d;
}

Decision Planner::decide(const PlannerView& v) {
    Decision d;
    if (!v.clock_valid || !v.has_location || !v.link_up) return d;  // FR-006; pending events wait
    // Any refresh that starts inside tonight's window counts as tonight's (no second one a minute later).
    const int64_t local = v.now_utc + v.utc_offset_s;
    const engine::DayNumber day = static_cast<engine::DayNumber>(floor_div(local, 86400));
    const int64_t minute = (local - static_cast<int64_t>(day) * 86400) / 60;
    const bool in_window = minute >= night_ && minute < night_ + kNightWindow;
    d = decide_events(v);
    if (d.start) {
        if (in_window) last_night_day_ = day;
        return d;
    }
    // The night window (FR-004 c, d; FR-005).
    if (in_window && day != last_night_day_) {
        last_night_day_ = day;
        if (st_.last_success_utc == 0 || v.now_utc - st_.last_success_utc >= kWeek) {
            d.start = true;
            d.trigger = Trigger::Weekly;
            d.why = "7 days since the last success";
        } else if (v.days_ahead < kGoalDaysAhead) {
            d.start = true;
            d.trigger = Trigger::LowCoverage;
            d.why = "under 30 days ahead";
        }
    }
    return d;
}

void Planner::on_finished(store::RefreshOutcome outcome, bool diyanet_stored, int64_t now, bool today_calculated) {
    st_.last_attempt_utc = now;
    st_.last_outcome = outcome;
    if (diyanet_stored) st_.last_success_utc = now;
    if (outcome == store::RefreshOutcome::Complete) {
        st_.retry_step = 0;
        st_.next_retry_utc = 0;
        return;
    }
    if (st_.retry_step < 5) ++st_.retry_step;
    st_.next_retry_utc = now + (today_calculated ? kCalculatedRetry : retry_delay(st_.retry_step));
}

int64_t Planner::next_night_after(int64_t utc, int32_t offset) const {
    const int64_t local = utc + offset;
    const int64_t day = floor_div(local, 86400);
    int64_t candidate = day * 86400 + static_cast<int64_t>(night_) * 60;
    if (candidate < local) candidate += 86400;
    return candidate - offset;
}

int64_t Planner::next_weekly_utc(const PlannerView& v) const {
    const int64_t base = st_.last_success_utc == 0 ? v.now_utc
                                                   : (st_.last_success_utc + kWeek > v.now_utc
                                                          ? st_.last_success_utc + kWeek
                                                          : v.now_utc);
    return next_night_after(base, v.utc_offset_s);
}

int64_t Planner::next_scheduled_utc(const PlannerView& v) const {
    int64_t best = next_weekly_utc(v);
    if (v.days_ahead < kGoalDaysAhead) {
        const int64_t n = next_night_after(v.now_utc, v.utc_offset_s);
        if (n < best) best = n;
    }
    if (st_.next_retry_utc != 0 && st_.next_retry_utc < best) best = st_.next_retry_utc;
    return best;
}

}  // namespace nvp::providers
