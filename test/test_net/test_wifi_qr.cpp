// Wi-Fi join payload of the setup screen's QR code (spec 004 FR-004, 02 §7.2, research R11).
// Written before the code.
#include <unity.h>

#include <cstring>

#include "nvp/net/wifi_qr.h"

using namespace nvp::net;

static void test_device_payload() {
    char out[96];
    const size_t n = wifi_qr_payload("NamazVakti-7F3A", "48271935", out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("WIFI:T:WPA;S:NamazVakti-7F3A;P:48271935;;", out);
    TEST_ASSERT_EQUAL_UINT32(std::strlen(out), n);
}

static void test_escaping() {
    char out[96];
    wifi_qr_payload("a;b,c:d\\e\"f", "p;w", out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("WIFI:T:WPA;S:a\\;b\\,c\\:d\\\\e\\\"f;P:p\\;w;;", out);
}

static void test_too_small() {
    char out[10] = "x";
    TEST_ASSERT_EQUAL_UINT32(0, wifi_qr_payload("NamazVakti-7F3A", "48271935", out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("", out);
}

void run_wifi_qr_tests() {
    RUN_TEST(test_device_payload);
    RUN_TEST(test_escaping);
    RUN_TEST(test_too_small);
}
