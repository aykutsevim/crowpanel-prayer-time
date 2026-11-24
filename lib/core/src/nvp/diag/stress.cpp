#include "nvp/diag/stress.h"

#include <cstdio>

namespace nvp::diag {

void StressSchedule::start(uint32_t now_ms) {
    first_cycle_ = true;
    https_attempted_ = false;
    last_cycle_ms_ = now_ms;
    last_https_ms_ = now_ms;
}

StressPlan StressSchedule::next(uint32_t now_ms, bool connected, bool ap_up) {
    StressPlan plan;
    if (ap_up) {
        plan.paused = true;
        return plan;
    }
    if (!first_cycle_ && now_ms - last_cycle_ms_ < kCycleMs) return plan;
    first_cycle_ = false;
    last_cycle_ms_ = now_ms;
    plan.scan = true;
    plan.write = true;
    if (!https_attempted_ || now_ms - last_https_ms_ >= kHttpsMs) {
        https_attempted_ = true;
        last_https_ms_ = now_ms;
        plan.https = connected;
        plan.https_skip = !connected;
    }
    return plan;
}

namespace {

void note_error(StressCounters& c, const char* error) {
    if (error != nullptr && error[0] != '\0') std::snprintf(c.last_error, sizeof c.last_error, "%s", error);
}

}  // namespace

void StressCounters::start(uint32_t now_ms) {
    *this = StressCounters{};
    started_ms = now_ms;
}

void StressCounters::record_scan(bool ok, const char* error) {
    if (ok) {
        ++scan_ok;
    } else {
        ++scan_err;
        note_error(*this, error);
    }
}

void StressCounters::record_write(bool ok, const char* error) {
    if (ok) {
        ++write_ok;
    } else {
        ++write_err;
        note_error(*this, error);
    }
}

void StressCounters::record_https(HttpsOutcome outcome, const char* error) {
    switch (outcome) {
        case HttpsOutcome::Ok: ++https_ok; break;
        case HttpsOutcome::Error:
            ++https_err;
            note_error(*this, error);
            break;
        case HttpsOutcome::Skipped: ++https_skip; break;
    }
}

void StressCounters::cycle_done(uint32_t now_ms) {
    ++cycles;
    update_elapsed(now_ms);
}

void StressCounters::update_elapsed(uint32_t now_ms) { elapsed_s = (now_ms - started_ms) / 1000; }

}  // namespace nvp::diag
