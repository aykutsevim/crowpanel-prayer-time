// Approximate, and back to published by itself (spec 007 T017; US3, FR-003, FR-007, FR-012,
// 04 §12 rows 6 and 7).
#include <unity.h>

#include <memory>

#include "../support/scripted_source.h"

using nvp::providers::ProviderId;
using nvp::providers::Trigger;
using nvp::store::RefreshOutcome;
using nvp_test::Date;
using nvp_test::Mode;
using nvp_test::Sim;

namespace {

const Date kDay0{2026, 11, 26};

void test_calculated_then_published_returns_within_an_hour() {
    std::unique_ptr<Sim> s(new Sim());
    s->source.set(ProviderId::Imsakiyem, Mode::Failed);
    s->source.set(ProviderId::Emushaf, Mode::Failed);
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);
    const nvp::engine::DayRecord* today = nvp::store::find(s->store, kDay0);
    TEST_ASSERT_NOT_NULL(today);
    TEST_ASSERT_EQUAL_UINT8(20, nvp::engine::rank(today->source));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(RefreshOutcome::Failed), static_cast<int>(s->last.outcome));
    TEST_ASSERT_EQUAL_INT64(s->now - 60 + 3600, s->state.next_retry_utc);  // hourly, not 5 minutes

    s->source.set(ProviderId::Imsakiyem, Mode::Ok);
    s->advance_minutes(70);
    s->print_log("calculated, then İmsakiyem returns:");
    today = nvp::store::find(s->store, kDay0);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::engine::SourceId::Imsakiyem), static_cast<int>(today->source));
    // T045 (constitution II): yesterday is published again too (the night starts at its Yatsı).
    const nvp::engine::DayRecord* yesterday = nvp::store::find(s->store, nvp_test::plus_days(kDay0, -1));
    TEST_ASSERT_NOT_NULL(yesterday);
    TEST_ASSERT_EQUAL_UINT8(40, nvp::engine::rank(yesterday->source));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(RefreshOutcome::Complete), static_cast<int>(s->last.outcome));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Trigger::Retry), static_cast<int>(s->last_trigger));
    TEST_ASSERT_EQUAL_INT64(0, s->state.next_retry_utc);
    TEST_ASSERT_EQUAL_INT(2, s->refreshes);
    // The retry was lite: İmsakiyem without its v1 fallback.
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ProviderId::Imsakiyem), static_cast<int>(s->source.calls.back().provider));
    TEST_ASSERT_FALSE(s->source.calls.back().allow_fallback);
}

void test_no_coordinates_no_data() {
    std::unique_ptr<Sim> s(new Sim());
    s->has_coords = false;
    s->source.set(ProviderId::Imsakiyem, Mode::Failed);
    s->source.set(ProviderId::Emushaf, Mode::Failed);
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);
    TEST_ASSERT_EQUAL_INT(0, s->source.count(ProviderId::Aladhan));
    TEST_ASSERT_NULL(nvp::store::find(s->store, kDay0));
    // Not calculated (nothing to show), so the ordinary backoff applies: 5 minutes.
    TEST_ASSERT_EQUAL_INT64(s->now - 60 + 5 * 60, s->state.next_retry_utc);
}

}  // namespace

void run_calculated_tests() {
    RUN_TEST(test_calculated_then_published_returns_within_an_hour);
    RUN_TEST(test_no_coordinates_no_data);
}
