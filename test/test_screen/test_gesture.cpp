// Hidden diagnostic gesture: two fingers held for 5 s (spec 003 FR-022, research R13).
// Written before the code.
#include <unity.h>

#include "nvp/screen/gesture.h"

using nvp::screen::TwoFingerHold;

static int feed_range(TwoFingerHold& g, uint32_t from, uint32_t to, uint8_t points) {
    int fired = 0;
    for (uint32_t t = from; t <= to; t += 33) fired += g.feed(t, points) ? 1 : 0;
    return fired;
}

static void test_fires_after_five_seconds() {
    TwoFingerHold g;
    TEST_ASSERT_FALSE(g.feed(1000, 2));
    TEST_ASSERT_FALSE(g.feed(5999, 2));  // 4999 ms
    TEST_ASSERT_TRUE(g.feed(6000, 2));   // 5000 ms
    TEST_ASSERT_FALSE(g.feed(6033, 2));  // once only
}

static void test_three_fingers_count() {
    TwoFingerHold g;
    g.feed(0, 3);
    TEST_ASSERT_TRUE(g.feed(5000, 2));
}

static void test_one_finger_never_fires() {
    TwoFingerHold g;
    TEST_ASSERT_EQUAL_INT(0, feed_range(g, 0, 10000, 1));
}

static void test_lifting_one_finger_resets() {
    TwoFingerHold g;
    feed_range(g, 0, 4000, 2);
    TEST_ASSERT_FALSE(g.feed(4033, 1));
    TEST_ASSERT_FALSE(g.feed(5100, 2));  // started again at 5100
    TEST_ASSERT_FALSE(g.feed(10099, 2));
    TEST_ASSERT_TRUE(g.feed(10100, 2));
}

static void test_no_second_fire_without_release() {
    TwoFingerHold g;
    TEST_ASSERT_EQUAL_INT(1, feed_range(g, 0, 20000, 2));
    feed_range(g, 20033, 20200, 0);  // released
    TEST_ASSERT_EQUAL_INT(1, feed_range(g, 21000, 27000, 2));
}

static void test_tap_does_nothing() {
    TwoFingerHold g;
    TEST_ASSERT_EQUAL_INT(0, feed_range(g, 0, 200, 1));
    TEST_ASSERT_EQUAL_INT(0, feed_range(g, 233, 2000, 0));
}

static void test_millisecond_wrap() {
    TwoFingerHold g;
    g.feed(0xFFFFF000u, 2);
    TEST_ASSERT_TRUE(g.feed(0xFFFFF000u + 5000u, 2));
}

void run_gesture_tests() {
    RUN_TEST(test_fires_after_five_seconds);
    RUN_TEST(test_three_fingers_count);
    RUN_TEST(test_one_finger_never_fires);
    RUN_TEST(test_lifting_one_finger_resets);
    RUN_TEST(test_no_second_fire_without_release);
    RUN_TEST(test_tap_does_nothing);
    RUN_TEST(test_millisecond_wrap);
}
