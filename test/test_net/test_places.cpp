// Checking a submitted location against the bundled place list (spec 004 FR-012, FR-017, OD4-3,
// research R6, R10). Written before the code.
#include <unity.h>

#include <cstring>

#include "nvp/net/places.h"

using namespace nvp::net;

static const char kList[] =
    "{\"generated\":\"2026-11-24\",\"source\":\"test\",\"sha256\":\"00\",\"provinces\":["
    "[506,\"ANKARA\",[[9206,\"ANKARA\"],[9206,\"ÇANKAYA\"],[9205,\"AYAŞ\"]]],"
    "[539,\"İSTANBUL\",[[9541,\"İSTANBUL\"],[9541,\"KADIKÖY\"],[9541,\"KÂĞITHANE\"],[9549,\"SULTANBEYLİ\"]]]"
    "]}";

static bool look(uint32_t id, const char* p, const char* d) {
    return place_lookup(kList, std::strlen(kList), id, p, d);
}

static void test_accepts() {
    TEST_ASSERT_TRUE(look(9549, "İSTANBUL", "SULTANBEYLİ"));
    TEST_ASSERT_TRUE(look(9541, "İSTANBUL", "KADIKÖY"));  // alias under the centre's id
    TEST_ASSERT_TRUE(look(9541, "İSTANBUL", "KÂĞITHANE"));
    TEST_ASSERT_TRUE(look(9206, "ANKARA", "ÇANKAYA"));
}

static void test_refuses() {
    TEST_ASSERT_FALSE(look(9548, "İSTANBUL", "SULTANBEYLİ"));  // wrong id
    TEST_ASSERT_FALSE(look(9549, "ANKARA", "SULTANBEYLİ"));    // wrong province
    TEST_ASSERT_FALSE(look(9206, "İSTANBUL", "ÇANKAYA"));      // name from another province
    TEST_ASSERT_FALSE(look(9549, "İSTANBUL", ""));
    TEST_ASSERT_FALSE(look(9549, "İSTANBUL", "SULTANBEYL"));   // prefix only
    TEST_ASSERT_FALSE(look(0, "İSTANBUL", "İSTANBUL"));
    TEST_ASSERT_FALSE(place_lookup("{\"provinces\":[[1,\"A\",[[2,", 25, 2, "A", "B"));  // truncated
    TEST_ASSERT_FALSE(place_lookup(nullptr, 0, 9549, "İSTANBUL", "SULTANBEYLİ"));
}

void run_places_tests() {
    RUN_TEST(test_accepts);
    RUN_TEST(test_refuses);
}
