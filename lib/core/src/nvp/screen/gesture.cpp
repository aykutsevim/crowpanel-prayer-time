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

}  // namespace nvp::screen
