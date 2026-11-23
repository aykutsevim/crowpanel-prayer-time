// The auxiliary-controller command gate and the silence sequence (SIL-02, FR-001, FR-003,
// FR-018, contracts/aux-controller.md). Written before the code. Host-only: the native env
// reaches the pure part of the header through -Isrc/platform, with no Arduino include.
#include <unity.h>

#include "audio_off.h"

using namespace platform::silence;

void setUp() {}
void tearDown() {}

static void test_gate_allows_exactly_the_contract_bytes() {
    for (int b = 0; b <= 255; ++b) {
        const bool expected = b == 0 || b == 247 || b == 249 || b == 250;
        TEST_ASSERT_EQUAL_MESSAGE(expected, aux_byte_allowed(static_cast<uint8_t>(b)),
                                  "aux_byte_allowed disagrees with contracts/aux-controller.md");
    }
}

static void test_buzzer_on_and_amplifier_on_are_rejected() {
    TEST_ASSERT_FALSE(aux_byte_allowed(246));  // buzzer on
    TEST_ASSERT_FALSE(aux_byte_allowed(248));  // amplifier on
}

static void test_every_dimming_value_is_rejected() {
    // OD1 (2026-11-20): no dimming, ever. 1..244 dim, 245 off; 2 and 3 also overlap audio paths.
    for (int b = 1; b <= 245; ++b) {
        TEST_ASSERT_FALSE(aux_byte_allowed(static_cast<uint8_t>(b)));
    }
}

static void test_undocumented_bytes_are_rejected() {
    for (int b = 251; b <= 255; ++b) {
        TEST_ASSERT_FALSE(aux_byte_allowed(static_cast<uint8_t>(b)));
    }
}

static void test_silence_sequence_is_mute_then_buzzer_off() {
    TEST_ASSERT_EQUAL_UINT32(2, sizeof kSilenceSequence);
    TEST_ASSERT_EQUAL_UINT8(249, kSilenceSequence[0]);
    TEST_ASSERT_EQUAL_UINT8(247, kSilenceSequence[1]);
    for (uint8_t b : kSilenceSequence) {
        TEST_ASSERT_TRUE(aux_byte_allowed(b));
    }
}

static void test_retry_policy() {
    TEST_ASSERT_EQUAL_UINT8(20, kSilenceMaxAttempts);
    TEST_ASSERT_EQUAL_UINT32(50, kSilenceRetryMs);
}

static void test_named_commands_pass_the_gate() {
    TEST_ASSERT_TRUE(aux_byte_allowed(kAuxBacklightFull));
    TEST_ASSERT_TRUE(aux_byte_allowed(kAuxTouchReset));
    TEST_ASSERT_EQUAL_UINT8(0, kAuxBacklightFull);
    TEST_ASSERT_EQUAL_UINT8(250, kAuxTouchReset);
}

static void test_gate_is_usable_at_compile_time() {
    static_assert(aux_byte_allowed(249) && !aux_byte_allowed(248), "gate must be constexpr");
    TEST_PASS();
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_gate_allows_exactly_the_contract_bytes);
    RUN_TEST(test_buzzer_on_and_amplifier_on_are_rejected);
    RUN_TEST(test_every_dimming_value_is_rejected);
    RUN_TEST(test_undocumented_bytes_are_rejected);
    RUN_TEST(test_silence_sequence_is_mute_then_buzzer_off);
    RUN_TEST(test_retry_policy);
    RUN_TEST(test_named_commands_pass_the_gate);
    RUN_TEST(test_gate_is_usable_at_compile_time);
    return UNITY_END();
}
