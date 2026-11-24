// JSON escaping, reconnect backoff and hotspot password (tasks T060-T062; NET-01, NET-02,
// contracts/setup-portal.md). Written before the code.
#include <unity.h>

#include <cstdio>
#include <cstring>

#include "nvp/diag/ap_password.h"
#include "nvp/diag/backoff.h"
#include "nvp/diag/json.h"

using namespace nvp::diag;

// --- JSON (T060) ---

static void test_json_plain_text_unchanged() {
    char out[64];
    TEST_ASSERT_EQUAL_UINT32(5, json_escape("EvAgi", out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("EvAgi", out);
}

static void test_json_escapes_quote_and_backslash() {
    char out[64];
    json_escape("a\"b\\c", out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("a\\\"b\\\\c", out);
}

static void test_json_escapes_control_characters() {
    char out[64];
    json_escape("a\nb\tc\x01", out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("a\\nb\\tc\\u0001", out);
}

static void test_json_passes_utf8_through() {
    char out[64];
    json_escape("Ev \xC4\x9Finternet", out, sizeof out);  // "Ev ğinternet"
    TEST_ASSERT_EQUAL_STRING("Ev \xC4\x9Finternet", out);
}

static void test_json_32_byte_ssid_and_empty() {
    char out[256];
    const char* ssid = "0123456789abcdef0123456789abcdef";
    TEST_ASSERT_EQUAL_UINT32(32, json_escape(ssid, out, sizeof out));
    TEST_ASSERT_EQUAL_STRING(ssid, out);
    TEST_ASSERT_EQUAL_UINT32(0, json_escape("", out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("", out);
}

static void test_json_short_buffer_is_safe() {
    char out[4];
    std::memset(out, 'X', sizeof out);
    const size_t need = json_escape("abcdef", out, sizeof out);
    TEST_ASSERT_EQUAL_UINT32(6, need);
    TEST_ASSERT_EQUAL_STRING("abc", out);
}

static void test_json_never_splits_an_escape() {
    char out[3];  // room for two characters: "a\" must not become a dangling backslash
    json_escape("a\"", out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("a", out);
}

// --- Backoff (T061, NET-01: 5 s to 5 min) ---

static void test_backoff_doubles_and_caps() {
    Backoff b;
    const uint32_t expected[] = {5000, 10000, 20000, 40000, 80000, 160000, 300000, 300000, 300000};
    for (uint32_t e : expected) {
        TEST_ASSERT_EQUAL_UINT32(e, b.next_delay_ms());
    }
}

static void test_backoff_resets_on_success() {
    Backoff b;
    b.next_delay_ms();
    b.next_delay_ms();
    b.reset();
    TEST_ASSERT_EQUAL_UINT32(5000, b.next_delay_ms());
}

// --- AP password (T062, NET-02: 8 decimal digits) ---

namespace {
uint32_t g_words[4];
int g_next = 0;
uint32_t fake_random() { return g_words[g_next++ % 4]; }
}  // namespace

static void test_ap_password_is_eight_digits_with_leading_zeros() {
    char out[9];
    g_words[0] = 42;
    g_next = 0;
    make_ap_password(fake_random, out);
    TEST_ASSERT_EQUAL_STRING("00000042", out);
}

static void test_ap_password_always_eight_digits() {
    char out[9];
    const uint32_t samples[] = {0u, 1u, 99999999u, 100000000u, 123456789u, 0xFFFFFFFFu};
    for (uint32_t s : samples) {
        g_words[0] = s;
        g_next = 0;
        make_ap_password(fake_random, out);
        TEST_ASSERT_EQUAL_UINT32(8, std::strlen(out));
        for (const char* p = out; *p; ++p) TEST_ASSERT_TRUE(*p >= '0' && *p <= '9');
    }
}

// --- Setup page JSON (contracts/setup-portal.md, FR-021, FR-024) ---

// The portal's JSON bodies moved to nvp/net/portal_json (spec 004): test/test_net/test_scan.cpp.

void run_net_helper_tests() {
    RUN_TEST(test_json_plain_text_unchanged);
    RUN_TEST(test_json_escapes_quote_and_backslash);
    RUN_TEST(test_json_escapes_control_characters);
    RUN_TEST(test_json_passes_utf8_through);
    RUN_TEST(test_json_32_byte_ssid_and_empty);
    RUN_TEST(test_json_short_buffer_is_safe);
    RUN_TEST(test_json_never_splits_an_escape);
    RUN_TEST(test_backoff_doubles_and_caps);
    RUN_TEST(test_backoff_resets_on_success);
    RUN_TEST(test_ap_password_is_eight_digits_with_leading_zeros);
    RUN_TEST(test_ap_password_always_eight_digits);
}
