// The hidden gesture that opens the diagnostic screen: two (or more) fingers held on the main
// screen for 5 seconds (spec 003 FR-022, research R13; spec 001 FR-007a). It must not collide with
// 02-ui-spec.md §8, whose gestures are all single-finger.
#pragma once

#include <cstdint>

namespace nvp::screen {

class TwoFingerHold {
public:
    static constexpr uint32_t kHoldMs = 5000;

    // Feed every touch report: monotonic time and the number of touch points.
    // Returns true exactly once per hold, when 2+ points have been held for kHoldMs.
    // Fewer than 2 points resets the hold; after firing, it re-arms only when all fingers lift.
    bool feed(uint32_t ms, uint8_t points);

private:
    bool holding_ = false;
    bool fired_ = false;
    uint32_t start_ = 0;
};

}  // namespace nvp::screen
