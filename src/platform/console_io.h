// Serial console byte I/O on UART0 through the CH340K bridge, 115200 8N1 (research R2,
// contracts/serial-console.md). The only serial code in the firmware; src/app calls it
// (constitution VI). Never pass a password to it (FR-024, SEC-04).
#pragma once

#include <cstddef>

namespace platform::console {

void begin();  // UART0 at 115200

enum class LineStatus { None, Line, Overflow };

// Non-blocking. Collects bytes until '\n'. Returns Line with the text (without the line end) in
// `out`, Overflow when the line did not fit (the rest of it was discarded), or None.
LineStatus poll_line(char* out, size_t cap);

void write(const char* text);
void printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

}  // namespace platform::console
