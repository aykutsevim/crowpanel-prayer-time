// The shared I2C bus (SDA 15, SCL 16, 100 kHz; research R3, R5): auxiliary controller 0x30 and
// GT911 0x5D. Internal to src/platform. The silence step opens it first (SIL-02).
#pragma once

#include <cstddef>
#include <cstdint>

namespace platform::i2c {

// Starts the bus once. Returns false if the driver could not start.
bool begin();

// True when a device acknowledges its address.
bool probe(uint8_t address);

// Plain write of `n` bytes. True when acknowledged.
bool write(uint8_t address, const uint8_t* data, size_t n);

// Write then read with a repeated start (16-bit register reads on the GT911).
bool write_read(uint8_t address, const uint8_t* wdata, size_t wn, uint8_t* rdata, size_t rn);

}  // namespace platform::i2c
