// Corner-touch judge (FR-014a, data-model.md CornerTouchResult). Written before the code.
#include <unity.h>

#include "nvp/diag/touch_judge.h"

using namespace nvp::diag;

static void test_target_centres() {
    TEST_ASSERT_EQUAL_INT16(20, kCornerTargets[0].x);
    TEST_ASSERT_EQUAL_INT16(20, kCornerTargets[0].y);
    TEST_ASSERT_EQUAL_INT16(779, kCornerTargets[1].x);
    TEST_ASSERT_EQUAL_INT16(20, kCornerTargets[1].y);
    TEST_ASSERT_EQUAL_INT16(20, kCornerTargets[2].x);
    TEST_ASSERT_EQUAL_INT16(459, kCornerTargets[2].y);
    TEST_ASSERT_EQUAL_INT16(779, kCornerTargets[3].x);
    TEST_ASSERT_EQUAL_INT16(459, kCornerTargets[3].y);
}

static void test_all_start_not_touched() {
    TouchJudge j;
    for (size_t i = 0; i < kCornerCount; ++i) {
        TEST_ASSERT_TRUE(j.result(static_cast<Corner>(i)).state == CornerState::NotTouched);
    }
}

static void test_exact_hit_passes() {
    TouchJudge j;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Corner::TopLeft), j.judge(20, 20));
    const CornerResult& r = j.result(Corner::TopLeft);
    TEST_ASSERT_TRUE(r.state == CornerState::Passed);
    TEST_ASSERT_EQUAL_INT16(0, r.dx);
    TEST_ASSERT_EQUAL_INT16(0, r.dy);
}

static void test_ten_pixels_pass_eleven_fail() {
    TouchJudge j;
    j.judge(779 + 10, 20 - 10);
    TEST_ASSERT_TRUE(j.result(Corner::TopRight).state == CornerState::Passed);
    j.judge(779 - 11, 20);
    TEST_ASSERT_TRUE(j.result(Corner::TopRight).state == CornerState::Failed);
    j.judge(779, 20 + 11);
    TEST_ASSERT_TRUE(j.result(Corner::TopRight).state == CornerState::Failed);
}

static void test_signed_errors() {
    TouchJudge j;
    j.judge(20 + 14, 459 + 3);
    const CornerResult& r = j.result(Corner::BottomLeft);
    TEST_ASSERT_EQUAL_INT16(14, r.dx);
    TEST_ASSERT_EQUAL_INT16(3, r.dy);
    j.judge(779 - 4, 459 - 7);
    TEST_ASSERT_EQUAL_INT16(-4, j.result(Corner::BottomRight).dx);
    TEST_ASSERT_EQUAL_INT16(-7, j.result(Corner::BottomRight).dy);
}

static void test_assignment_only_within_60_px() {
    TouchJudge j;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Corner::TopLeft), j.judge(20 + 60, 20 + 60));
    TouchJudge k;
    TEST_ASSERT_EQUAL_INT(-1, k.judge(20 + 61, 20));
    TEST_ASSERT_EQUAL_INT(-1, k.judge(400, 240));
    for (size_t i = 0; i < kCornerCount; ++i) {
        TEST_ASSERT_TRUE(k.result(static_cast<Corner>(i)).state == CornerState::NotTouched);
    }
}

static void test_latest_touch_wins() {
    TouchJudge j;
    j.judge(20 + 30, 20);  // failed
    TEST_ASSERT_TRUE(j.result(Corner::TopLeft).state == CornerState::Failed);
    j.judge(21, 19);       // passed
    TEST_ASSERT_TRUE(j.result(Corner::TopLeft).state == CornerState::Passed);
    TEST_ASSERT_EQUAL_INT16(1, j.result(Corner::TopLeft).dx);
    TEST_ASSERT_EQUAL_INT16(-1, j.result(Corner::TopLeft).dy);
}

static void test_other_corners_untouched_by_a_judgement() {
    TouchJudge j;
    j.judge(779, 459);
    TEST_ASSERT_TRUE(j.result(Corner::BottomRight).state == CornerState::Passed);
    TEST_ASSERT_TRUE(j.result(Corner::TopLeft).state == CornerState::NotTouched);
    TEST_ASSERT_TRUE(j.result(Corner::TopRight).state == CornerState::NotTouched);
    TEST_ASSERT_TRUE(j.result(Corner::BottomLeft).state == CornerState::NotTouched);
}

static void test_out_of_range_coordinates_are_ignored() {
    TouchJudge j;
    TEST_ASSERT_EQUAL_INT(-1, j.judge(-5, -5));
    TEST_ASSERT_EQUAL_INT(-1, j.judge(900, 600));
}

void run_touch_judge_tests() {
    RUN_TEST(test_target_centres);
    RUN_TEST(test_all_start_not_touched);
    RUN_TEST(test_exact_hit_passes);
    RUN_TEST(test_ten_pixels_pass_eleven_fail);
    RUN_TEST(test_signed_errors);
    RUN_TEST(test_assignment_only_within_60_px);
    RUN_TEST(test_latest_touch_wins);
    RUN_TEST(test_other_corners_untouched_by_a_judgement);
    RUN_TEST(test_out_of_range_coordinates_are_ignored);
}
