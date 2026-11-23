// Firmware version: the single version constant used by the boot screen, status panel,
// User-Agent and release tag (Slice 9). Pure C++, no platform headers (05-architecture.md §4).
#pragma once

#define NVP_VERSION_STRING "0.1.0"

namespace nvp {

inline constexpr const char* kFirmwareVersion = NVP_VERSION_STRING;

// SEC-03, handover/docs/04-data-sources.md §10.
inline constexpr const char* kUserAgent =
    "NamazVaktiPanel/" NVP_VERSION_STRING " (ESP32-S3; personal device)";

}  // namespace nvp
