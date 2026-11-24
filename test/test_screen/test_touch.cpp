// Tap and hold on the main screen and the overlay (spec 004 FR-008, 02 §8, research R12).
// Written before the code.
#include <unity.h>

#include "nvp/screen/gesture.h"

using nvp::screen::Touch;
using nvp::screen::TouchClassifier;

// Feeds one finger at (x, y) from `from` to `to` ms in 20 ms steps and returns the first non-None result.
static Touch hold(TouchClassifier& c, uint32_t from, uint32_t to, int16_t x, int16_t y, uint8_t points = 1) {
    Touch got = Touch::None;
    for (uint32_t t = from; t <= to; t += 20) {
        const Touch r = c.feed(t, points, x, y);
        if (got == Touch::None) got = r;
    }
    return got;
}

static void test_tap() {
    TouchClassifier c;
    TEST_ASSERT_TRUE(hold(c, 1000, 1600, 400, 240) == Touch::None);  // down 600 ms
    TEST_ASSERT_TRUE(c.feed(1600, 0, 0, 0) == Touch::Tap);           // lift: tap
    TEST_ASSERT_TRUE(c.feed(1620, 0, 0, 0) == Touch::None);          // reported once
}

static void test_too_long_or_moved_is_not_a_tap() {
    TouchClassifier c;
    hold(c, 0, 620, 400, 240);  // 620 ms down
    TEST_ASSERT_TRUE(c.feed(640, 0, 0, 0) == Touch::None);
    TouchClassifier m;
    m.feed(0, 1, 400, 240);
    m.feed(100, 1, 425, 240);  // moved 25 px
    TEST_ASSERT_TRUE(m.feed(200, 0, 0, 0) == Touch::None);
    TouchClassifier ok;
    ok.feed(0, 1, 400, 240);
    ok.feed(100, 1, 424, 240);  // 24 px is still a tap
    TEST_ASSERT_TRUE(ok.feed(200, 0, 0, 0) == Touch::Tap);
}

static void test_hold_five_seconds_once() {
    TouchClassifier c;
    TEST_ASSERT_TRUE(hold(c, 0, 4980, 300, 200) == Touch::None);
    TEST_ASSERT_TRUE(c.feed(5000, 1, 300, 200) == Touch::Hold5s);
    TEST_ASSERT_TRUE(hold(c, 5020, 9000, 300, 200) == Touch::None);
    TEST_ASSERT_TRUE(c.feed(9020, 0, 0, 0) == Touch::None);  // no tap after a hold
}

static void test_second_finger_cancels_tap() {
    TouchClassifier c;
    c.feed(0, 1, 400, 240);
    c.feed(100, 2, 400, 240);
    c.feed(150, 1, 400, 240);
    TEST_ASSERT_TRUE(c.feed(200, 0, 0, 0) == Touch::None);
}

static void test_two_finger_hold_still_fires_and_never_taps() {
    TouchClassifier c;
    TEST_ASSERT_TRUE(hold(c, 0, 4980, 400, 240, 2) == Touch::None);
    TEST_ASSERT_TRUE(c.feed(5000, 2, 400, 240) == Touch::TwoFingerHold5s);
    TEST_ASSERT_TRUE(c.feed(5100, 0, 0, 0) == Touch::None);
}

void run_touch_tests() {
    RUN_TEST(test_tap);
    RUN_TEST(test_too_long_or_moved_is_not_a_tap);
    RUN_TEST(test_hold_five_seconds_once);
    RUN_TEST(test_second_finger_cancels_tap);
    RUN_TEST(test_two_finger_hold_still_fires_and_never_taps);
}
