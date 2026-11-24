// The time the main screen shows (spec 003 OD3-1, FR-018, FR-026 to FR-030, research R11).
//
// Two sources:
//   Rtc        the board's battery-backed clock, read once at start and then advanced by the
//              system timer (TIME-05). Valid only when the reading was (TIME-03).
//   Simulated  a settable local time and speed, for fixtures and fast-forward tests (SYS-06).
// Uses only src/platform and lib/core (constitution VI).
#pragma once

#include <cstddef>
#include <cstdint>

#include "nvp/time/rtc_codec.h"

namespace app::clock {

enum class Source : uint8_t { Rtc, Simulated };

struct Now {
    bool valid = false;  // false: show the no_time state
    int64_t local = 0;   // local seconds since 1970 (UTC+3, nvp::time::zone)
};

void start();  // reads the board clock and logs the result; call once after the silence step
Now now();
Source source();

// Reads the chip again now (for `time get`): the reading, as UTC.
nvp::time::RtcReading read_rtc();

// Writes the chip from a local time (all time registers in one write, integrity bit cleared) and
// switches to the Rtc source. False with `why` filled when the chip does not acknowledge.
bool set_rtc_local(int64_t local, const char** why);

void use_rtc();
void use_sim(int64_t local, bool paused);
bool set_speed(uint16_t speed);  // Simulated only; 1..3600
void pause();
void run();

// The report's "clock:" line, without the key (contracts/serial-console.md).
void describe(char* out, size_t cap);

}  // namespace app::clock
