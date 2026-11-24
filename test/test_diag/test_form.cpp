// Form-encoded body decoding for POST /api/setup (contracts/setup-portal.md; FR-021, FR-024).
// Written before the code (constitution VI). The page sends URLSearchParams:
// application/x-www-form-urlencoded, '+' for space, %XX for other bytes.
#include <unity.h>

#include "nvp/diag/form.h"

using nvp::diag::form_value;

static void test_plain_fields() {
    char out[64];
    TEST_ASSERT_TRUE(form_value("ssid=Home&password=secret1", "ssid", out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("Home", out);
    TEST_ASSERT_TRUE(form_value("ssid=Home&password=secret1", "password", out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("secret1", out);
}

static void test_plus_and_percent_are_decoded() {
    char out[64];
    TEST_ASSERT_TRUE(form_value("ssid=Ev+A%C4%9Fi%26Co&password=a%2Bb%3D", "ssid", out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("Ev A\xC4\x9Fi&Co", out);  // "Ev Ağı&Co" in UTF-8, partly
    TEST_ASSERT_TRUE(form_value("ssid=x&password=a%2Bb%3D", "password", out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("a+b=", out);
}

static void test_key_must_match_whole_name() {
    char out[16];
    TEST_ASSERT_TRUE(form_value("xssid=1&ssid=2", "ssid", out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("2", out);
    TEST_ASSERT_FALSE(form_value("ssidx=1", "ssid", out, sizeof out));
}

static void test_missing_and_empty() {
    char out[16] = "junk";
    TEST_ASSERT_FALSE(form_value("ssid=a", "password", out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("", out);
    TEST_ASSERT_TRUE(form_value("ssid=a&password=", "password", out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("", out);
    TEST_ASSERT_FALSE(form_value(nullptr, "ssid", out, sizeof out));
}

static void test_too_long_value_is_refused() {
    char out[4];
    TEST_ASSERT_FALSE(form_value("ssid=abcd", "ssid", out, sizeof out));  // needs 5 bytes
    TEST_ASSERT_TRUE(form_value("ssid=abc", "ssid", out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("abc", out);
}

static void test_bad_escapes_are_refused() {
    char out[16];
    TEST_ASSERT_FALSE(form_value("ssid=a%2", "ssid", out, sizeof out));
    TEST_ASSERT_FALSE(form_value("ssid=a%zz", "ssid", out, sizeof out));
    TEST_ASSERT_FALSE(form_value("ssid=a%00b", "ssid", out, sizeof out));  // no embedded NUL
}

void run_form_tests() {
    RUN_TEST(test_plain_fields);
    RUN_TEST(test_plus_and_percent_are_decoded);
    RUN_TEST(test_key_must_match_whole_name);
    RUN_TEST(test_missing_and_empty);
    RUN_TEST(test_too_long_value_is_refused);
    RUN_TEST(test_bad_escapes_are_refused);
}
