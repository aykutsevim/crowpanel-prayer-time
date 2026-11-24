// Wi-Fi radio, stored details and hotspot (research R11, contracts/setup-portal.md; NET-01,
// NET-02, FR-020..FR-025). Thin wrappers only: the state machine is nvp::diag::wifi_step, driven
// from src/app. Call only from the network worker (core 0).
//
// Passwords: the stored Wi-Fi password and the hotspot password are never logged or printed here
// (FR-024, SEC-04). The hotspot password is handed to the app only for the device screen.
#pragma once

#include <cstddef>
#include <cstdint>

#include "nvp/diag/portal_json.h"

namespace platform::wifi {

// --- Stored details (NVS namespace "wifi", keys "ssid" and "pass"; Slice 4 reads the same) ---
bool load_stored(char ssid[33], char pass[64]);
bool store(const char* ssid, const char* pass);
void erase_stored();

// Hotspot identity (NET-02): "NamazVakti-" + last 4 hex digits of the station MAC, and the
// 8-digit password from NVS "ap"/"pass", generated with esp_random() on first use.
void hotspot_ssid(char out[33]);
void hotspot_password(char out[9]);

// --- Radio ---
void radio_on();  // station mode; nothing is persisted by the Wi-Fi driver itself
void join(const char* ssid, const char* pass);
void disconnect();

enum class Link : uint8_t { Down, Connecting, Up, Failed };
Link link();
int8_t rssi();

bool start_hotspot(const char* ssid, const char* pass);  // AP+STA, WPA2, 2 clients, 192.168.4.1
void stop_hotspot();
bool hotspot_up();
const char* hotspot_address();  // "192.168.4.1"

// --- Asynchronous scan (zero networks counts as success) ---
bool scan_start();
// -1 while running, -2 on failure, otherwise the number of networks found.
int scan_poll();
// Copies up to `max` results and frees the scan memory.
size_t scan_take(nvp::diag::ScanEntry* out, size_t max);

}  // namespace platform::wifi
