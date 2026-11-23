// Self-test results of the diagnostic mode (data-model.md SelfTestResult, FR-026, FR-027).
// Pure C++ (no Arduino, ESP-IDF or LVGL headers; constitution VI).
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvp::diag {

// The order of the report.
enum class TestName : uint8_t { Silence, Display, Touch, Backlight, Storage, WiFi };
inline constexpr size_t kTestCount = 6;

enum class Outcome : uint8_t { Pending, Passed, Failed, Skipped };

struct SelfTestResult {
    Outcome outcome = Outcome::Pending;
    // Short, printable ASCII, never a password (FR-024).
    char reason[48] = {};
};

constexpr size_t index(TestName name) { return static_cast<size_t>(name); }

const char* test_name(TestName name);       // "silence", "display", ...
const char* outcome_name(Outcome outcome);  // "pending", "passed", "failed", "skipped"

// Records an outcome. The reason is copied as printable ASCII and truncated to fit; other bytes
// become '?'. Failed and Skipped always carry a reason ("no reason given" if none was passed).
void set_result(SelfTestResult& result, Outcome outcome, const char* reason);

}  // namespace nvp::diag
