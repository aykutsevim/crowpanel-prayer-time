// İmsakiyem district detail, the coordinates fallback (spec 007 T033; FR-015, research R4).
#include <unity.h>

#include <string>

#include "../support/fixture_file.h"
#include "nvp/providers/imsakiyem.h"

using nvp::providers::DistrictDetail;
using nvp::providers::DistrictDetailParser;

namespace {

DistrictDetail parse(const std::string& body) {
    DistrictDetailParser p;
    p.feed(body.data(), body.size());
    p.finish();
    return p.detail();
}

void test_ankara() {
    const DistrictDetail d = parse(nvp_test::read_fixture("imsakiyem/v2_district_9206.json"));
    TEST_ASSERT_TRUE(d.success);
    TEST_ASSERT_TRUE(d.has_coords);
    TEST_ASSERT_EQUAL_INT32(3992063, d.lat_e5);
    TEST_ASSERT_EQUAL_INT32(3285403, d.lon_e5);
    TEST_ASSERT_EQUAL_STRING("high", d.confidence);
    TEST_ASSERT_TRUE(d.accepted());
}

void test_kadikoy_own_coordinates() {
    const DistrictDetail d = parse(nvp_test::read_fixture("imsakiyem/v2_district_100059.json"));
    TEST_ASSERT_TRUE(d.accepted());
    TEST_ASSERT_EQUAL_INT32(4098229, d.lat_e5);
    TEST_ASSERT_EQUAL_INT32(2909032, d.lon_e5);
}

void test_not_found() {
    const DistrictDetail d = parse(nvp_test::read_fixture("imsakiyem/v2_district_not_found.json"));
    TEST_ASSERT_FALSE(d.success);
    TEST_ASSERT_TRUE(d.not_found);
    TEST_ASSERT_FALSE(d.accepted());
}

void test_low_confidence_refused() {
    std::string body = nvp_test::read_fixture("imsakiyem/v2_district_9206.json");
    TEST_ASSERT_TRUE(nvp_test::replace_first(body, "\"confidence\":\"high\"", "\"confidence\":\"low\""));
    TEST_ASSERT_FALSE(parse(body).accepted());
    std::string medium = nvp_test::read_fixture("imsakiyem/v2_district_9206.json");
    nvp_test::replace_first(medium, "\"confidence\":\"high\"", "\"confidence\":\"medium\"");
    TEST_ASSERT_TRUE(parse(medium).accepted());
}

void test_outside_turkey_refused() {
    std::string body = nvp_test::read_fixture("imsakiyem/v2_district_9206.json");
    TEST_ASSERT_TRUE(nvp_test::replace_first(body, "\"lat\":39.92063", "\"lat\":8.8888888"));
    TEST_ASSERT_FALSE(parse(body).accepted());
}

void test_url() {
    char url[96];
    TEST_ASSERT_TRUE(nvp::providers::district_detail_url(9206, url, sizeof url));
    TEST_ASSERT_EQUAL_STRING("https://ezanvakti.imsakiyem.com/api/v2/locations/districts/9206", url);
}

}  // namespace

void run_district_detail_tests() {
    RUN_TEST(test_ankara);
    RUN_TEST(test_kadikoy_own_coordinates);
    RUN_TEST(test_not_found);
    RUN_TEST(test_low_confidence_refused);
    RUN_TEST(test_outside_turkey_refused);
    RUN_TEST(test_url);
}
