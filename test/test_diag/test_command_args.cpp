// Serial commands with arguments (spec 003 FR-018, FR-022, FR-029, FR-030;
// specs/003-main-screen/contracts/serial-console.md). Written before the code.
#include <unity.h>

#include "nvp/diag/command.h"

using nvp::diag::Command;
using nvp::diag::ParsedCommand;
using nvp::diag::parse_command_line;

static void expect_cmd(const char* line, Command c) {
    const ParsedCommand p = parse_command_line(line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(static_cast<int>(c), static_cast<int>(p.command), line);
}

static void test_plain_commands_still_parse() {
    expect_cmd("report", Command::Report);
    expect_cmd("diag", Command::Diag);
    expect_cmd("  MAIN ", Command::Main);
    expect_cmd("time get", Command::TimeGet);
    expect_cmd("clock rtc", Command::ClockRtc);
    expect_cmd("clock pause", Command::ClockPause);
    expect_cmd("clock run", Command::ClockRun);
    expect_cmd("state auto", Command::StateAuto);
    expect_cmd("fixture off", Command::FixtureOff);
    expect_cmd("", Command::Empty);
    expect_cmd("bogus", Command::Unknown);
}

static void test_time_set() {
    const ParsedCommand p = parse_command_line("time set 2026-11-24 12:30:05");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::TimeSet), static_cast<int>(p.command));
    TEST_ASSERT_EQUAL_INT(2026, p.year);
    TEST_ASSERT_EQUAL_INT(11, p.month);
    TEST_ASSERT_EQUAL_INT(24, p.day);
    TEST_ASSERT_EQUAL_INT(12, p.hour);
    TEST_ASSERT_EQUAL_INT(30, p.minute);
    TEST_ASSERT_EQUAL_INT(5, p.second);
}

static void test_clock_sim_and_speed() {
    ParsedCommand p = parse_command_line("Clock Sim 2026-11-17 00:00:00");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::ClockSim), static_cast<int>(p.command));
    TEST_ASSERT_EQUAL_INT(17, p.day);
    p = parse_command_line("clock speed 3600");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::ClockSpeed), static_cast<int>(p.command));
    TEST_ASSERT_EQUAL_INT(3600, p.number);
}

static void test_state_and_fixture() {
    ParsedCommand p = parse_command_line("state offline");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::StateForce), static_cast<int>(p.command));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::screen::SystemState::Offline), static_cast<int>(p.state));
    p = parse_command_line("state cache_ending 5");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::StateForce), static_cast<int>(p.command));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::screen::SystemState::CacheEnding), static_cast<int>(p.state));
    TEST_ASSERT_EQUAL_INT(5, p.number);
    p = parse_command_line("fixture 14");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Fixture), static_cast<int>(p.command));
    TEST_ASSERT_EQUAL_INT(14, p.number);
}

static void test_invalid_arguments() {
    expect_cmd("clock speed 0", Command::InvalidArgument);
    expect_cmd("clock speed 3601", Command::InvalidArgument);
    expect_cmd("clock speed fast", Command::InvalidArgument);
    expect_cmd("fixture 0", Command::InvalidArgument);
    expect_cmd("fixture 15", Command::InvalidArgument);
    expect_cmd("time set 2026-13-01 00:00:00", Command::InvalidArgument);
    expect_cmd("time set 2026-02-29 00:00:00", Command::InvalidArgument);
    expect_cmd("time set 2026-11-24 24:00:00", Command::InvalidArgument);
    expect_cmd("time set 2026-11-24", Command::InvalidArgument);
    expect_cmd("time set 2100-01-01 00:00:00", Command::InvalidArgument);  // 2000..2099 only
    expect_cmd("time set 26-11-24 12:00:00", Command::InvalidArgument);
    expect_cmd("state cache_ending 8", Command::InvalidArgument);  // 0..7
    expect_cmd("state cache_ending", Command::InvalidArgument);
    expect_cmd("state offline 3", Command::InvalidArgument);
    expect_cmd("state nonsense", Command::InvalidArgument);
    expect_cmd("clock sim tomorrow", Command::InvalidArgument);
}

void run_command_args_tests() {
    RUN_TEST(test_plain_commands_still_parse);
    RUN_TEST(test_time_set);
    RUN_TEST(test_clock_sim_and_speed);
    RUN_TEST(test_state_and_fixture);
    RUN_TEST(test_invalid_arguments);
}
