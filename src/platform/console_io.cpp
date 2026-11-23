#include "console_io.h"

#include <Arduino.h>

#include <cstdarg>
#include <cstdio>

#include "sysinfo.h"

namespace platform::console {
namespace {

constexpr unsigned long kBaud = 115200;
char g_line[96];
size_t g_len = 0;
bool g_overflow = false;

}  // namespace

void begin() {
    sysinfo::note_peripheral_init();
    Serial.begin(kBaud);  // UART0; USB-CDC is off (ARDUINO_USB_CDC_ON_BOOT=0, research R2)
}

LineStatus poll_line(char* out, size_t cap) {
    while (Serial.available() > 0) {
        const int c = Serial.read();
        if (c < 0) break;
        if (c == '\n') {
            const bool overflow = g_overflow;
            g_line[g_len] = '\0';
            g_len = 0;
            g_overflow = false;
            if (overflow) return LineStatus::Overflow;
            std::snprintf(out, cap, "%s", g_line);
            return LineStatus::Line;
        }
        if (g_len + 1 < sizeof g_line) {
            g_line[g_len++] = static_cast<char>(c);
        } else {
            g_overflow = true;
        }
    }
    return LineStatus::None;
}

void write(const char* text) { Serial.print(text); }

void printf(const char* fmt, ...) {
    char buf[160];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);
    Serial.print(buf);
}

}  // namespace platform::console
