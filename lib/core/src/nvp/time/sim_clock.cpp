#include "nvp/time/sim_clock.h"

namespace nvp::time {
namespace {

int64_t floor_div(int64_t a, int64_t b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }

}  // namespace

void SimClock::set(int64_t local, uint32_t ms) {
    base_local_ms_ = local * 1000;
    base_ms_ = ms;
    speed_ = 1;
    paused_ = false;
}

void SimClock::rebase(uint32_t ms) {
    const uint32_t elapsed = ms - base_ms_;  // unsigned: correct across the 2^32 wrap
    if (!paused_) base_local_ms_ += static_cast<int64_t>(elapsed) * speed_;
    base_ms_ = ms;
}

bool SimClock::set_speed(uint16_t speed, uint32_t ms) {
    if (speed < 1 || speed > kMaxSpeed) return false;
    rebase(ms);
    speed_ = speed;
    return true;
}

void SimClock::pause(uint32_t ms) {
    rebase(ms);
    paused_ = true;
}

void SimClock::run(uint32_t ms) {
    rebase(ms);
    paused_ = false;
}

int64_t SimClock::now(uint32_t ms) const {
    const uint32_t elapsed = ms - base_ms_;
    const int64_t local_ms = paused_ ? base_local_ms_ : base_local_ms_ + static_cast<int64_t>(elapsed) * speed_;
    return floor_div(local_ms, 1000);
}

}  // namespace nvp::time
