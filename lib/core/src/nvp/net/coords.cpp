#include "nvp/net/coords.h"

namespace nvp::net {

uint32_t fnv1a32(const char* text) {
    uint32_t h = 2166136261u;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); p != nullptr && *p != 0; ++p) {
        h ^= *p;
        h *= 16777619u;
    }
    return h;
}

bool in_turkey(int32_t lat_e5, int32_t lon_e5) {
    return lat_e5 >= 3580000 && lat_e5 <= 4220000 && lon_e5 >= 2560000 && lon_e5 <= 4490000;
}

Coords lookup_coords(const CoordRow* rows, size_t n, uint32_t district_id, const char* upper_name) {
    Coords c;
    if (rows == nullptr || n == 0) return c;
    const uint32_t h = fnv1a32(upper_name);
    // First row with this id.
    size_t lo = 0, hi = n;
    while (lo < hi) {
        const size_t mid = (lo + hi) / 2;
        if (rows[mid].district_id < district_id) lo = mid + 1; else hi = mid;
    }
    size_t same_id = 0;
    const CoordRow* only = nullptr;
    for (size_t i = lo; i < n && rows[i].district_id == district_id; ++i) {
        ++same_id;
        only = &rows[i];
        if (rows[i].name_hash == h) {
            c.valid = in_turkey(rows[i].lat_e5, rows[i].lon_e5);
            c.lat_e5 = rows[i].lat_e5;
            c.lon_e5 = rows[i].lon_e5;
            return c;
        }
    }
    if (same_id == 1) {
        c.valid = in_turkey(only->lat_e5, only->lon_e5);
        c.lat_e5 = only->lat_e5;
        c.lon_e5 = only->lon_e5;
    }
    return c;
}

}  // namespace nvp::net
