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
        "  diag          show the diagnostic screen\n");
}

void serial_console_poll(const ConsoleHandlers& handlers) {
    using nvp::diag::Command;
    char line[96];
    const platform::console::LineStatus status = platform::console::poll_line(line, sizeof line);
    if (status == platform::console::LineStatus::None) return;
    const Command command = status == platform::console::LineStatus::Overflow
                                ? Command::Unknown
                                : nvp::diag::parse_command(line);
    switch (command) {
        case Command::Empty: break;
        case Command::Help: serial_console_print_help(); break;
        case Command::Report: call(handlers.report); break;
        case Command::StressOn: call(handlers.stress_on); break;
        case Command::StressOff: call(handlers.stress_off); break;
        case Command::WifiSetup: call(handlers.wifi_setup); break;
        case Command::WifiCancel: call(handlers.wifi_cancel); break;
        case Command::WifiForget: call(handlers.wifi_forget); break;
        // Slice 1 has no product screen: the device is always in the diagnostic mode (FR-007a).
        case Command::Diag: platform::console::write("already in diagnostic mode\n"); break;
        case Command::Unknown: platform::console::write("unknown command, type help\n"); break;
    }
}

}  // namespace app
