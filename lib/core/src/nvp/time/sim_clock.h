// A simulated local clock for fixtures and fast-forward tests (spec 003 FR-018, research R11).
// Pure: it is driven by a monotonic millisecond counter the caller passes in (wraps at 2^32).
#pragma once

#include <cstdint>

namespace nvp::time {

class SimClock {
public:
    static constexpr uint16_t kMaxSpeed = 3600;

    // Starts at local second count `local` at monotonic time `ms`, speed 1, running.
    void set(int64_t local, uint32_t ms);
    // 1..kMaxSpeed; rebases first so the time does not jump. False if out of range.
    bool set_speed(uint16_t speed, uint32_t ms);
    void pause(uint32_t ms);
    void run(uint32_t ms);

    int64_t now(uint32_t ms) const;  // local seconds
    uint16_t speed() const { return speed_; }
    bool paused() const { return paused_; }

private:
    void rebase(uint32_t ms);

    int64_t base_local_ms_ = 0;  // local time in milliseconds at base_ms_
    uint32_t base_ms_ = 0;
    uint16_t speed_ = 1;
    bool paused_ = false;
};

}  // namespace nvp::time
