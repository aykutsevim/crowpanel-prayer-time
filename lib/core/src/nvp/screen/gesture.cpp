#include "nvp/screen/gesture.h"

namespace nvp::screen {

bool TwoFingerHold::feed(uint32_t ms, uint8_t points) {
    if (points == 0) {
        holding_ = false;
        fired_ = false;  // all fingers lifted: re-arm
        return false;
    }
    if (points < 2) {
        holding_ = false;
        return false;
    }
    if (fired_) return false;
    if (!holding_) {
        holding_ = true;
        start_ = ms;
        return false;
    }
    if (ms - start_ >= kHoldMs) {  // unsigned difference: correct across the 2^32 wrap
        fired_ = true;
        holding_ = false;
        return true;
    }
    return false;
}

Touch TouchClassifier::feed(uint32_t ms, uint8_t points, int16_t x, int16_t y) {
    if (two_.feed(ms, points)) {
        down_ = false;
        return Touch::TwoFingerHold5s;
    }
    if (points == 0) {
        const bool tap = down_ && !spoiled_ && ms - start_ <= kTapMaxMs;
        down_ = false;
        return tap ? Touch::Tap : Touch::None;
    }
    if (!down_) {
        down_ = true;
        spoiled_ = points > 1;
        start_ = ms;
        x0_ = x;
        y0_ = y;
        return Touch::None;
    }
    if (points > 1) {
        spoiled_ = true;
        return Touch::None;
    }
    if (spoiled_) return Touch::None;
    const int32_t dx = x - x0_, dy = y - y0_;
    if (dx * dx + dy * dy > kMoveMaxPx * kMoveMaxPx) {
        spoiled_ = true;
        return Touch::None;
    }
    if (ms - start_ >= kHoldMs) {
        spoiled_ = true;
        return Touch::Hold5s;
    }
    return Touch::None;
}

}  // namespace nvp::screen
