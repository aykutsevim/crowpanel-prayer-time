// Generated device secrets: the access-point password and the settings PIN (NET-02, settings table
// of 03 §8; spec 004 FR-010, FR-022, research R10). Written before the code.
#include <unity.h>

#include <cstring>
#include <initializer_list>

#include "nvp/diag/ap_password.h"

namespace {
uint32_t g_seq[8];
int g_at = 0;
uint32_t next() { return g_seq[g_at++]; }
void seq(std::initializer_list<uint32_t> v) {
    int i = 0;
    for (uint32_t x : v) g_seq[i++] = x;
    g_at = 0;
}
}  // namespace

static void test_pin_six_digits_with_leading_zeros() {
    char pin[7];
    seq({42});
    nvp::diag::make_settings_pin(next, pin);
    TEST_ASSERT_EQUAL_STRING("000042", pin);
}

static void test_pin_rejects_the_biased_top() {
    // 4294000000 is the first value of the incomplete last block of 10^6: it is drawn again.
    char pin[7];
    seq({4294000000u, 4294967295u, 123456});
    nvp::diag::make_settings_pin(next, pin);
    TEST_ASSERT_EQUAL_STRING("123456", pin);
    TEST_ASSERT_EQUAL_INT(3, g_at);
}

static void test_ap_password_rejects_the_biased_top() {
    char pw[9];
    seq({4200000000u, 7});
    nvp::diag::make_ap_password(next, pw);
    TEST_ASSERT_EQUAL_STRING("00000007", pw);
}

void run_secrets_tests() {
    RUN_TEST(test_pin_six_digits_with_leading_zeros);
    RUN_TEST(test_pin_rejects_the_biased_top);
    RUN_TEST(test_ap_password_rejects_the_biased_top);
}
