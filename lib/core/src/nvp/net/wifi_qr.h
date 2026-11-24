// The Wi-Fi join payload of the setup screen's QR code (02 §7.2; spec 004 FR-004, research R11):
// WIFI:T:WPA;S:<ssid>;P:<password>;; with \ ; , : " escaped by a backslash (the ZXing format).
#pragma once

#include <cstddef>

namespace nvp::net {

// Returns the payload length, or 0 with `out` empty when it does not fit in `cap`.
size_t wifi_qr_payload(const char* ssid, const char* password, char* out, size_t cap);

}  // namespace nvp::net
