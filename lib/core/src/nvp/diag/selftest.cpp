#include "nvp/diag/selftest.h"

namespace nvp::diag {

const char* test_name(TestName name) {
    switch (name) {
        case TestName::Silence: return "silence";
        case TestName::Display: return "display";
        case TestName::Touch: return "touch";
        case TestName::Backlight: return "backlight";
        case TestName::Storage: return "storage";
        case TestName::WiFi: return "wifi";
    }
    return "unknown";
}

const char* outcome_name(Outcome outcome) {
    switch (outcome) {
        case Outcome::Pending: return "pending";
        case Outcome::Passed: return "passed";
        case Outcome::Failed: return "failed";
        case Outcome::Skipped: return "skipped";
    }
    return "unknown";
}

void set_result(SelfTestResult& result, Outcome outcome, const char* reason) {
    result.outcome = outcome;
    const bool needs_reason = outcome == Outcome::Failed || outcome == Outcome::Skipped;
    if (reason == nullptr || reason[0] == '\0') {
        reason = needs_reason ? "no reason given" : "";
    }
    size_t i = 0;
    for (; reason[i] != '\0' && i + 1 < sizeof result.reason; ++i) {
        const auto c = static_cast<unsigned char>(reason[i]);
        result.reason[i] = (c >= 0x20 && c < 0x7F) ? static_cast<char>(c) : '?';
    }
    result.reason[i] = '\0';
}

}  // namespace nvp::diag
