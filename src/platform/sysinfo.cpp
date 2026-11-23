#include "sysinfo.h"

#include <Arduino.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_mac.h>
#include <esp_psram.h>
#include <esp_system.h>

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

}  // namespace

Snapshot read() {
    Snapshot s;
    s.internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s.internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s.psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    uint32_t flash = 0;
    if (esp_flash_get_physical_size(nullptr, &flash) == ESP_OK) {
        s.flash_bytes = flash;
    }
    s.psram_bytes = static_cast<uint32_t>(esp_psram_get_size());
    esp_read_mac(s.mac, ESP_MAC_WIFI_STA);
    s.reset_reason = reset_reason_name(esp_reset_reason());
    return s;
}

uint32_t uptime_ms() { return millis(); }

void note_peripheral_init() { ++g_inits; }

uint32_t peripheral_inits() { return g_inits; }

}  // namespace platform::sysinfo
