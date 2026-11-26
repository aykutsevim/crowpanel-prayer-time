// Coordinates lookup in the bundled table (spec 007 T032; FR-015, LOC-03, research R4).
#include <unity.h>

#include "nvp/net/coords.h"

using nvp::net::Coords;
using nvp::net::CoordRow;

namespace {

// Rows as tools/gen_web.py generates them from data/places_coords.json (sorted by id, then hash).
const CoordRow kRows[] = {
    {9206u, 0x8cd8ee03u, 3992063, 3285403},  // ANKARA
    {9541u, 0x5282d4b9u, 4098229, 2909032},  // KADIKÖY (shares İstanbul's id, OD4-3)
    {9541u, 0xf0000001u, 4100000, 2897000},  // another entry under 9541 (test data)
    {9549u, 0x12345678u, 4096800, 2926000},  // a single entry under its id (test data)
    {9999u, 0x00000001u, 800000, 700000},    // outside Türkiye (test data)
};
constexpr size_t kN = sizeof kRows / sizeof kRows[0];

// The hash must equal the generator's (Python, same UTF-8 bytes).
void test_hash_matches_generator() {
    TEST_ASSERT_EQUAL_HEX32(0x8cd8ee03u, nvp::net::fnv1a32("ANKARA"));
    TEST_ASSERT_EQUAL_HEX32(0x5282d4b9u, nvp::net::fnv1a32("KADIKÖY"));
}

void test_lookup_by_id_and_name() {
    Coords c = nvp::net::lookup_coords(kRows, kN, 9206, "ANKARA");
    TEST_ASSERT_TRUE(c.valid);
    TEST_ASSERT_EQUAL_INT32(3992063, c.lat_e5);
    TEST_ASSERT_EQUAL_INT32(3285403, c.lon_e5);
    c = nvp::net::lookup_coords(kRows, kN, 9541, "KADIKÖY");
    TEST_ASSERT_TRUE(c.valid);
    TEST_ASSERT_EQUAL_INT32(4098229, c.lat_e5);
}

void test_fallback_only_when_unambiguous() {
    // A single row under the id: used even if the name differs.
    TEST_ASSERT_TRUE(nvp::net::lookup_coords(kRows, kN, 9549, "SULTANBEYLİ").valid);
    // Two rows under 9541 and no matching name: none (the device then asks the district detail).
    TEST_ASSERT_FALSE(nvp::net::lookup_coords(kRows, kN, 9541, "ÜSKÜDAR").valid);
    TEST_ASSERT_FALSE(nvp::net::lookup_coords(kRows, kN, 1234, "YOK").valid);
}

void test_outside_turkey_invalid() {
    TEST_ASSERT_FALSE(nvp::net::lookup_coords(kRows, kN, 9999, "X").valid);
    TEST_ASSERT_TRUE(nvp::net::in_turkey(3992063, 3285403));
    TEST_ASSERT_FALSE(nvp::net::in_turkey(888888, 777777));  // Aladhan's placeholder (research R2)
}

}  // namespace

void run_coords_tests() {
    RUN_TEST(test_hash_matches_generator);
    RUN_TEST(test_lookup_by_id_and_name);
    RUN_TEST(test_fallback_only_when_unambiguous);
    RUN_TEST(test_outside_turkey_invalid);
}
