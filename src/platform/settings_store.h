// Stored location and the settings PIN (LOC-01, settings table of 03 §8; spec 004 FR-017, FR-020,
// FR-022, research R10). NVS namespaces "loc" (district_id, province, district) and "sec" (pin),
// beside Slice 1's "wifi" and "ap". Call only from the network worker. Nothing here logs a PIN.
#pragma once

#include "nvp/net/places.h"

namespace platform::settings {

bool load_location(nvp::net::Location& out);         // false when none is stored
bool store_location(const nvp::net::Location& loc);  // committed before the Wi-Fi details (R10)
// Generates and stores the 6-digit PIN on the first start of this firmware; later calls keep it.
// The PIN itself is not returned: it is shown only from Slice 8.
bool ensure_pin();

}  // namespace platform::settings
