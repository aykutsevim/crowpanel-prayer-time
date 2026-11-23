// Backlight: fully on, never dimmed (owner decision OD1, 2026-11-20; FR-016, FR-017, FR-018).
//
// The ESP32 has no backlight pin. The backlight driver is switched by the auxiliary controller,
// whose dimming frequency is undocumented (research R6), so the only command ever sent is
// byte 0 ("full on"), through the command gate in audio_off.cpp. There is no other level and no
// other backlight function.
#pragma once

namespace platform::backlight {

// Sends "full on" once. Returns true when the auxiliary controller acknowledged it.
bool full_on();

}  // namespace platform::backlight
