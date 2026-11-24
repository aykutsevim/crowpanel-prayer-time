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

// The touch gestures of 02 §8 plus the hidden one (spec 004 FR-008, research R12):
//   Tap             one finger, down at most 600 ms, moved at most 24 px; reported on lift;
//   Hold5s          one finger kept down 5 s within 24 px; reported once, no tap follows;
//   TwoFingerHold5s two or more fingers for 5 s (TwoFingerHold above).
// A second finger at any time cancels the tap and the one-finger hold of that touch.
enum class Touch : uint8_t { None, Tap, Hold5s, TwoFingerHold5s };

class TouchClassifier {
public:
    static constexpr uint32_t kTapMaxMs = 600;
    static constexpr int32_t kMoveMaxPx = 24;
    static constexpr uint32_t kHoldMs = 5000;

    // Feed every touch report: monotonic time, number of points and the first point's position.
    Touch feed(uint32_t ms, uint8_t points, int16_t x, int16_t y);

private:
    TwoFingerHold two_;
    bool down_ = false;
    bool spoiled_ = false;  // moved, a second finger, or the hold already fired
    uint32_t start_ = 0;
    int16_t x0_ = 0, y0_ = 0;
};

}  // namespace nvp::screen
