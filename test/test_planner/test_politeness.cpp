// Gentle on the services (spec 007 T021; US8, FR-003, FR-008, FR-010..FR-013, SC-001, research R6,
// analysis C1, I2).
#include <unity.h>

#include <cstdio>
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
constexpr size_t kIms = static_cast<size_t>(ProviderId::Imsakiyem);

void test_rate_limit_cools_down_30_minutes() {
    std::unique_ptr<Sim> s(new Sim());
    s->source.set(ProviderId::Imsakiyem, Mode::RateLimited);
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);
    TEST_ASSERT_EQUAL_INT(1, s->source.count(ProviderId::Imsakiyem));
    s->source.set(ProviderId::Imsakiyem, Mode::Ok);
    s->advance_minutes(10);
    TEST_ASSERT_TRUE(s->planner.on_manual(s->now));
    s->step();  // even a manual refresh respects the cool-down
    TEST_ASSERT_EQUAL_INT(1, s->source.count(ProviderId::Imsakiyem));
    TEST_ASSERT_EQUAL_STRING("cooling down", nvp::providers::skip_text(s->last.providers[kIms].skip));
    s->advance_minutes(25);
    TEST_ASSERT_TRUE(s->planner.on_manual(s->now));
    s->step();
    TEST_ASSERT_EQUAL_INT(2, s->source.count(ProviderId::Imsakiyem));
}

// Five failed refreshes in a row: skipped for 24 h (today published, so no exemption).
void test_five_failures_skip_24h() {
    std::unique_ptr<Sim> s(new Sim());
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);  // a full store from İmsakiyem
    s->source.set(ProviderId::Imsakiyem, Mode::Failed);
    for (int i = 0; i < 5; ++i) {
        s->advance_minutes(2);
        TEST_ASSERT_TRUE(s->planner.on_manual(s->now));
        s->step();
    }
    TEST_ASSERT_EQUAL_UINT8(5, s->state.providers[kIms].failures);
    TEST_ASSERT_TRUE(s->state.providers[kIms].skip_until > s->now);
    const int asked = s->source.count(ProviderId::Imsakiyem);
    s->advance_minutes(2);
    s->planner.on_manual(s->now);
    s->step();
    TEST_ASSERT_EQUAL_INT(asked, s->source.count(ProviderId::Imsakiyem));
    TEST_ASSERT_EQUAL_STRING("skipped 24 h", nvp::providers::skip_text(s->last.providers[kIms].skip));
    s->source.set(ProviderId::Imsakiyem, Mode::Ok);
    s->advance_minutes(24 * 60);
    s->planner.on_manual(s->now);
    s->step();
    TEST_ASSERT_EQUAL_INT(asked + 1, s->source.count(ProviderId::Imsakiyem));
    TEST_ASSERT_EQUAL_UINT8(0, s->state.providers[kIms].failures);
}

void test_retry_schedule() {
    nvp::store::RefreshState st;
    nvp::providers::Planner p(st);
    const int64_t want[] = {300, 900, 3600, 10800, 21600, 21600};
    int64_t t = 1000;
    for (int64_t w : want) {
        p.on_finished(RefreshOutcome::Failed, false, t, false);
        TEST_ASSERT_EQUAL_INT64(t + w, st.next_retry_utc);
        t = st.next_retry_utc;
    }
    p.on_finished(RefreshOutcome::Complete, true, t, false);
    TEST_ASSERT_EQUAL_INT64(0, st.next_retry_utc);
    TEST_ASSERT_EQUAL_UINT8(0, st.retry_step);
}

// A pending retry: a reconnection brings it forward (one extra attempt).
void test_reconnect_extra_attempt() {
    std::unique_ptr<Sim> s(new Sim());
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);  // full store
    s->source.set(ProviderId::Imsakiyem, Mode::Failed);
    s->source.set(ProviderId::Emushaf, Mode::Failed);
    s->state.retry_step = 2;  // as after two failed refreshes; next in 1 h
    s->state.next_retry_utc = s->now + 3600;
    s->link(false);
    s->advance_minutes(3);
    s->link(true);
    s->advance_minutes(1);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Trigger::Reconnect), static_cast<int>(s->last_trigger));
}

