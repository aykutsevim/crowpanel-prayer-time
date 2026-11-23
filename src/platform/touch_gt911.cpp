#include "touch_gt911.h"

#include <Arduino.h>
#include <Wire.h>

#include <cstdio>
#include <cstring>

#include "audio_off.h"
#include "board.h"
#include "sysinfo.h"

namespace platform::touch {
namespace {

constexpr uint16_t kRegProductId = 0x8140;
constexpr uint16_t kRegStatus = 0x814E;
constexpr uint16_t kRegPoint1 = 0x8150;
constexpr int kStartAttempts = 10;

bool g_ready = false;
bool g_pressed = false;
int16_t g_x = 0;
int16_t g_y = 0;

bool probe(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

bool read_regs(uint16_t reg, uint8_t* out, size_t n) {
    Wire.beginTransmission(board::kGt911Address);
    Wire.write(static_cast<uint8_t>(reg >> 8));
    Wire.write(static_cast<uint8_t>(reg & 0xFF));
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(static_cast<uint8_t>(board::kGt911Address), static_cast<uint8_t>(n)) != n) return false;
    for (size_t i = 0; i < n; ++i) out[i] = static_cast<uint8_t>(Wire.read());
    return true;
}

bool write_reg(uint16_t reg, uint8_t value) {
    Wire.beginTransmission(board::kGt911Address);
    Wire.write(static_cast<uint8_t>(reg >> 8));
    Wire.write(static_cast<uint8_t>(reg & 0xFF));
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

}  // namespace

Result begin() {
    Result r;
    sysinfo::note_peripheral_init();
    // The I2C bus was started by the silence step (SDA 15, SCL 16).
    bool found = false;
    for (int attempt = 1; attempt <= kStartAttempts; ++attempt) {
        if (probe(board::kAuxAddress) && probe(board::kGt911Address)) {
            found = true;
            break;
        }
        silence::aux_send(silence::kAuxTouchReset);  // 250: touch reset via the auxiliary controller
        pinMode(board::kTouchInt, OUTPUT);
        digitalWrite(board::kTouchInt, LOW);  // INT low while reset is released selects 0x5D
        delay(120);
        pinMode(board::kTouchInt, INPUT);
        delay(100);
    }
    if (!found) {
        std::snprintf(r.reason, sizeof r.reason, "GT911 not found at 0x%02X", board::kGt911Address);
        return r;
    }
    uint8_t id[4] = {};
    if (!read_regs(kRegProductId, id, sizeof id)) {
        std::snprintf(r.reason, sizeof r.reason, "product ID read failed");
        return r;
    }
    if (std::memcmp(id, "911", 3) != 0) {
        std::snprintf(r.reason, sizeof r.reason, "unexpected product ID %02X%02X%02X%02X", id[0], id[1], id[2],
                      id[3]);
        return r;
    }
    g_ready = true;
    r.ok = true;
    return r;
}

bool read_point(int16_t* x, int16_t* y) {
    if (!g_ready) return false;
    uint8_t status = 0;
    if (read_regs(kRegStatus, &status, 1) && (status & 0x80) != 0) {
        const uint8_t points = status & 0x0F;
        if (points > 0) {
            uint8_t p[4] = {};
            if (read_regs(kRegPoint1, p, sizeof p)) {
                g_x = static_cast<int16_t>(p[0] | (p[1] << 8));
                g_y = static_cast<int16_t>(p[2] | (p[3] << 8));
                g_pressed = true;
            }
        } else {
            g_pressed = false;
        }
        // Hand the buffer back to the controller (status register is R/W; Goodix convention).
        write_reg(kRegStatus, 0);
    }
    *x = g_x;
    *y = g_y;
    return g_pressed;
}

}  // namespace platform::touch
