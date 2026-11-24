// Scan entries for the setup page (NET-04, NET-06; spec 004 FR-014, research R3). Pure C++, no heap.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvp::net {

enum class Auth : uint8_t { Open, Secured, Unsupported };

// Raw wifi_auth_mode_t value of ESP-IDF 5.5.5 (esp_wifi_types_generic.h) -> class. Open: 0. Unsupported:
// WEP, enterprise of every kind, WAPI, OWE, DPP and unknown values. Everything else is a PSK/SAE
// network the station can join with a password.
Auth auth_class(int raw);

struct ScanEntry {
    char ssid[33] = {};
    int8_t rssi = 0;
    Auth auth = Auth::Secured;
};

inline constexpr size_t kMaxScanEntries = 20;

// In place: drops hidden networks (empty name), merges duplicate names keeping the strongest,
// sorts by signal (strongest first) and keeps at most 20. Returns the new count.
size_t merge_scan(ScanEntry* entries, size_t count);

// 02 §7.4: "iyi" (>= -60 dBm), "orta" (>= -75), "zayıf" (below).
const char* signal_class(int rssi);

const char* auth_name(Auth a);  // "open", "secured", "unsupported"

}  // namespace nvp::net
