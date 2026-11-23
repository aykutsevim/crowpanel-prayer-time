// Silence: the first action after reset (SIL-01, SIL-02, MODE-01, FR-001, FR-001a).
//
// On the CrowPanel Advance V1.4 the amplifier control, the buzzer and the backlight driver are
// reachable only through the auxiliary controller STC8H1K28 at I2C 0x30, which takes
// single-byte commands (research R3). This file is the only one allowed to name audio parts
// (tools/check_silence.py), and audio_off.cpp holds the only writer to 0x30.
//
// The top half is pure and host-tested (test/test_audio_off). The platform half is compiled on
// the device only.
#pragma once

#include <cstdint>

namespace platform::silence {

// --- Command bytes (research R3; contracts/aux-controller.md) ---
inline constexpr uint8_t kAuxBacklightFull = 0;  // backlight fully on (the only level, OD1)
inline constexpr uint8_t kAuxBuzzerOff = 247;
inline constexpr uint8_t kAuxAmpMute = 249;      // amplifier mute; the factory firmware sends it first
inline constexpr uint8_t kAuxTouchReset = 250;   // GT911 reset sequence

// The silence sequence, in order: amplifier mute, then buzzer off.
inline constexpr uint8_t kSilenceSequence[2] = {kAuxAmpMute, kAuxBuzzerOff};

// Bounded retries: the auxiliary controller may start later than the ESP32 (research R3.1).
inline constexpr uint8_t kSilenceMaxAttempts = 20;
inline constexpr uint32_t kSilenceRetryMs = 50;

// The command gate. Allows only 0 (backlight full), 247 (buzzer off), 249 (amplifier mute) and
// 250 (touch reset). Rejects 246 (buzzer on), 248 (amplifier on), every dimming value 1..245
// and the undocumented 251..255 (FR-003, FR-018, OD1).
constexpr bool aux_byte_allowed(uint8_t b) {
    return b == kAuxBacklightFull || b == kAuxBuzzerOff || b == kAuxAmpMute || b == kAuxTouchReset;
}

// What the silence step did (data-model.md SilenceState). "acknowledged" means the controller
// received the bytes; it does not prove silence, which only the owner's ears can (research R3.2).
struct SilenceState {
    bool ran = false;
    bool acknowledged = false;  // both 249 and 247 acknowledged in the same attempt
    bool ran_first = false;     // no other peripheral had been started before it
    uint8_t attempts = 0;       // 1..kSilenceMaxAttempts
    uint8_t rejected_count = 0; // bytes the gate refused (saturates at 255)
    uint8_t last_rejected = 0;
    char reason[48] = {};
};

#ifdef ARDUINO
// Parks the audio data lines low, starts I2C and sends the silence sequence with bounded
// retries. Must be the first statement of setup().
void audio_off();

// The only writer to the auxiliary controller. Refuses any byte the gate rejects, records it and
// logs `aux: rejected byte N` once the serial console is up. Returns true when acknowledged.
bool aux_send(uint8_t b);

const SilenceState& silence_state();

// Called once the serial console is up: from then on refusals are logged at once, and any
// recorded earlier are printed now (plan startup step 2).
void on_console_ready();
#endif

}  // namespace platform::silence
