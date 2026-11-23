// Board profile: Elecrow CrowPanel Advance 7.0", DIS02170A, hardware revision V1.4,
// ESP32-S3-WROOM-1-N16R8 (FR-029, constitution V).
//
// Every value is cited. Source keys are defined in specs/001-silent-board-bringup/research.md:
//   REPO   Elecrow CrowPanel-Advance-7 repository, commit 70db9eda (2026-08-21)
//   EX     REPO/example/V1.3_and_V1.4_and_V1.5/
//   SCH14  REPO/Eagle_SCH&PCB/version1.4/ (schematic and netlist)
//   WIKI   Elecrow wiki page for the board
// Nothing here is taken from the V1.3 documentation alone or guessed (CLAUDE.md rule 4).
#pragma once

#include <cstdint>

namespace board {

// --- Identity (owner, read from the board on 2026-11-20; I1 2026-11-23) ---
inline constexpr const char* kModel = "DIS02170A";
inline constexpr const char* kRevision = "V1.4";  // configured; the firmware cannot read it

// --- Shared I2C bus: auxiliary controller, GT911 touch, RTC (research R3, R5; SCH14) ---
inline constexpr int kI2cSda = 15;
inline constexpr int kI2cScl = 16;
inline constexpr uint32_t kI2cHz = 100000;  // the silence step runs at 100 kHz (tasks T023)

// --- Auxiliary controller STC8H1K28 (U13), single-byte commands (research R3; REPO readme,
//     EX lesson-02/-10, factory source HMI-bigInch7.ino) ---
inline constexpr uint8_t kAuxAddress = 0x30;

// --- ESP32 ends of the digital audio data lines to the amplifier: GPIO4 data, GPIO5 bit
//     clock, GPIO6 word clock (research R3.1; EX `#define ... 4 / 5 / 6`; SCH14 U11 channel 0).
//     Driven low by the silence step only (SIL-02). ---
inline constexpr int kAmpDataPins[3] = {4, 5, 6};

// --- Touch GT911 (research R5; EX lesson-03 LovyanGFX_Driver.h; SCH14 IO1_TP_INT) ---
inline constexpr uint8_t kGt911Address = 0x5D;
inline constexpr int kTouchInt = 1;  // reset is via the auxiliary controller (command 250)

// --- RGB panel 800x480, SC7277 (research R4; EX lesson-03 LovyanGFX_Driver.h, factory source,
//     EX IDF_Code Lesson03 waveshare_rgb_lcd_port.h, SCH14 net names) ---
inline constexpr int kLcdWidth = 800;
inline constexpr int kLcdHeight = 480;
inline constexpr int kLcdData[16] = {
    21, 47, 48, 45, 38,      // B0..B4
    9, 10, 11, 12, 13, 14,   // G0..G5
    7, 17, 18, 3, 46,        // R0..R4
};
inline constexpr int kLcdDe = 42;
inline constexpr int kLcdVsync = 41;
inline constexpr int kLcdHsync = 40;
inline constexpr int kLcdPclk = 39;
inline constexpr uint32_t kPclkHz = 16000000;  // IDF example, lesson-03/-05; 39 Hz refresh (R4, R8 L0)
inline constexpr int kHsyncFrontPorch = 8;
inline constexpr int kHsyncPulseWidth = 4;
inline constexpr int kHsyncBackPorch = 8;
inline constexpr int kVsyncFrontPorch = 8;
inline constexpr int kVsyncPulseWidth = 4;
inline constexpr int kVsyncBackPorch = 8;
inline constexpr bool kHsyncIdleLow = false;   // polarities 0
inline constexpr bool kVsyncIdleLow = false;
inline constexpr bool kPclkActiveNeg = true;   // data sampled on the falling edge (IDF example)

// The ESP32 has no backlight pin: the backlight driver (MT9201) is controlled by the auxiliary
// controller only (research R6; IDF example EXAMPLE_PIN_NUM_BK_LIGHT = -1).

}  // namespace board
