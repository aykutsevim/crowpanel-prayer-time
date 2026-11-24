// Network time over SNTP (TIME-01; spec 005 FR-001, research R1). Three servers of different
// operators, hourly resync, immediate mode. The firmware overrides lwIP's weak sntp_sync_time(), so
// an answer never sets the clock by itself: it is stored as a sample, and the app applies it
// through nvp::time::WallClock (validity rule, logging, board-clock write).
#pragma once

#include "nvp/time/wall_clock.h"

namespace platform::time_sync {

void start();    // once, after the first address on the owner's network
void restart();  // ask for a fresh answer now (link back, `time sync`)
void stop();     // `ntp off`
bool running();

// The newest answer since the last call, if any. Thread-safe (lwIP's task stores, the app takes).
bool take_sample(nvp::time::TimeSample& out);

}  // namespace platform::time_sync
