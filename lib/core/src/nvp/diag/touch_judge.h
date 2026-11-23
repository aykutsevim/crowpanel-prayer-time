// Corner-touch judge for the diagnostic screen (FR-014a, data-model.md CornerTouchResult).
// Pure C++. The device judges each corner target itself (clarification 2026-11-20).
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvp::diag {

enum class Corner : uint8_t { TopLeft, TopRight, BottomLeft, BottomRight };
inline constexpr size_t kCornerCount = 4;

struct Point16 {
    int16_t x;
    int16_t y;
};

// Target centres on the 800x480 screen, in Corner order.
inline constexpr Point16 kCornerTargets[kCornerCount] = {{20, 20}, {779, 20}, {20, 459}, {779, 459}};

inline constexpr int kPassTolerancePx = 10;  // passed when |dx| <= 10 and |dy| <= 10
inline constexpr int kAssignRadiusPx = 60;   // a touch counts for a target within 60 px
inline constexpr int kScreenWidth = 800;
inline constexpr int kScreenHeight = 480;

enum class CornerState : uint8_t { NotTouched, Passed, Failed };

struct CornerResult {
    CornerState state = CornerState::NotTouched;
    int16_t dx = 0;  // touch minus target, signed
    int16_t dy = 0;
};

class TouchJudge {
public:
    // Judges one touch-down point. Returns the index of the corner it was assigned to, or -1 when
    // it is off screen or not within kAssignRadiusPx of any target. The latest touch counts.
    int judge(int x, int y);

    const CornerResult& result(Corner corner) const { return results_[static_cast<size_t>(corner)]; }
    const CornerResult* results() const { return results_; }

private:
    CornerResult results_[kCornerCount];
};

}  // namespace nvp::diag
