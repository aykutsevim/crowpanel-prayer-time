// System information for the report (FR-026): heap, PSRAM, detected memory sizes, MAC, reset
// reason and uptime. Also counts peripheral start-ups so the silence step can prove it ran
// first (tasks T024, SIL-02).
#pragma once

#include <cstdint>

namespace platform::sysinfo {

struct Snapshot {
    uint32_t internal_free = 0;
    uint32_t internal_largest = 0;
    uint32_t psram_free = 0;
    uint32_t flash_bytes = 0;  // physical flash chip size read from the chip, 0 if unknown
    uint32_t psram_bytes = 0;  // PSRAM size found at boot, 0 if none
    uint8_t mac[6] = {};       // station MAC
    const char* reset_reason = "unknown";
};

// Reads the values that never change (flash and PSRAM size, MAC, reset reason) once. Call it
// before the display starts: the flash ID read must not run while the panel scans out of PSRAM.
void init();

Snapshot read();
uint32_t uptime_ms();

// Every platform start-up function except the silence step calls this before it touches its
// peripheral. The silence step reads the count: zero means it really ran first.
void note_peripheral_init();
uint32_t peripheral_inits();

}  // namespace platform::sysinfo
