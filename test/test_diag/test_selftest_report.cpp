// Self-test results and the report (FR-026, FR-027, FR-024, contracts/serial-console.md).
// Written before the code.
#include <unity.h>

#include <cstring>

#include "nvp/diag/report.h"
#include "nvp/diag/selftest.h"

using namespace nvp::diag;

namespace {

ReportInput stage1_input() {
    ReportInput in;
    in.firmware = "0.1.0";
    in.model = "DIS02170A";
    in.revision = "V1.4";
    in.revision_detected = false;
    in.flash_bytes = 16u * 1024u * 1024u;
    in.psram_bytes = 8u * 1024u * 1024u;
    in.silence.ran = true;
    in.silence.acknowledged = true;
    in.silence.ran_first = true;
    in.silence.attempts = 1;
    set_result(in.tests[index(TestName::Silence)], Outcome::Passed, "");
    set_result(in.tests[index(TestName::Display)], Outcome::Skipped, "not in this stage");
    set_result(in.tests[index(TestName::Touch)], Outcome::Skipped, "not in this stage");
    set_result(in.tests[index(TestName::Backlight)], Outcome::Passed, "");
    set_result(in.tests[index(TestName::Storage)], Outcome::Passed, "");
    set_result(in.tests[index(TestName::WiFi)], Outcome::Skipped, "not in this stage");
    in.heap_internal_free = 182340;
    in.heap_internal_largest = 160000;
    in.heap_psram_free = 7340032;
    in.boot_to_diag_ms = 0;
    in.uptime_s = 42;
    in.reset_reason = "power-on";
    return in;
}

const char* kStage1Report =
    "=== NVP REPORT ===\n"
    "firmware: 0.1.0\n"
    "board: DIS02170A V1.4 (configured; not detected)\n"
    "memory: flash 16 MB, psram 8 MB (detected)\n"
    "audio-off: acknowledged first (attempts 1)\n"
    "selftest silence: passed\n"
    "selftest display: skipped (not in this stage)\n"
    "selftest touch: skipped (not in this stage)\n"
    "selftest backlight: passed\n"
    "selftest storage: passed\n"
    "selftest wifi: skipped (not in this stage)\n"
    "wifi: off\n"
    "backlight: full (dimming disabled)\n"
    "touch corners: TL not touched TR not touched BL not touched BR not touched\n"
    "heap internal: 182340 free, 160000 largest\n"
    "heap psram: 7340032 free\n"
    "stress: off\n"
    "boot-to-diag: n/a (no display)\n"
    "uptime: 42 s\n"
    "reset: power-on\n"
    "=== END ===\n";

}  // namespace

static void test_report_has_contract_keys_in_order() {
    char buf[1024];
    const ReportInput in = stage1_input();
    const size_t n = report::format(in, buf, sizeof buf);
    TEST_ASSERT_EQUAL_STRING(kStage1Report, buf);
    TEST_ASSERT_EQUAL_UINT32(std::strlen(kStage1Report), n);
}

static void test_clock_and_screen_lines() {  // spec 003 contracts/serial-console.md
    char buf[1024];
    ReportInput in = stage1_input();
    in.clock_line = "rtc 2026-11-24 12:30:05 local, valid";
    in.screen_line = "main (state no_data)";
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(
        std::strstr(buf, "\nclock: rtc 2026-11-24 12:30:05 local, valid\nscreen: main (state no_data)\n=== END ===\n"));
}

static void test_time_line() {  // spec 005 FR-015, contracts/serial-console.md
    char buf[1024];
    ReportInput in = stage1_input();
    in.clock_line = "rtc 2026-11-24 14:41:17 local, valid";
    in.time_line = "valid, source ntp, last sync 12 min ago, correction -0.184 s, zone <+03>-3";
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nclock: rtc 2026-11-24 14:41:17 local, valid\ntime: valid, source ntp, "
                                          "last sync 12 min ago, correction -0.184 s, zone <+03>-3\n"));
    in.time_line = "not valid (waiting for network), zone <+03>-3";
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\ntime: not valid (waiting for network), zone <+03>-3\n"));
    in.time_line = nullptr;
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NULL(std::strstr(buf, "\ntime: "));
}

static void test_mode_and_location_lines() {  // spec 004 contracts/serial-console.md, FR-024
    char buf[1024];
    ReportInput in = stage1_input();
    in.screen_line = "setup";
    in.mode_line = "setup, phones 1, last wrong_password";
    in.location_line = "İSTANBUL / SULTANBEYLİ (9549)";
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(
        buf, "\nscreen: setup\nmode: setup, phones 1, last wrong_password\nlocation: İSTANBUL / SULTANBEYLİ (9549)\n"
             "=== END ===\n"));
    in.mode_line = nullptr;
    in.location_line = nullptr;
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NULL(std::strstr(buf, "\nmode: "));
}

