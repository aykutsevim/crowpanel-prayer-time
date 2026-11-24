// JSON bodies of the portal's API (NET-04, NET-08; spec 004 contracts/portal-http.md). Replaces
// nvp/diag/portal_json of Slice 1. Pure C++, no heap. No function has a parameter that could carry
// a password.
#pragma once

#include <cstddef>

#include "nvp/net/places.h"
#include "nvp/net/scan.h"

namespace nvp::net {

// {"scanning":false,"networks":[{"ssid":"…","rssi":-54,"signal":"iyi","auth":"secured"},…]}
// `entries` must already be merged (merge_scan). Returns the length needed, like snprintf.
size_t scan_json(const ScanEntry* entries, size_t count, bool scanning, char* out, size_t cap);

// {"mode":"setup","result":"none","ssid":"…","location":{"id":9549,"province":"…","district":"…"}}
// or "location":null when `loc` is null. Returns the length needed, like snprintf.
size_t status_json(const char* mode, const char* result, const char* ssid, const Location* loc, char* out, size_t cap);

}  // namespace nvp::net
