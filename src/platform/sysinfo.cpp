#include "sysinfo.h"

#include <esp_core_dump.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_mac.h>
#include <esp_psram.h>
#include <esp_system.h>
#include <esp_timer.h>

namespace platform::sysinfo {
namespace {

uint32_t g_inits = 0;

const char* reset_reason_name(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_POWERON: return "power-on";
        case ESP_RST_EXT: return "external pin";
        case ESP_RST_SW: return "software";
        case ESP_RST_PANIC: return "panic";
        case ESP_RST_INT_WDT: return "interrupt watchdog";
        case ESP_RST_TASK_WDT: return "task watchdog";
        case ESP_RST_WDT: return "other watchdog";
        case ESP_RST_DEEPSLEEP: return "deep sleep";
        case ESP_RST_BROWNOUT: return "brownout";
        case ESP_RST_SDIO: return "sdio";
        default: return "unknown";
    }
}

// Values that never change, read once. Reading the flash chip ID is a flash command on the bus
// it shares with PSRAM; repeated while the RGB panel scans out of PSRAM, it disturbs the picture
// (research R8, owner check D1 on 2026-11-23). So it happens once, before the display starts.
struct Fixed {
    bool read = false;
    uint32_t flash_bytes = 0;
    uint32_t psram_bytes = 0;
    uint8_t mac[6] = {};
    const char* reset_reason = "unknown";
    uint32_t coredump_bytes = 0;
};
Fixed g_fixed;

void read_fixed() {
    if (g_fixed.read) return;
    uint32_t flash = 0;
    if (esp_flash_get_physical_size(nullptr, &flash) == ESP_OK) g_fixed.flash_bytes = flash;
    g_fixed.psram_bytes = static_cast<uint32_t>(esp_psram_get_size());
    esp_read_mac(g_fixed.mac, ESP_MAC_WIFI_STA);
    g_fixed.reset_reason = reset_reason_name(esp_reset_reason());
    size_t addr = 0, size = 0;
    if (esp_core_dump_image_get(&addr, &size) == ESP_OK) g_fixed.coredump_bytes = static_cast<uint32_t>(size);
    g_fixed.read = true;
}

}  // namespace

void init() { read_fixed(); }

Snapshot read() {
    read_fixed();
    Snapshot s;
    s.internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s.internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s.psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    s.flash_bytes = g_fixed.flash_bytes;
    s.psram_bytes = g_fixed.psram_bytes;
    for (int i = 0; i < 6; ++i) s.mac[i] = g_fixed.mac[i];
    s.reset_reason = g_fixed.reset_reason;
    s.coredump_bytes = g_fixed.coredump_bytes;
    return s;
}

uint32_t uptime_ms() { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }

int64_t uptime_us() { return esp_timer_get_time(); }

void note_peripheral_init() { ++g_inits; }

uint32_t peripheral_inits() { return g_inits; }

}  // namespace platform::sysinfo
