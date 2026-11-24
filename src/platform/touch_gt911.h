// GT911 touch controller at I2C 0x5D on the shared bus (research R5, FR-015).
//
// Register map from the vendor's "GT911 Datasheet - English 20130827.pdf" (REPO/Datasheet):
// product ID 0x8140..0x8143 (ASCII), status 0x814E (R/W: bit 7 buffer ready, bits 0..3 number of
// points), point 1 at 0x8150..0x8153 (x low, x high, y low, y high). 16-bit register addresses,
// high byte first.
#pragma once

#include <cstdint>

namespace platform::touch {

struct Result {
    bool ok = false;
    char reason[48] = {};
};

// Vendor start-up sequence, bounded to 10 attempts (research R5): while 0x30 or 0x5D does not
// answer, send command 250 to the auxiliary controller, hold GPIO1 low for 120 ms, release it and
// wait 100 ms. Then reads the product ID, which must start with "911".
Result begin();

// Reads the first touch point. Returns true while a finger is down. Keeps the last state when
// the controller has no new data. Never produces sound or vibration (SIL-06).
bool read_point(int16_t* x, int16_t* y);

// Number of touch points the controller reported at the last read_point() (status register
// 0x814E bits 3..0; spec 003 research R13). Used for the hidden two-finger gesture.
uint8_t last_points();

}  // namespace platform::touch
