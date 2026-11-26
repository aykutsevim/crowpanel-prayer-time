// District coordinates from the table bundled with the firmware (spec 007 T032, FR-015, LOC-03,
// research R4). The table is generated at build time by tools/gen_web.py from
// data/places_coords.json (src/generated/places_coords.cpp); this lookup is pure and host-tested.
//
// A bundled entry is identified by the id the location stores (Diyanet's, or the centre's for an
// entry without its own times, OD4-3) together with its upper-case district name, because several
// entries share a centre's id (İstanbul 9541: İSTANBUL, KADIKÖY, ...).
#pragma once
#include <cstddef>
#include <cstdint>

namespace nvp::net {

struct CoordRow {
    uint32_t district_id;
    uint32_t name_hash;  // fnv1a32 of the upper-case UTF-8 name
    int32_t lat_e5;      // degrees x 100 000
    int32_t lon_e5;
};

struct Coords {
    bool valid = false;
    int32_t lat_e5 = 0;
    int32_t lon_e5 = 0;
};

// FNV-1a, 32 bit, over the bytes up to the terminating NUL.
uint32_t fnv1a32(const char* text);

// Türkiye's box of 04 §6: latitude 35.8..42.2, longitude 25.6..44.9.
bool in_turkey(int32_t lat_e5, int32_t lon_e5);

// `rows` sorted by (district_id, name_hash). The row with both the id and the name; else the only
// row with that id; else none.
Coords lookup_coords(const CoordRow* rows, size_t n, uint32_t district_id, const char* upper_name);

}  // namespace nvp::net
