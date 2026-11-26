// One mirror down goes unnoticed (spec 007 T013; US2, FR-007, FR-012, 04 §4, research R1, R6).
#include <unity.h>

#include <cstdio>
#include <memory>

#include "../support/scripted_source.h"

using nvp::providers::ProviderId;
using nvp::store::RefreshOutcome;
using nvp_test::Date;
using nvp_test::Mode;
using nvp_test::Sim;

namespace {

const Date kDay0{2026, 11, 26};

void test_imsakiyem_down_emushaf_covers() {
    std::unique_ptr<Sim> s(new Sim());
    s->source.set(ProviderId::Imsakiyem, Mode::Failed);
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);
    TEST_ASSERT_EQUAL_INT(30, s->last.days_ahead);
    const nvp::engine::DayRecord* r = nvp::store::find(s->store, nvp_test::plus_days(kDay0, 30));
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::engine::SourceId::Emushaf), static_cast<int>(r->source));
    // 30 days reached: the goal holds although İmsakiyem failed.
    TEST_ASSERT_EQUAL_INT(static_cast<int>(RefreshOutcome::Complete), static_cast<int>(s->last.outcome));
}

// emushaf's cached month reaches only 24 days ahead (research R1): partial, and a retry follows.
void test_imsakiyem_down_short_emushaf_is_partial() {
    std::unique_ptr<Sim> s(new Sim());
    s->source.set(ProviderId::Imsakiyem, Mode::Failed);
    s->source.set_last(ProviderId::Emushaf, nvp_test::plus_days(kDay0, 24));
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);
    TEST_ASSERT_EQUAL_INT(24, s->last.days_ahead);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(RefreshOutcome::Partial), static_cast<int>(s->last.outcome));
    TEST_ASSERT_EQUAL_INT64(s->now - 60 + 5 * 60, s->state.next_retry_utc);
}

// Year end: İmsakiyem stops at 31 December, emushaf reaches into January. Both answered, so the
// refresh is complete and no retry follows. The request sequence is printed for the report.
void test_year_end() {
    std::unique_ptr<Sim> s(new Sim());
    const Date day{2026, 12, 20};
    s->source.set_last(ProviderId::Imsakiyem, Date{2026, 12, 31});
    s->source.set_last(ProviderId::Emushaf, Date{2027, 1, 20});
    s->start(day, 10, 0);
    s->advance_minutes(1);
    std::printf("year end, request sequence:\n");
    for (const nvp_test::Call& c : s->source.calls) {
        std::printf("  %s %04d-%02d-%02d..%04d-%02d-%02d -> %s\n", nvp::providers::provider_key(c.provider),
                    c.from.year, c.from.month, c.from.day, c.to.year, c.to.month, c.to.day,
                    nvp::providers::fetch_outcome_text(c.outcome));
    }
    s->advance_minutes(3 * 1440);
    s->print_log("year end, refreshes over 3 days:");
    TEST_ASSERT_TRUE(s->source.calls.size() >= 2);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ProviderId::Imsakiyem), static_cast<int>(s->source.calls[0].provider));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ProviderId::Emushaf), static_cast<int>(s->source.calls[1].provider));
    const nvp::engine::DayRecord* last = &s->store.records[s->store.count - 1];
    TEST_ASSERT_TRUE(last->date == (Date{2027, 1, 20}));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::engine::SourceId::Emushaf), static_cast<int>(last->source));
    TEST_ASSERT_EQUAL_INT(0, s->state.retry_step);
}

// İmsakiyem has published only 20 days, emushaf its cached 24: both answered, so complete, no retry.
void test_short_but_all_answered_is_complete() {
    std::unique_ptr<Sim> s(new Sim());
    s->source.set_last(ProviderId::Imsakiyem, nvp_test::plus_days(kDay0, 20));
    s->source.set_last(ProviderId::Emushaf, nvp_test::plus_days(kDay0, 24));
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);
    TEST_ASSERT_EQUAL_INT(24, s->last.days_ahead);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(RefreshOutcome::Complete), static_cast<int>(s->last.outcome));
    TEST_ASSERT_EQUAL_INT64(0, s->state.next_retry_utc);
}

// The mirrors disagree by a minute: the newer kept, the mismatch counted with both time sets.
void test_mismatch_recorded() {
    std::unique_ptr<Sim> s(new Sim());
    s->source.set_last(ProviderId::Imsakiyem, nvp_test::plus_days(kDay0, 20));
    s->source.behaviour[static_cast<size_t>(ProviderId::Emushaf)].shift = 1;
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);
    TEST_ASSERT_TRUE(s->last.mismatches > 0);
    TEST_ASSERT_TRUE(s->last.has_mismatch);
    TEST_ASSERT_EQUAL_UINT16(s->last.mismatch_stored.minutes[0] + 1, s->last.mismatch_incoming.minutes[0]);
    const nvp::engine::DayRecord* r = nvp::store::find(s->store, kDay0);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::engine::SourceId::Emushaf), static_cast<int>(r->source));
}

// 04 §12 row 5: both mirrors down while the store covers today: nothing changes on screen (today
// stays published, Aladhan is not asked), and the retries continue.
void test_both_mirrors_down_store_covers_today() {
    std::unique_ptr<Sim> s(new Sim());
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);  // a year from İmsakiyem
    s->source.set(ProviderId::Imsakiyem, Mode::Failed);
    s->source.set(ProviderId::Emushaf, Mode::Failed);
    TEST_ASSERT_TRUE(s->planner.on_manual(s->now));
    s->step();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(RefreshOutcome::Complete), static_cast<int>(s->last.outcome));  // 30+ days stored
    TEST_ASSERT_EQUAL_INT(0, s->source.count(ProviderId::Aladhan));
    TEST_ASSERT_FALSE(s->today_calculated());
    // With fewer than 30 days left the failure counts: partial or failed, and a retry is planned.
    std::unique_ptr<Sim> t(new Sim());
    t->source.set_last(ProviderId::Imsakiyem, nvp_test::plus_days(kDay0, 10));
    t->source.set_last(ProviderId::Emushaf, nvp_test::plus_days(kDay0, 10));
    t->start(kDay0, 10, 0);
    t->advance_minutes(1);
    t->source.set(ProviderId::Imsakiyem, Mode::Failed);
    t->source.set(ProviderId::Emushaf, Mode::Failed);
    TEST_ASSERT_TRUE(t->planner.on_manual(t->now));
    t->step();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(RefreshOutcome::Failed), static_cast<int>(t->last.outcome));
    TEST_ASSERT_EQUAL_INT(0, t->source.count(ProviderId::Aladhan));
    TEST_ASSERT_FALSE(t->today_calculated());
    TEST_ASSERT_EQUAL_INT64(t->now + 5 * 60, t->state.next_retry_utc);
}

}  // namespace

void run_runner_mirror_tests() {
    RUN_TEST(test_both_mirrors_down_store_covers_today);
    RUN_TEST(test_imsakiyem_down_emushaf_covers);
    RUN_TEST(test_imsakiyem_down_short_emushaf_is_partial);
    RUN_TEST(test_year_end);
    RUN_TEST(test_short_but_all_answered_is_complete);
    RUN_TEST(test_mismatch_recorded);
}
