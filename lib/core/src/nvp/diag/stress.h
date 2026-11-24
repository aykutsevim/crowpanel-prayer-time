// Stress mode schedule and counters (data-model.md StressCounters; FR-009..FR-013).
// One cycle about every 10 s (Wi-Fi scan and a scratch write); an HTTPS request at most every
// 30 s (clarification 2026-11-20). Nothing runs while the hotspot is up. Pure C++.
#pragma once

#include <cstdint>

namespace nvp::diag {

struct StressPlan {
    bool paused = false;      // hotspot open: nothing runs
    bool scan = false;
    bool write = false;
    bool https = false;       // a request is due and the device is connected
    bool https_skip = false;  // a request is due but the device is not connected (FR-011)
};

class StressSchedule {
public:
    static constexpr uint32_t kCycleMs = 10000;
    static constexpr uint32_t kHttpsMs = 30000;

    void start(uint32_t now_ms);
    StressPlan next(uint32_t now_ms, bool connected, bool ap_up);

private:
    bool first_cycle_ = true;
    bool https_attempted_ = false;
    uint32_t last_cycle_ms_ = 0;
    uint32_t last_https_ms_ = 0;
};

enum class HttpsOutcome : uint8_t { Ok, Error, Skipped };

struct StressCounters {
    uint32_t started_ms = 0;
    uint32_t elapsed_s = 0;
    uint32_t cycles = 0;
    uint32_t scan_ok = 0, scan_err = 0;
    uint32_t write_ok = 0, write_err = 0;
    uint32_t https_ok = 0, https_err = 0, https_skip = 0;
    char last_error[48] = {};

    void start(uint32_t now_ms);
    void record_scan(bool ok, const char* error = nullptr);
    void record_write(bool ok, const char* error = nullptr);
    void record_https(HttpsOutcome outcome, const char* error = nullptr);
    void cycle_done(uint32_t now_ms);
    void update_elapsed(uint32_t now_ms);  // keeps elapsed_s current between cycles
};

}  // namespace nvp::diag
