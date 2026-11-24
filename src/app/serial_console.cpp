#include "serial_console.h"

#include "nvp/diag/command.h"
#include "platform/console_io.h"

namespace app {
namespace {

void call(void (*handler)()) {
    if (handler != nullptr) {
        handler();
    } else {
        platform::console::write("not available in this stage\n");
    }
}

}  // namespace

void serial_console_print_help() {
    // contracts/serial-console.md. There is deliberately no backlight command (OD1).
    platform::console::write(
        "commands:\n"
        "  help          this list\n"
        "  report        print the report\n"
        "  stress on     start stress mode\n"
        "  stress off    stop stress mode and print the final counters\n"
        "  wifi setup    start the setup hotspot\n"
        "  wifi cancel   close the hotspot, keep stored details\n"
        "  wifi forget   erase stored Wi-Fi details and disconnect\n"
        "  diag          show the diagnostic screen\n"
        "  main          back to the main screen (stops stress mode)\n"
        "  test write    one 4 kB scratch write (diagnostic)\n"
        "  test scan     one Wi-Fi scan (diagnostic)\n"
        "  test https    one HTTPS request (diagnostic)\n"
        "  test mix      one scan with a write at its start (diagnostic)\n"
#if defined(NVP_DEV_TOOLS) && NVP_DEV_TOOLS
        "  time get                         read the board clock\n"
        "  time set YYYY-MM-DD HH:MM:SS     set the board clock (local time)\n"
        "  clock rtc                        follow the board clock\n"
        "  clock sim YYYY-MM-DD HH:MM:SS    simulated local time\n"
        "  clock speed N | pause | run      simulated clock, speed 1..3600\n"
        "  state ok|offline|calculated|no_time|no_data | cache_ending N | auto\n"
        "  fixture N | off                  reference fixture 1..14\n"
#endif
    );
}

void serial_console_poll(const ConsoleHandlers& handlers) {
    using nvp::diag::Command;
    char line[96];
    const platform::console::LineStatus status = platform::console::poll_line(line, sizeof line);
    if (status == platform::console::LineStatus::None) return;
    nvp::diag::ParsedCommand parsed;
    if (status != platform::console::LineStatus::Overflow) parsed = nvp::diag::parse_command_line(line);
    Command command = parsed.command;
#if !(defined(NVP_DEV_TOOLS) && NVP_DEV_TOOLS)
    // Release build: the development commands do not exist (SYS-06, spec 003 FR-018).
    switch (command) {
        case Command::TimeGet: case Command::TimeSet: case Command::ClockRtc: case Command::ClockSim:
        case Command::ClockSpeed: case Command::ClockPause: case Command::ClockRun: case Command::StateForce:
        case Command::StateAuto: case Command::Fixture: case Command::FixtureOff: case Command::InvalidArgument:
            command = Command::Unknown;
            break;
        default: break;
    }
#endif
    switch (command) {
        case Command::Empty: break;
        case Command::Help: serial_console_print_help(); break;
        case Command::Report: call(handlers.report); break;
        case Command::StressOn: call(handlers.stress_on); break;
        case Command::StressOff: call(handlers.stress_off); break;
        case Command::WifiSetup: call(handlers.wifi_setup); break;
        case Command::WifiCancel: call(handlers.wifi_cancel); break;
        case Command::WifiForget: call(handlers.wifi_forget); break;
        case Command::TestWrite: call(handlers.test_write); break;
        case Command::TestScan: call(handlers.test_scan); break;
        case Command::TestHttps: call(handlers.test_https); break;
        case Command::TestMix: call(handlers.test_mix); break;
        case Command::Diag:
        case Command::Main:
        case Command::TimeGet:
        case Command::TimeSet:
        case Command::ClockRtc:
        case Command::ClockSim:
        case Command::ClockSpeed:
        case Command::ClockPause:
        case Command::ClockRun:
        case Command::StateForce:
        case Command::StateAuto:
        case Command::Fixture:
        case Command::FixtureOff:
            if (handlers.screen_command != nullptr) {
                handlers.screen_command(parsed);
            } else {
                platform::console::write("not available in this stage\n");
            }
            break;
        case Command::InvalidArgument: platform::console::write("invalid argument, type help\n"); break;
        case Command::Unknown: platform::console::write("unknown command, type help\n"); break;
    }
}

}  // namespace app
