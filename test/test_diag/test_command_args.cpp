// Serial commands with arguments (spec 003 FR-018, FR-022, FR-029, FR-030;
// specs/003-main-screen/contracts/serial-console.md). Written before the code.
#include <unity.h>

#include <cstring>

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
    p = parse_command_line("fixture 17");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::Fixture), static_cast<int>(p.command));
    TEST_ASSERT_EQUAL_INT(17, p.number);
}

static void test_invalid_arguments() {
    expect_cmd("clock speed 0", Command::InvalidArgument);
    expect_cmd("clock speed 3601", Command::InvalidArgument);
    expect_cmd("clock speed fast", Command::InvalidArgument);
    expect_cmd("fixture 0", Command::InvalidArgument);
    expect_cmd("fixture 18", Command::InvalidArgument);  // 15-17: spec 004 setup screens (FR-009)
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

static void test_time_commands() {  // spec 005 contracts/serial-console.md
    expect_cmd("tz", Command::TzGet);
    expect_cmd("time sync", Command::TimeSync);
    expect_cmd("NTP off", Command::NtpOff);
    expect_cmd("ntp on", Command::NtpOn);
    expect_cmd("clock invalid", Command::ClockInvalid);
    ParsedCommand p = parse_command_line("tz CET-1CEST,M3.5.0,M10.5.0/3");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::TzSet), static_cast<int>(p.command));
    TEST_ASSERT_EQUAL_STRING("CET-1CEST,M3.5.0,M10.5.0/3", p.text);  // case kept: the zone is case-sensitive
    p = parse_command_line("  tz   <+03>-3  ");
    TEST_ASSERT_EQUAL_STRING("<+03>-3", p.text);
    p = parse_command_line("time jitter 600");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::TimeJitter), static_cast<int>(p.command));
    TEST_ASSERT_EQUAL_INT(600, p.number);
    TEST_ASSERT_EQUAL_INT(10, parse_command_line("time jitter 10").number);
    TEST_ASSERT_EQUAL_INT(3600, parse_command_line("time jitter 3600").number);
    expect_cmd("time jitter 9", Command::InvalidArgument);
    expect_cmd("time jitter 3601", Command::InvalidArgument);
    expect_cmd("time jitter", Command::InvalidArgument);
    char long_zone[80];
    std::memset(long_zone, 'A', sizeof long_zone);
    std::memcpy(long_zone, "tz ", 3);
    long_zone[79] = '\0';
    expect_cmd(long_zone, Command::InvalidArgument);  // over 63 bytes
}

static void test_data_commands() {  // spec 006 contracts/serial-console.md, FR-016
    expect_cmd("data refresh", Command::DataRefresh);
    expect_cmd("  DATA   show ", Command::DataShow);
    expect_cmd("data corrupt", Command::DataCorrupt);
    expect_cmd("data erase", Command::DataErase);
    expect_cmd("data", Command::Unknown);
    expect_cmd("data wipe", Command::Unknown);
    // Spec 007 (FR-022).
    expect_cmd("data plan", Command::DataPlan);
    expect_cmd("DATA calib", Command::DataCalib);
    ParsedCommand p = nvp::diag::parse_command_line("data block emushaf");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::DataBlock), static_cast<int>(p.command));
    TEST_ASSERT_EQUAL_INT(2, p.number);
    p = nvp::diag::parse_command_line("data block  ALADHAN ");
    TEST_ASSERT_EQUAL_INT(3, p.number);
    p = nvp::diag::parse_command_line("data unblock all");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Command::DataUnblock), static_cast<int>(p.command));
    TEST_ASSERT_EQUAL_INT(0, p.number);
    expect_cmd("data block all", Command::InvalidArgument);
    expect_cmd("data block", Command::InvalidArgument);
    expect_cmd("data block diyanet", Command::InvalidArgument);
}

void run_command_args_tests() {
    RUN_TEST(test_data_commands);
    RUN_TEST(test_time_commands);
    RUN_TEST(test_plain_commands_still_parse);
    RUN_TEST(test_time_set);
    RUN_TEST(test_clock_sim_and_speed);
    RUN_TEST(test_state_and_fixture);
    RUN_TEST(test_invalid_arguments);
}
