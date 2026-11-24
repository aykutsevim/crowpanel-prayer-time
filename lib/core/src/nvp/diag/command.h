// Serial command parser (FR-028, contracts/serial-console.md). Pure C++.
// There is deliberately no backlight command (OD1, FR-016).
#pragma once

#include <cstdint>

#include "nvp/screen/system_state.h"

namespace nvp::diag {

enum class Command {
    Empty,  // blank line: nothing to do
    Help,
    Report,
    StressOn,
    StressOff,
    WifiSetup,
    WifiCancel,
    WifiForget,
    Diag,
    TestWrite,  // one 4 kB scratch write (diagnostic one-shot)
    TestScan,   // one Wi-Fi scan (diagnostic one-shot)
    TestHttps,  // one HTTPS request (diagnostic one-shot)
    TestMix,    // a scan with a write at its start, as one stress cycle (diagnostic one-shot)
    // Spec 003 (contracts/serial-console.md): modes, clock, states, fixtures.
    Main,        // leave the diagnostic mode for the main screen
    TimeGet,     // read the board's real-time clock
    TimeSet,     // write it from a local date and time (year 2000..2099)
    ClockRtc,    // the screen follows the board's clock
    ClockSim,    // simulated local time
    ClockSpeed,  // speed of the simulated clock, 1..3600
    ClockPause,
    ClockRun,
    StateForce,  // force a system state (cache_ending takes 0..7 stored days)
    StateAuto,
    Fixture,     // show reference fixture 1..17 (15-17: spec 004 setup screens)
    FixtureOff,
    InvalidArgument,  // a known command with a bad or missing argument
    Unknown,
};

struct ParsedCommand {
    Command command = Command::Unknown;
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;  // TimeSet, ClockSim
    int32_t number = 0;  // ClockSpeed, Fixture, StateForce cache_ending days
    screen::SystemState state = screen::SystemState::Ok;  // StateForce
};

// parse_command plus the commands that take arguments. Same case and whitespace rules.
ParsedCommand parse_command_line(const char* line);

// Case-insensitive; leading, trailing and repeated whitespace and line-end bytes are ignored.
Command parse_command(const char* line);

}  // namespace nvp::diag
