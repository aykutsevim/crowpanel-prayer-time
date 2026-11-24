// The time everything on the screen uses (spec 003 OD3-1, FR-018; spec 005 FR-003..FR-006, FR-011,
// FR-012, FR-014, FR-015; research R4, R5).
//
// Two sources:
//   Real       the system clock, set from the board's clock at start and corrected by network
//              time (SNTP, or the HTTP Date fallback) through nvp::time::WallClock, which decides
//              validity (TIME-03). Shown in the stored POSIX time zone.
//   Simulated  a settable local time and speed, for fixtures and fast-forward tests (SYS-06); it
//              overrides the real time everywhere while it is on.
// Lives on the UI core: samples from the network core arrive through post_sample() and SNTP's
// own slot, and are applied by apply_pending() in loop(). Uses only src/platform and lib/core.
#pragma once

#include <cstddef>
#include <cstdint>

#include "nvp/time/rtc_codec.h"
#include "nvp/time/wall_clock.h"

namespace app::clock {

enum class Source : uint8_t { Real, Simulated };

struct Now {
    bool valid = false;  // false: show the no_time state
    int64_t local = 0;   // local seconds since 1970 in the stored zone
};

void start();  // reads the board clock and the stored zone, sets the system clock; logs the result
Now now();
Source source();

// Takes SNTP and fallback samples and applies them (system clock, board clock, log). Call often.
void apply_pending();
// From the network worker (core 0): a fallback sample. Thread-safe.
void post_sample(const nvp::time::TimeSample& s);
// For the network worker: is the time valid now; how many network samples were accepted so far.
bool time_valid();
uint32_t network_syncs();
nvp::time::TimeSource last_source();  // the source of the time now (for the fallback's repeat, T038)

// Development commands (contracts/serial-console.md of specs 003 and 005).
nvp::time::RtcReading read_rtc();                    // `time get`: the chip, as UTC
bool set_rtc_local(int64_t local, const char** why);  // `time set`: chip + system clock, source rtc
void invalidate();                                    // `clock invalid`
void forget_ntp_freshness();                          // `ntp off`
void use_real();
void use_sim(int64_t local, bool paused);
bool set_speed(uint16_t speed);  // Simulated only; 1..3600
void pause();
void run();
const char* zone();                // the stored POSIX string
bool set_zone(const char* posix);  // parse, apply and store; false (nothing changed) when invalid
int64_t to_local(int64_t utc);     // in the stored zone
int64_t to_utc(int64_t local);

// The report's "clock:" and "time:" lines, without the key.
void describe(char* out, size_t cap);
void describe_time(char* out, size_t cap);

}  // namespace app::clock
