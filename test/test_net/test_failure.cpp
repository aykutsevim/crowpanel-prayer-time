// Classifying a failed attempt (spec 004 FR-016, NET-05, research R4). Written before the code.
#include <unity.h>

#include <cstdint>
#include <initializer_list>

#include "nvp/net/failure.h"

using namespace nvp::net;

template <size_t N>
static FailureClass c(const uint16_t (&r)[N], bool seen) {
    return classify_failure(r, N, seen);
}

static void test_not_found() {
    const uint16_t a[] = {201};
    TEST_ASSERT_TRUE(c(a, false) == FailureClass::NotFound);
    for (uint16_t r : {210, 211, 212}) {
        const uint16_t b[] = {r};
        TEST_ASSERT_TRUE(c(b, true) == FailureClass::NotFound);
    }
    const uint16_t last[] = {15, 15, 201};  // the last reason decides
    TEST_ASSERT_TRUE(c(last, true) == FailureClass::NotFound);
}

static void test_wrong_password_needs_two_and_a_seen_network() {
    const uint16_t twice[] = {15, 15};
    TEST_ASSERT_TRUE(c(twice, true) == FailureClass::WrongPassword);
    const uint16_t once[] = {15};
    TEST_ASSERT_TRUE(c(once, true) == FailureClass::Other);
    const uint16_t unseen[] = {204, 204};
    TEST_ASSERT_TRUE(c(unseen, false) == FailureClass::Other);
    const uint16_t mixed[] = {202, 15};
    TEST_ASSERT_TRUE(c(mixed, true) == FailureClass::WrongPassword);
    const uint16_t group[] = {2, 14};
    TEST_ASSERT_TRUE(c(group, true) == FailureClass::WrongPassword);
}

static void test_other() {
    TEST_ASSERT_TRUE(classify_failure(nullptr, 0, true) == FailureClass::Other);  // timeout, no event
    const uint16_t a[] = {200, 205};
    TEST_ASSERT_TRUE(c(a, true) == FailureClass::Other);
    const uint16_t b[] = {15, 15, 205};  // last reason is not a password reason
    TEST_ASSERT_TRUE(c(b, true) == FailureClass::Other);
}

static void test_names() {
    TEST_ASSERT_EQUAL_STRING("none", failure_name(FailureClass::None));
    TEST_ASSERT_EQUAL_STRING("wrong_password", failure_name(FailureClass::WrongPassword));
    TEST_ASSERT_EQUAL_STRING("not_found", failure_name(FailureClass::NotFound));
    TEST_ASSERT_EQUAL_STRING("other", failure_name(FailureClass::Other));
}

void run_failure_tests() {
    RUN_TEST(test_not_found);
    RUN_TEST(test_wrong_password_needs_two_and_a_seen_network);
    RUN_TEST(test_other);
    RUN_TEST(test_names);
}
