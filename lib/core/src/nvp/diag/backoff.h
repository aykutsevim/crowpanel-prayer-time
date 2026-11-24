// Reconnect backoff: 5 s, doubling, capped at 5 min; reset on success (NET-01, research R11).
#pragma once

#include <cstdint>

namespace nvp::diag {

class Backoff {
public:
    static constexpr uint32_t kFirstMs = 5000;
    static constexpr uint32_t kMaxMs = 300000;

    // The delay before the next attempt; each call doubles the following one, up to the cap.
    uint32_t next_delay_ms() {
        const uint32_t d = next_;
        next_ = next_ >= kMaxMs / 2 ? kMaxMs : next_ * 2;
        return d;
    }

    void reset() { next_ = kFirstMs; }

private:
    uint32_t next_ = kFirstMs;
};

}  // namespace nvp::diag
