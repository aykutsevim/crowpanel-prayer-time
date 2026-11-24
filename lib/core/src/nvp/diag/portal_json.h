// JSON bodies of the setup page's API (contracts/setup-portal.md). Pure C++, no heap.
// Neither function can carry a password: there is no parameter for one (FR-024, NET-08).
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvp::diag {

struct ScanEntry {
    char ssid[33] = {};
    int8_t rssi = 0;
    bool secured = false;
};

inline constexpr size_t kMaxScanEntries = 20;

// {"networks":[{"ssid":"…","rssi":-54,"secured":true}, …]}: strongest first, duplicate names
// merged (strongest kept), hidden networks (empty name) left out, at most 20 entries.
// Sorts `entries` in place. Returns the length needed, like snprintf.
size_t build_scan_json(ScanEntry* entries, size_t count, char* out, size_t cap);

enum class PortalState : uint8_t { Setup, Trying, Connected, Failed };

// {"state":"setup|trying|connected|failed","ssid":"…"}
size_t build_status_json(PortalState state, const char* ssid, char* out, size_t cap);

}  // namespace nvp::diag
