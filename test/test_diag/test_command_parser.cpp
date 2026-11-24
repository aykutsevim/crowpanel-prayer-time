// Serial command parser (FR-028, contracts/serial-console.md). Written before the code.
#include <unity.h>

#include "nvp/diag/command.h"

using nvp::diag::Command;
using nvp::diag::parse_command;

static void test_every_contract_command_parses() {
    TEST_ASSERT_TRUE(parse_command("help") == Command::Help);
    TEST_ASSERT_TRUE(parse_command("report") == Command::Report);
    TEST_ASSERT_TRUE(parse_command("stress on") == Command::StressOn);
    TEST_ASSERT_TRUE(parse_command("stress off") == Command::StressOff);
    TEST_ASSERT_TRUE(parse_command("wifi setup") == Command::WifiSetup);
    TEST_ASSERT_TRUE(parse_command("wifi cancel") == Command::WifiCancel);
    TEST_ASSERT_TRUE(parse_command("wifi forget") == Command::WifiForget);
    TEST_ASSERT_TRUE(parse_command("diag") == Command::Diag);
    // Diagnostic one-shots used to isolate a display glitch (stack decision log, 2026-11-23).
    TEST_ASSERT_TRUE(parse_command("test write") == Command::TestWrite);
    TEST_ASSERT_TRUE(parse_command("TEST  SCAN") == Command::TestScan);
    TEST_ASSERT_TRUE(parse_command("test https") == Command::TestHttps);
    TEST_ASSERT_TRUE(parse_command("test mix") == Command::TestMix);
    TEST_ASSERT_TRUE(parse_command("test") == Command::Unknown);
}

static void test_case_and_whitespace_are_tolerated() {
    TEST_ASSERT_TRUE(parse_command("  REPORT ") == Command::Report);
    TEST_ASSERT_TRUE(parse_command("Stress   On") == Command::StressOn);
    TEST_ASSERT_TRUE(parse_command("\twifi\t forget  ") == Command::WifiForget);
}

static void test_carriage_return_is_ignored() {
    TEST_ASSERT_TRUE(parse_command("report\r") == Command::Report);
    TEST_ASSERT_TRUE(parse_command("stress off\r\n") == Command::StressOff);
}

static void test_empty_line_is_empty() {
    TEST_ASSERT_TRUE(parse_command("") == Command::Empty);
    TEST_ASSERT_TRUE(parse_command("   \r") == Command::Empty);
    TEST_ASSERT_TRUE(parse_command(nullptr) == Command::Empty);
}

static void test_unknown_and_absent_commands() {
    // There is deliberately no backlight command (OD1, FR-016, FR-028).
    TEST_ASSERT_TRUE(parse_command("bl") == Command::Unknown);
    TEST_ASSERT_TRUE(parse_command("bl 50") == Command::Unknown);
    TEST_ASSERT_TRUE(parse_command("dim") == Command::Unknown);
    TEST_ASSERT_TRUE(parse_command("stress") == Command::Unknown);
    TEST_ASSERT_TRUE(parse_command("stress onn") == Command::Unknown);
    TEST_ASSERT_TRUE(parse_command("reportx") == Command::Unknown);
    TEST_ASSERT_TRUE(parse_command("wifi setup now") == Command::Unknown);
}

static void test_overlong_line_is_unknown() {
    char line[300];
    for (char& c : line) c = 'a';
    line[sizeof line - 1] = '\0';
    TEST_ASSERT_TRUE(parse_command(line) == Command::Unknown);
}

void run_command_parser_tests() {
    RUN_TEST(test_every_contract_command_parses);
    RUN_TEST(test_case_and_whitespace_are_tolerated);
    RUN_TEST(test_carriage_return_is_ignored);
    RUN_TEST(test_empty_line_is_empty);
    RUN_TEST(test_unknown_and_absent_commands);
    RUN_TEST(test_overlong_line_is_unknown);
}