static void test_failed_and_skipped_carry_reasons() {
    char buf[1024];
    ReportInput in = stage1_input();
    set_result(in.tests[index(TestName::Touch)], Outcome::Failed, "GT911 not found at 0x5D");
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nselftest touch: failed (GT911 not found at 0x5D)\n"));
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nselftest display: skipped (not in this stage)\n"));
}

static void test_skipped_without_reason_still_says_why_missing() {
    SelfTestResult r;
    set_result(r, Outcome::Skipped, "");
    TEST_ASSERT_TRUE(r.outcome == Outcome::Skipped);
    TEST_ASSERT_EQUAL_STRING("no reason given", r.reason);
}

static void test_reason_is_truncated_and_ascii() {
    SelfTestResult r;
    set_result(r, Outcome::Failed,
               "a very long reason that is certainly longer than forty-seven characters\n\xC3\xA7");
    TEST_ASSERT_EQUAL_UINT32(sizeof r.reason - 1, std::strlen(r.reason));
    for (const char* p = r.reason; *p; ++p) {
        TEST_ASSERT_TRUE(*p >= 0x20 && *p < 0x7F);
    }
}

static void test_pending_is_reported() {
    char buf[1024];
    ReportInput in = stage1_input();
    in.tests[index(TestName::Storage)] = SelfTestResult{};
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nselftest storage: pending\n"));
}

static void test_board_line_configured_not_detected() {
    char buf[1024];
    const ReportInput in = stage1_input();
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nboard: DIS02170A V1.4 (configured; not detected)\n"));
}

static void test_audio_off_lines() {
    char buf[1024];
    ReportInput in = stage1_input();

    in.silence.acknowledged = false;
    in.silence.attempts = 20;
    std::strcpy(in.silence.reason, "no acknowledgement from 0x30");
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(
        std::strstr(buf, "\naudio-off: failed (no acknowledgement from 0x30, attempts 20)\n"));

    in.silence.acknowledged = true;
    in.silence.ran_first = false;
    in.silence.attempts = 3;
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\naudio-off: acknowledged, not first (attempts 3)\n"));

    in.silence = SilenceLine{};
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\naudio-off: not run\n"));
}

static void test_silence_line_alone_matches_report_line() {
    // The first serial line after start-up (plan startup step 2) is the report's audio-off line.
    char line[96];
    const ReportInput in = stage1_input();
    const size_t n = report::format_silence_line(in.silence, line, sizeof line);
    TEST_ASSERT_EQUAL_STRING("audio-off: acknowledged first (attempts 1)\n", line);
    TEST_ASSERT_EQUAL_UINT32(std::strlen(line), n);
}

static void test_audio_off_line_names_no_part() {
    // FR-026, SIL-05: `audio-off` covers the whole sequence and names no individual part.
    char buf[1024];
    const ReportInput in = stage1_input();
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NULL(std::strstr(buf, "amplifier"));
    TEST_ASSERT_NULL(std::strstr(buf, "speaker"));
}

static void test_backlight_full_dimming_disabled() {
    char buf[1024];
    ReportInput in = stage1_input();
    set_result(in.tests[index(TestName::Backlight)], Outcome::Failed, "byte 0 not acknowledged");
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nbacklight: full (dimming disabled)\n"));
}

static void test_memory_line_variants() {
    char buf[1024];
    ReportInput in = stage1_input();
    in.psram_bytes = 0;
    in.flash_bytes = 4u * 1024u * 1024u;
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nmemory: flash 4 MB, psram none (detected)\n"));
    in.flash_bytes = 0;
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nmemory: flash unknown, psram none (detected)\n"));
}

static void test_boot_to_diag_value() {
    char buf[1024];
    ReportInput in = stage1_input();
    in.boot_to_diag_ms = 2840;
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nboot-to-diag: 2840 ms\n"));
}

static void test_wifi_lines() {
    char buf[1024];
    ReportInput in = stage1_input();

    in.wifi.kind = WifiKind::NotSetUp;
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nwifi: not set up\n"));

    in.wifi.kind = WifiKind::Joining;
    std::strcpy(in.wifi.ssid, "EvAgi");
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nwifi: joining EvAgi\n"));

    in.wifi.kind = WifiKind::Connected;
    in.wifi.rssi_dbm = -54;
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nwifi: connected EvAgi -54 dBm\n"));

    in.wifi.kind = WifiKind::Hotspot;
    std::strcpy(in.wifi.ap_ssid, "NamazVakti-1A2B");
    std::strcpy(in.wifi.ap_address, "192.168.4.1");
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nwifi: hotspot NamazVakti-1A2B at 192.168.4.1\n"));
}

static void test_ssid_control_characters_cannot_break_lines() {
    char buf[1024];
    ReportInput in = stage1_input();
    in.wifi.kind = WifiKind::Joining;
    std::strcpy(in.wifi.ssid, "a\nb\rc");
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nwifi: joining a?b?c\n"));
}

