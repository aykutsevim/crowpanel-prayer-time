// Serial command dispatch (FR-028, contracts/serial-console.md). Reads lines through
// platform::console, parses them with the pure nvp::diag::parse_command and calls the handlers.
// No Arduino, ESP-IDF or FreeRTOS header here (constitution VI).
#pragma once

namespace app {

struct ConsoleHandlers {
    void (*report)() = nullptr;
    void (*stress_on)() = nullptr;
    void (*stress_off)() = nullptr;
    void (*wifi_setup)() = nullptr;
    void (*wifi_cancel)() = nullptr;
    void (*wifi_forget)() = nullptr;
    void (*test_write)() = nullptr;
    void (*test_scan)() = nullptr;
    void (*test_https)() = nullptr;
    void (*test_mix)() = nullptr;
};

// Call from loop(): handles at most one complete line per call.
void serial_console_poll(const ConsoleHandlers& handlers);

void serial_console_print_help();

}  // namespace app
