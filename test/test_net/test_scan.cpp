// Scan entries for the setup page (spec 004 FR-014, NET-06, 02 §7.4; research R3). Written before the code.
#include <unity.h>

#include <cstdio>
#include <cstring>

#include "nvp/net/portal_json.h"
#include "nvp/net/scan.h"

using namespace nvp::net;

static ScanEntry e(const char* ssid, int8_t rssi, Auth a = Auth::Secured) {
    ScanEntry x;
    std::strncpy(x.ssid, ssid, sizeof x.ssid - 1);
    x.rssi = rssi;
    x.auth = a;
    return x;
}

static void test_auth_class() {  // wifi_auth_mode_t 0..17, esp_wifi_types_generic.h (5.5.5)
    const Auth expected[18] = {Auth::Open,        Auth::Unsupported, Auth::Secured,     Auth::Secured,
                               Auth::Secured,     Auth::Unsupported, Auth::Secured,     Auth::Secured,
                               Auth::Unsupported, Auth::Unsupported, Auth::Unsupported, Auth::Secured,
                               Auth::Secured,     Auth::Unsupported, Auth::Unsupported, Auth::Unsupported,
                               Auth::Unsupported, Auth::Unsupported};
    for (int i = 0; i < 18; ++i) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(static_cast<int>(expected[i]), static_cast<int>(auth_class(i)), "auth mode");
    }
    TEST_ASSERT_TRUE(auth_class(-1) == Auth::Unsupported);
    TEST_ASSERT_TRUE(auth_class(99) == Auth::Unsupported);
}

static void test_merge() {
    ScanEntry list[30];
    size_t n = 0;
    list[n++] = e("Ev", -70);
    list[n++] = e("", -30);  // hidden
    list[n++] = e("Komsu", -50);
    list[n++] = e("Ev", -55);  // stronger duplicate
    for (int i = 0; i < 24; ++i) {
        char name[8];
        snprintf(name, sizeof name, "N%02d", i);
        list[n++] = e(name, static_cast<int8_t>(-80 - i));
    }
    const size_t m = merge_scan(list, n);
    TEST_ASSERT_EQUAL_UINT32(20, m);
    TEST_ASSERT_EQUAL_STRING("Komsu", list[0].ssid);
    TEST_ASSERT_EQUAL_STRING("Ev", list[1].ssid);
    TEST_ASSERT_EQUAL_INT8(-55, list[1].rssi);
    for (size_t i = 1; i < m; ++i) TEST_ASSERT_TRUE(list[i - 1].rssi >= list[i].rssi);
    for (size_t i = 0; i < m; ++i) TEST_ASSERT_TRUE(list[i].ssid[0] != '\0');
}

static void test_signal_class() {
    TEST_ASSERT_EQUAL_STRING("iyi", signal_class(-60));
    TEST_ASSERT_EQUAL_STRING("orta", signal_class(-61));
    TEST_ASSERT_EQUAL_STRING("orta", signal_class(-75));
    TEST_ASSERT_EQUAL_STRING("zayıf", signal_class(-76));
}

static void test_scan_json() {
    ScanEntry list[2] = {e("Ev \"A\"", -54), e("Kafe", -80, Auth::Open)};
    char out[256];
    const size_t n = scan_json(list, 2, false, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING(
        "{\"scanning\":false,\"networks\":[{\"ssid\":\"Ev \\\"A\\\"\",\"rssi\":-54,\"signal\":\"iyi\",\"auth\":\"secured\"},"
        "{\"ssid\":\"Kafe\",\"rssi\":-80,\"signal\":\"zayıf\",\"auth\":\"open\"}]}",
        out);
    TEST_ASSERT_EQUAL_UINT32(std::strlen(out), n);
    char small[8];
    TEST_ASSERT_TRUE(scan_json(list, 2, true, small, sizeof small) >= sizeof small);  // needed length, like snprintf
}

static void test_status_json() {
    char out[256];
    Location loc;
    loc.district_id = 9549;
    std::strcpy(loc.province, "İSTANBUL");
    std::strcpy(loc.district, "SULTANBEYLİ");
    status_json("setup", "wrong_password", "Ev", &loc, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING(
        "{\"mode\":\"setup\",\"result\":\"wrong_password\",\"ssid\":\"Ev\",\"location\":{\"id\":9549,"
        "\"province\":\"İSTANBUL\",\"district\":\"SULTANBEYLİ\"}}",
        out);
    status_json("connecting", "none", "", nullptr, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("{\"mode\":\"connecting\",\"result\":\"none\",\"ssid\":\"\",\"location\":null}", out);
}

void run_scan_tests() {
    RUN_TEST(test_auth_class);
    RUN_TEST(test_merge);
    RUN_TEST(test_signal_class);
    RUN_TEST(test_scan_json);
    RUN_TEST(test_status_json);
}
