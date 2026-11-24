#include "nvp/diag/touch_judge.h"

namespace nvp::diag {
namespace {

int abs_int(int v) { return v < 0 ? -v : v; }

}  // namespace

int TouchJudge::judge(int x, int y) {
    if (x < 0 || y < 0 || x >= kScreenWidth || y >= kScreenHeight) return -1;
    int best = -1;
    long best_d2 = 0;
    for (size_t i = 0; i < kCornerCount; ++i) {
        const int dx = x - kCornerTargets[i].x;
        const int dy = y - kCornerTargets[i].y;
        if (abs_int(dx) > kAssignRadiusPx || abs_int(dy) > kAssignRadiusPx) continue;
        const long d2 = static_cast<long>(dx) * dx + static_cast<long>(dy) * dy;
        if (best < 0 || d2 < best_d2) {
            best = static_cast<int>(i);
            best_d2 = d2;
        }
    }
    if (best < 0) return -1;
    CornerResult& r = results_[static_cast<size_t>(best)];
    r.dx = static_cast<int16_t>(x - kCornerTargets[best].x);
    r.dy = static_cast<int16_t>(y - kCornerTargets[best].y);
    const bool pass = abs_int(r.dx) <= kPassTolerancePx && abs_int(r.dy) <= kPassTolerancePx;
    r.state = pass ? CornerState::Passed : CornerState::Failed;
    return best;
}

}  // namespace nvp::diag