static void test_touch_corners_line() {
    // contracts/serial-console.md: TL passed (+2,-1) TR not touched BL failed (+14,+3) BR passed (0,0)
    char buf[1024];
    ReportInput in = stage1_input();
    in.corners[0] = CornerResult{CornerState::Passed, 2, -1};
    in.corners[2] = CornerResult{CornerState::Failed, 14, 3};
    in.corners[3] = CornerResult{CornerState::Passed, 0, 0};
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(
        buf, "\ntouch corners: TL passed (+2,-1) TR not touched BL failed (+14,+3) BR passed (0,0)\n"));
}

static void test_reset_line_mentions_a_saved_core_dump() {
    char buf[1024];
    ReportInput in = stage1_input();
    in.reset_reason = "panic";
    in.coredump_bytes = 23552;
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nreset: panic, core dump saved (23552 B)\n"));
}

static void test_stress_lines() {
    // contracts/serial-console.md: stress: off | on 312 s cycles 31 scan 31/0 write 31/0 https 10/0/0 last-error none
    char buf[1024];
    ReportInput in = stage1_input();
    in.stress_running = true;
    in.stress.elapsed_s = 312;
    in.stress.cycles = 31;
    in.stress.scan_ok = 31;
    in.stress.write_ok = 31;
    in.stress.https_ok = 10;
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(
        buf, "\nstress: on 312 s cycles 31 scan 31/0 write 31/0 https 10/0/0 last-error none\n"));

    in.stress_paused = true;
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nstress: paused (hotspot open) 312 s cycles 31 "));

    in.stress_running = false;
    in.stress_paused = false;
    in.stress_has_run = true;
    in.stress.https_err = 1;
    std::strcpy(in.stress.last_error, "https: timeout");
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(
        buf, "\nstress: off, last run 312 s cycles 31 scan 31/0 write 31/0 https 10/1/0 last-error https: timeout\n"));
}

static void test_hotspot_line_never_has_its_password() {
    // FR-024, NET-08, tasks T070: the hotspot line has the name and address only.
    char buf[1024];
    ReportInput in = stage1_input();
    in.wifi.kind = WifiKind::Hotspot;
    std::strcpy(in.wifi.ap_ssid, "NamazVakti-1A2B");
    std::strcpy(in.wifi.ap_address, "192.168.4.1");
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "\nwifi: hotspot NamazVakti-1A2B at 192.168.4.1\n"));
    TEST_ASSERT_NULL(std::strstr(buf, "password"));
    TEST_ASSERT_NULL(std::strstr(buf, "pass:"));
}

static void test_report_never_contains_secret() {
    // FR-024, FR-026: ReportInput has no password field, so a password has nowhere to go.
    // The SSID may look like a password; it is the network name and is shown as such.
    const char* stored_password = "Gizli-Parola-2026!";
    char buf[1024];
    ReportInput in = stage1_input();
    in.wifi.kind = WifiKind::Connected;
    std::strcpy(in.wifi.ssid, "P@ssw0rd!123");
    in.wifi.rssi_dbm = -60;
    report::format(in, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(std::strstr(buf, "P@ssw0rd!123"));
    TEST_ASSERT_NULL(std::strstr(buf, stored_password));
    TEST_ASSERT_NULL(std::strstr(buf, "pass:"));
    TEST_ASSERT_NULL(std::strstr(buf, "password"));
}

static void test_small_buffer_is_truncated_safely() {
    char buf[32];
    std::memset(buf, 'X', sizeof buf);
    const ReportInput in = stage1_input();
    const size_t needed = report::format(in, buf, 20);
    TEST_ASSERT_EQUAL_UINT32(std::strlen(kStage1Report), needed);
    TEST_ASSERT_EQUAL_UINT32(19, std::strlen(buf));
    TEST_ASSERT_EQUAL_CHAR('X', buf[20]);
}

void run_selftest_report_tests() {
    RUN_TEST(test_report_has_contract_keys_in_order);
    RUN_TEST(test_clock_and_screen_lines);
    RUN_TEST(test_mode_and_location_lines);
    RUN_TEST(test_time_line);
    RUN_TEST(test_failed_and_skipped_carry_reasons);
    RUN_TEST(test_skipped_without_reason_still_says_why_missing);
    RUN_TEST(test_reason_is_truncated_and_ascii);
    RUN_TEST(test_pending_is_reported);
    RUN_TEST(test_board_line_configured_not_detected);
    RUN_TEST(test_audio_off_lines);
    RUN_TEST(test_silence_line_alone_matches_report_line);
    RUN_TEST(test_audio_off_line_names_no_part);
    RUN_TEST(test_backlight_full_dimming_disabled);
    RUN_TEST(test_memory_line_variants);
    RUN_TEST(test_boot_to_diag_value);
    RUN_TEST(test_wifi_lines);
    RUN_TEST(test_ssid_control_characters_cannot_break_lines);
    RUN_TEST(test_touch_corners_line);
    RUN_TEST(test_reset_line_mentions_a_saved_core_dump);
    RUN_TEST(test_stress_lines);
    RUN_TEST(test_hotspot_line_never_has_its_password);
    RUN_TEST(test_report_never_contains_secret);
    RUN_TEST(test_small_buffer_is_truncated_safely);
}
