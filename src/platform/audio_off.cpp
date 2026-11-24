// Silence step for the CrowPanel Advance V1.4 (research R3; SIL-02, FR-001, FR-001a, FR-002).
//
// No audio peripheral is ever started: no I2S driver, no tone, no DAC, no PWM. The ESP32 ends of
// the I2S data lines are only driven low as plain GPIO, and the auxiliary controller is told to
// mute the amplifier (249) and keep the buzzer off (247). Commands 246 (buzzer on) and 248
// (amplifier on) can never be sent: aux_send() checks every byte against the gate.
#include "audio_off.h"

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstdio>

#include "board.h"
#include "console_io.h"
#include "i2c_bus.h"
#include "sysinfo.h"

#if defined(NVP_DEBUG)
#include <cassert>
#endif

namespace platform::silence {
namespace {

SilenceState g_state;
bool g_console_ready = false;

bool write_byte(uint8_t b) { return i2c::write(board::kAuxAddress, &b, 1); }  // true: acknowledged

}  // namespace

bool aux_send(uint8_t b) {
    if (!aux_byte_allowed(b)) {
        if (g_state.rejected_count < 255) ++g_state.rejected_count;
        g_state.last_rejected = b;
        if (g_console_ready) {
            console::printf("aux: rejected byte %u\n", static_cast<unsigned>(b));
        }
        return false;
    }
    return write_byte(b);
}

void audio_off() {
    // Truthful "ran first": no other platform start-up may have run (tasks T024). ESP-IDF's own
    // start-up before app_main() starts no audio peripheral.
    g_state.ran_first = sysinfo::peripheral_inits() == 0;
#if defined(NVP_DEBUG)
    assert(g_state.ran_first && "audio_off() must be the first platform call");
#endif

    // 1. Park the ESP32 ends of the I2S lines (GPIO4 data, GPIO5 bit clock, GPIO6 word clock).
    for (int pin : board::kAmpDataPins) {
        const gpio_num_t g = static_cast<gpio_num_t>(pin);
        gpio_reset_pin(g);
        gpio_set_level(g, 0);
        gpio_set_direction(g, GPIO_MODE_OUTPUT);
        gpio_set_level(g, 0);
    }

    // 2. Start the shared I2C bus (SDA 15, SCL 16).
    if (!i2c::begin()) {
        g_state.ran = true;
        g_state.acknowledged = false;
        std::snprintf(g_state.reason, sizeof g_state.reason, "I2C start failed");
        return;
    }

    // 3. Amplifier mute, then buzzer off, until both are acknowledged in one attempt.
    for (uint8_t attempt = 1; attempt <= kSilenceMaxAttempts; ++attempt) {
        g_state.attempts = attempt;
        bool all = true;
        for (uint8_t b : kSilenceSequence) {
            all = aux_send(b) && all;
        }
        if (all) {
            g_state.acknowledged = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(kSilenceRetryMs));
    }
    g_state.ran = true;
    if (!g_state.acknowledged) {
        std::snprintf(g_state.reason, sizeof g_state.reason, "no acknowledgement from 0x%02X",
                      static_cast<unsigned>(board::kAuxAddress));
    }
}

const SilenceState& silence_state() { return g_state; }

void on_console_ready() {
    g_console_ready = true;
    if (g_state.rejected_count > 0) {
        console::printf("aux: %u byte(s) rejected before the console started, last %u\n",
                        static_cast<unsigned>(g_state.rejected_count),
                        static_cast<unsigned>(g_state.last_rejected));
    }
}

}  // namespace platform::silence
