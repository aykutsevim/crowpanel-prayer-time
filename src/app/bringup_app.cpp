#include "bringup_app.h"

#include <cstdio>

#include "app/serial_console.h"
#include "nvp/diag/report.h"
#include "nvp/diag/selftest.h"
#include "nvp/version.h"
#include "platform/audio_off.h"
#include "platform/backlight.h"
#include "platform/board.h"
#include "platform/console_io.h"
#include "platform/storage.h"
#include "platform/sysinfo.h"

namespace app {
namespace {

using nvp::diag::index;
using nvp::diag::Outcome;
using nvp::diag::SelfTestResult;
using nvp::diag::TestName;

constexpr const char* kNotInThisStage = "not in this stage";
constexpr const char* kSilenceRefusal = "refused: silence step not acknowledged\n";

SelfTestResult g_tests[nvp::diag::kTestCount];
char g_report[2048];

nvp::diag::SilenceLine silence_line() {
    const platform::silence::SilenceState& s = platform::silence::silence_state();
    nvp::diag::SilenceLine line;
    line.ran = s.ran;
    line.acknowledged = s.acknowledged;
    line.ran_first = s.ran_first;
    line.attempts = s.attempts;
    std::snprintf(line.reason, sizeof line.reason, "%s", s.reason);
    return line;
}

void print_report() {
    nvp::diag::ReportInput in;
    in.firmware = nvp::kFirmwareVersion;
    in.model = board::kModel;
    in.revision = board::kRevision;
    in.revision_detected = false;  // the firmware cannot read the board revision
    const platform::sysinfo::Snapshot snap = platform::sysinfo::read();
    in.flash_bytes = snap.flash_bytes;
    in.psram_bytes = snap.psram_bytes;
    in.silence = silence_line();
    for (size_t i = 0; i < nvp::diag::kTestCount; ++i) in.tests[i] = g_tests[i];
    in.wifi.kind = nvp::diag::WifiKind::Off;  // Stage 3
    in.heap_internal_free = snap.internal_free;
    in.heap_internal_largest = snap.internal_largest;
    in.heap_psram_free = snap.psram_free;
    in.stress_running = false;  // Stage 3
    in.boot_to_diag_ms = 0;     // Stage 2: no display yet
    in.uptime_s = platform::sysinfo::uptime_ms() / 1000;
    nvp::diag::report::format(in, g_report, sizeof g_report);
    platform::console::write(g_report);
}

// FR-001a: stress and Wi-Fi refuse while silence is not acknowledged. In Stage 1 neither exists
// yet, so an acknowledged silence step still gets "not available in this stage".
void start_needing_silence() {
    if (!can_use_radio_and_load()) {
        platform::console::write(kSilenceRefusal);
        return;
    }
    platform::console::write("not available in this stage\n");
}

void not_in_this_stage() { platform::console::write("not available in this stage\n"); }

const ConsoleHandlers kHandlers = [] {
    ConsoleHandlers h;
    h.report = print_report;
    h.stress_on = start_needing_silence;
    h.stress_off = not_in_this_stage;
    h.wifi_setup = start_needing_silence;
    h.wifi_cancel = not_in_this_stage;
    h.wifi_forget = not_in_this_stage;
    return h;
}();

}  // namespace

bool can_use_radio_and_load() { return platform::silence::silence_state().acknowledged; }

void print_silence_line() {
    char line[96];
    nvp::diag::report::format_silence_line(silence_line(), line, sizeof line);
    platform::console::write(line);
}

void start() {
    // Silence self-test: the result of the step that already ran first (FR-001a).
    const platform::silence::SilenceState& s = platform::silence::silence_state();
    if (s.acknowledged) {
        nvp::diag::set_result(g_tests[index(TestName::Silence)], Outcome::Passed, "");
    } else {
        nvp::diag::set_result(g_tests[index(TestName::Silence)], Outcome::Failed, s.reason);
    }

    // Step 3: backlight fully on, the only level (OD1, FR-017).
    if (platform::backlight::full_on()) {
        nvp::diag::set_result(g_tests[index(TestName::Backlight)], Outcome::Passed, "");
    } else {
        nvp::diag::set_result(g_tests[index(TestName::Backlight)], Outcome::Failed,
                              "byte 0 not acknowledged");
    }

    // Steps 4-6 arrive in Stage 2 (display, touch, diagnostic screen).
    nvp::diag::set_result(g_tests[index(TestName::Display)], Outcome::Skipped, kNotInThisStage);
    nvp::diag::set_result(g_tests[index(TestName::Touch)], Outcome::Skipped, kNotInThisStage);

    // Step 7: storage mount and self-test.
    platform::storage::Result r = platform::storage::mount();
    if (r.ok) r = platform::storage::self_test();
    nvp::diag::set_result(g_tests[index(TestName::Storage)], r.ok ? Outcome::Passed : Outcome::Failed,
                          r.reason);

    // Steps 8 and 10 arrive in Stage 3 (stored Wi-Fi details, background worker).
    nvp::diag::set_result(g_tests[index(TestName::WiFi)], Outcome::Skipped, kNotInThisStage);

    // Step 9: the report (FR-026).
    print_report();
}

void loop() { serial_console_poll(kHandlers); }

}  // namespace app
