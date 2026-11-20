// Namaz Vakti Panel firmware entry point.
//
// Slice 0: prints a version line only. No display, touch, Wi-Fi or other peripheral code.
//
// Slice 1 adds, as the very first call in setup(), the platform function that shuts the
// audio amplifier down and parks the audio data pins (SIL-02). Its pins are not yet verified
// against the vendor schematic, so it is deliberately absent here (CLAUDE.md rule 4).
#include <Arduino.h>

#include "nvp/version.h"

#ifndef NVP_BOARD_NAME
#define NVP_BOARD_NAME "unknown"
#endif

void setup() {
    Serial.begin(115200);
    Serial.printf("NamazVaktiPanel %s (%s)\n", nvp::kFirmwareVersion, NVP_BOARD_NAME);
}

void loop() {
    delay(1000);
}