// Wi-Fi flapping (every few seconds, here every minute) for an hour: at most 6 refreshes.
void test_reconnect_storm() {
    std::unique_ptr<Sim> s(new Sim());
    s->source.set_last(ProviderId::Imsakiyem, nvp_test::plus_days(kDay0, 20));
    s->source.set_last(ProviderId::Emushaf, nvp_test::plus_days(kDay0, 20));
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);
    const int before = s->refreshes;
    for (int i = 0; i < 60; ++i) {
        s->link(false);
        s->link(true);
        s->advance_minutes(1);
    }
    TEST_ASSERT_TRUE(s->refreshes - before <= 6);
    TEST_ASSERT_TRUE(s->refreshes - before >= 5);
}

// Today calculated: the mirrors are retried every hour for 30 hours, without the 24 h skip, and
// each retry asks only İmsakiyem v2 and emushaf (analysis C1, I2).
void test_calculated_mirrors_never_skipped() {
    std::unique_ptr<Sim> s(new Sim());
    s->source.set(ProviderId::Imsakiyem, Mode::Failed);
    s->source.set(ProviderId::Emushaf, Mode::Failed);
    s->start(kDay0, 10, 0);
    s->advance_minutes(30 * 60);
    TEST_ASSERT_TRUE(s->state.providers[kIms].failures >= 5);
    TEST_ASSERT_TRUE(s->source.count(ProviderId::Imsakiyem) >= 30);
    TEST_ASSERT_TRUE(s->source.count(ProviderId::Emushaf) >= 30);
    // Only the boot refresh and the night refresh may use the v1 fallback; every hourly retry is lite.
    int full = 0;
    for (const nvp_test::Call& c : s->source.calls)
        if (c.provider == ProviderId::Imsakiyem && c.allow_fallback) ++full;
    TEST_ASSERT_TRUE(full <= 2);
    TEST_ASSERT_TRUE(s->last.requests <= 2);  // İmsakiyem v2 + emushaf; Aladhan's months are covered
    s->source.set(ProviderId::Emushaf, Mode::Ok);
    s->advance_minutes(61);
    const nvp::engine::DayRecord* today = nvp::store::find(s->store, s->today());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::engine::SourceId::Emushaf), static_cast<int>(today->source));
}

// The request budget (SC-001, research R6): a normal week ≤ 2; everything failing with today
// calculated ≤ 400; no refresh above 12.
void test_budget() {
    {
        std::unique_ptr<Sim> s(new Sim());
        s->start(kDay0, 3, 0);
        s->advance_minutes(1);
        const int before = s->total_requests;
        s->advance_minutes(7 * 1440 + 60);  // through the weekly refresh of day 7
        std::printf("budget, normal week: %d requests\n", s->total_requests - before);
        TEST_ASSERT_TRUE(s->total_requests - before <= 2);
    }
    {
        std::unique_ptr<Sim> s(new Sim());
        s->source.set(ProviderId::Imsakiyem, Mode::Failed);
        s->source.set(ProviderId::Emushaf, Mode::Failed);
        s->source.set(ProviderId::Aladhan, Mode::Failed);
        s->start(kDay0, 10, 0);
        int most = 0;
        for (int m = 0; m < 7 * 1440; ++m) {
            if (s->step() && s->last.requests > most) most = s->last.requests;
            s->now += 60;
        }
        std::printf("budget, everything failing for a week: %d requests in %d refreshes, at most %d in one\n",
                    s->total_requests, s->refreshes, most);
        TEST_ASSERT_TRUE(s->total_requests <= 400);
        TEST_ASSERT_TRUE(most <= 12);
    }
    {
        std::unique_ptr<Sim> s(new Sim());
        s->has_coords = false;  // not calculated: the ordinary backoff and the 24 h skip
        s->source.set(ProviderId::Imsakiyem, Mode::Failed);
        s->source.set(ProviderId::Emushaf, Mode::Failed);
        s->start(kDay0, 10, 0);
        s->advance_minutes(7 * 1440);
        std::printf("budget, both mirrors failing, no coordinates: %d requests in %d refreshes\n",
                    s->total_requests, s->refreshes);
        TEST_ASSERT_TRUE(s->total_requests <= 400);
    }
}

}  // namespace

void run_politeness_tests() {
    RUN_TEST(test_rate_limit_cools_down_30_minutes);
    RUN_TEST(test_five_failures_skip_24h);
    RUN_TEST(test_retry_schedule);
    RUN_TEST(test_reconnect_extra_attempt);
    RUN_TEST(test_reconnect_storm);
    RUN_TEST(test_calculated_mirrors_never_skipped);
    RUN_TEST(test_budget);
}
