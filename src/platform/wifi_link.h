// Wi-Fi radio, stored details and the device's own network (NET-01, NET-02, NET-03; spec 004
// FR-010, FR-014, FR-016..FR-019, research R2, R3, R4; spec 001 contracts/setup-portal.md).
// Thin wrappers only: the decisions are in lib/core (nvp::net), driven from src/app. Call only
// from the network worker (core 0); the event handler runs on the default event loop task.
//
// Passwords: the stored Wi-Fi password and the access-point password are never logged or printed
// here (FR-023, SEC-04). The access-point password is handed to the app only for the device screen.
#pragma once

#include <cstddef>
#include <cstdint>

#include "nvp/net/scan.h"

namespace platform::wifi {

// --- Stored details (NVS namespace "wifi", keys "ssid" and "pass"; Slice 1's keys) ---
bool load_stored(char ssid[33], char pass[64]);
// Writes the password, commits, then the name, commits: the details count as present only once
// the name exists, so a power cut in between leaves "no Wi-Fi" (research R10).
bool store(const char* ssid, const char* pass);
void erase_stored();

// Identity of the device's network (NET-02): "NamazVakti-" + last 4 hex digits of the station MAC,
// and the 8-digit password from NVS "ap"/"pass", generated on first use and kept.
void hotspot_ssid(char out[33]);
void hotspot_password(char out[9]);

// --- Radio ---
void radio_on();  // station mode; nothing is persisted by the Wi-Fi driver itself
// Starts a connection. All channels are probed, so a hidden network is found (research R4).
void join(const char* ssid, const char* pass);
void disconnect();

enum class Link : uint8_t { Down, Connecting, Up, Failed };
Link link();
int8_t rssi();
// The station's address ("192.168.1.37"), or "" when not connected.
void station_ip(char out[16]);

// Disconnect reasons (raw wifi_err_reason_t numbers) since the last call, oldest first; our own
// disconnects are left out. At most `max`; the rest are dropped.
size_t take_reasons(uint16_t* out, size_t max);

// --- The device's own network: AP+STA, WPA2, 2 clients, 192.168.4.1, DHCP with DNS and option 114 ---
bool start_hotspot(const char* ssid, const char* pass);
void stop_hotspot();
bool hotspot_up();
const char* hotspot_address();  // "192.168.4.1"
uint8_t hotspot_clients();      // phones joined now (AP_STACONNECTED / AP_STADISCONNECTED)

// --- Asynchronous scans (zero networks counts as success) ---
// Full: every channel in one pass, before the device's network starts. Grouped: one channel per
// pass with a 120 ms pause on the home channel after every fourth, so a joined phone keeps its
// connection (research R3).
bool scan_start(bool grouped);
// Drives a grouped scan. -1 while running, -2 on failure, otherwise the number of networks found.
int scan_poll();
// Copies up to `max` results (auth classified from the raw mode) and frees the scan memory.
size_t scan_take(nvp::net::ScanEntry* out, size_t max);
bool scan_running();
void scan_stop();  // before a storage write or an attempt (FR-018)

}  // namespace platform::wifi
