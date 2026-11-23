// Serial command parser (FR-028, contracts/serial-console.md). Pure C++.
// There is deliberately no backlight command (OD1, FR-016).
#pragma once

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
    Unknown,
};

// Case-insensitive; leading, trailing and repeated whitespace and line-end bytes are ignored.
Command parse_command(const char* line);

}  // namespace nvp::diag
