// The board's battery-backed real-time clock: BM8563 (U4 of the DIS02170A V1.4 schematic,
// PCF8563-compatible; parts list alternative PCF8563MDTR), on the shared I2C bus (SDA 15, SCL 16)
// at 0x51, battery BT1 (CR1220), interrupt pin not connected. Spec 003 OD3-1, FR-026, research R1.
//
// Registers 02h..08h are read or written in one access, as the NXP PCF8563 data sheet (Rev. 11.1,
// §8.5) requires; encoding and validity live in lib/core (nvp::time::rtc_codec). Call only after
// the silence step has started the bus.
#pragma once

#include <cstdint>

namespace platform::rtc {

// Reads the seven time registers. False when the chip does not acknowledge.
bool read(uint8_t raw[7]);

// Writes the seven time registers. False when the chip does not acknowledge.
bool write(const uint8_t raw[7]);

}  // namespace platform::rtc
