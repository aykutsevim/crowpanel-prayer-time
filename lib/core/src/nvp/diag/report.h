// The diagnostic report (FR-026, contracts/serial-console.md). Built by a pure function from
// plain data. ReportInput has no password field, so a password can never be formatted
// (FR-024, NET-08). Pure C++, no heap.
#pragma once

#include <cstddef>
#include <cstdint>

#include "nvp/diag/selftest.h"
#include "nvp/diag/stress.h"
#include "nvp/diag/touch_judge.h"

namespace nvp::diag {

// The silence step as the platform recorded it (data-model.md SilenceState).
struct SilenceLine {
    bool ran = false;
    bool acknowledged = false;
    bool ran_first = false;
    uint8_t attempts = 0;
    char reason[48] = {};
};

enum class WifiKind : uint8_t { Off, NotSetUp, Joining, Connected, Hotspot };

// Wi-Fi state for the report: names and addresses only, never a password.
struct WifiLine {
    WifiKind kind = WifiKind::Off;
    char ssid[33] = {};        // station network name, up to 32 bytes
    int8_t rssi_dbm = 0;
    char ap_ssid[33] = {};     // hotspot name (NET-02)
    char ap_address[16] = {};  // hotspot address, for example 192.168.4.1
};

struct ReportInput {
    const char* firmware = "";
    const char* model = "";
    const char* revision = "";
    bool revision_detected = false;
    uint32_t flash_bytes = 0;  // detected flash chip size, 0 if unknown
    uint32_t psram_bytes = 0;  // detected PSRAM size, 0 if none
    SilenceLine silence;
    SelfTestResult tests[kTestCount];
    WifiLine wifi;
    CornerResult corners[kCornerCount];  // FR-014a, in Corner order
    uint32_t heap_internal_free = 0;
    uint32_t heap_internal_largest = 0;
    uint32_t heap_psram_free = 0;
    bool stress_running = false;
    bool stress_paused = false;   // hotspot open (spec edge case)
    bool stress_has_run = false;  // counters of the last run are kept after it stops (FR-009)
    StressCounters stress;
    uint32_t boot_to_diag_ms = 0;  // 0: no display (SC-006)
    uint32_t uptime_s = 0;
    const char* reset_reason = "unknown";  // why the chip last restarted (power-on, panic, ...)
    uint32_t coredump_bytes = 0;           // size of a crash record saved in flash, 0 if none
};

namespace report {

// Writes the report into `out` (always NUL-terminated when cap > 0) and returns the length the
// full report needs, like snprintf. A short buffer truncates; it never overflows.
size_t format(const ReportInput& in, char* out, size_t cap);

// Just the `audio-off: ...` line of the report, printed on its own right after the serial
// console starts (plan startup step 2). Same contract as format().
size_t format_silence_line(const SilenceLine& silence, char* out, size_t cap);

}  // namespace report
}  // namespace nvp::diag
