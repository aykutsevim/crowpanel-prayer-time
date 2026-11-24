#include "console_io.h"

#include <driver/uart.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "sysinfo.h"

namespace platform::console {
namespace {

constexpr int kBaud = 115200;
constexpr uart_port_t kPort = UART_NUM_0;  // CH340K bridge (research R2)
bool g_started = false;
char g_line[96];
size_t g_len = 0;
bool g_overflow = false;

}  // namespace

void begin() {
    sysinfo::note_peripheral_init();
    // UART0 is already set to 115200 8N1 by the ROM and ESP-IDF; the driver adds receive buffering.
    uart_config_t cfg = {};
    cfg.baud_rate = kBaud;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;
    g_started = uart_driver_install(kPort, 512, 0, 0, nullptr, 0) == ESP_OK && uart_param_config(kPort, &cfg) == ESP_OK;
}

LineStatus poll_line(char* out, size_t cap) {
    if (!g_started) return LineStatus::None;
    uint8_t byte = 0;
    while (uart_read_bytes(kPort, &byte, 1, 0) == 1) {
        const int c = byte;
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

void write(const char* text) {
    if (g_started) uart_write_bytes(kPort, text, std::strlen(text));
}

void printf(const char* fmt, ...) {
    char buf[160];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);
    write(buf);
}

}  // namespace platform::console
