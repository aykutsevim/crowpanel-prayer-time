// The location and the bundled place list (LOC-01, LOC-02; spec 004 FR-012, FR-017, OD4-3,
// research R6, R10). Pure C++, no heap.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvp::net {

struct Location {
    uint32_t district_id = 0;  // Diyanet's id; for a district without its own times, its centre's (OD4-3)
    char province[41] = {};    // upper case, as in the list ("İSTANBUL")
    char district[41] = {};    // upper case ("SULTANBEYLİ")
};

// True when `json` (data/places_tr.json, uncompressed) holds the district `district` with id
// `district_id` under the province named `province`. Streams the text; malformed or truncated
// input gives false.
bool place_lookup(const char* json, size_t len, uint32_t district_id, const char* province, const char* district);

}  // namespace nvp::net
