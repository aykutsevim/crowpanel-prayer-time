// Diyanet data first (spec 007 T011; FR-001, FR-002, FR-007, SC-006, research R6, owner direction).
#include <unity.h>

#include <memory>

#include "../support/scripted_source.h"

using nvp::providers::ProviderId;
using nvp::store::RefreshOutcome;
using nvp_test::Date;
using nvp_test::Mode;
using nvp_test::Sim;

namespace {

const Date kDay0{2026, 11, 26};

std::unique_ptr<Sim> boot(void (*setup)(Sim&)) {
    std::unique_ptr<Sim> s(new Sim());
    if (setup != nullptr) setup(*s);
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);
    return s;
}

// Every Diyanet-derived provider that was asked comes before any calculated one.
void assert_diyanet_first(const Sim& s) {
    bool calculated_seen = false;
    for (const nvp_test::Call& c : s.source.calls) {
        if (nvp::providers::diyanet_derived(c.provider)) {
            TEST_ASSERT_FALSE_MESSAGE(calculated_seen, "a Diyanet-derived provider asked after a calculated one");
        } else {
            calculated_seen = true;
        }
    }
}

void test_all_up_only_imsakiyem() {
    auto s = boot(nullptr);
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(s->source.calls.size()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ProviderId::Imsakiyem), static_cast<int>(s->source.calls[0].provider));
    TEST_ASSERT_EQUAL_INT(1, s->last.requests);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(RefreshOutcome::Complete), static_cast<int>(s->last.outcome));
    assert_diyanet_first(*s);
}

void test_short_imsakiyem_then_emushaf_never_aladhan() {
    auto s = boot([](Sim& x) {
        x.source.set_last(ProviderId::Imsakiyem, nvp_test::plus_days(kDay0, 20));
        x.source.set_last(ProviderId::Emushaf, nvp_test::plus_days(kDay0, 31));
    });
    TEST_ASSERT_EQUAL_INT(0, s->source.first(ProviderId::Imsakiyem));
    TEST_ASSERT_EQUAL_INT(1, s->source.first(ProviderId::Emushaf));
    TEST_ASSERT_EQUAL_INT(0, s->source.count(ProviderId::Aladhan));
    TEST_ASSERT_EQUAL_INT(30, s->last.days_ahead);  // emushaf's request is today .. today + 30
    assert_diyanet_first(*s);
}

void test_both_mirrors_down_then_aladhan() {
    auto s = boot([](Sim& x) {
        x.source.set(ProviderId::Imsakiyem, Mode::Failed);
        x.source.set(ProviderId::Emushaf, Mode::Failed);
    });
    TEST_ASSERT_EQUAL_INT(0, s->source.first(ProviderId::Imsakiyem));
    TEST_ASSERT_EQUAL_INT(1, s->source.first(ProviderId::Emushaf));
    TEST_ASSERT_EQUAL_INT(2, s->source.first(ProviderId::Aladhan));
    // 26 November: 4 days to month end, so November and December.
    TEST_ASSERT_EQUAL_INT(2, s->source.count(ProviderId::Aladhan));
    const nvp::engine::DayRecord* today = nvp::store::find(s->store, kDay0);
    TEST_ASSERT_NOT_NULL(today);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::engine::SourceId::Aladhan), static_cast<int>(today->source));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(RefreshOutcome::Failed), static_cast<int>(s->last.outcome));
    assert_diyanet_first(*s);
}

// Today is published, tomorrow is not: Aladhan fills tomorrow on, but never touches today.
void test_aladhan_never_overwrites_published_day() {
    auto s = boot([](Sim& x) { x.source.set_last(ProviderId::Imsakiyem, kDay0); x.source.set_last(ProviderId::Emushaf, kDay0); });
    const nvp::engine::DayRecord* today = nvp::store::find(s->store, kDay0);
    TEST_ASSERT_NOT_NULL(today);
    TEST_ASSERT_EQUAL_UINT8(40, nvp::engine::rank(today->source));  // either mirror, never Aladhan
    const nvp::engine::DayRecord* tomorrow = nvp::store::find(s->store, nvp_test::plus_days(kDay0, 1));
    TEST_ASSERT_NOT_NULL(tomorrow);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::engine::SourceId::Aladhan), static_cast<int>(tomorrow->source));
    assert_diyanet_first(*s);
}

// Aladhan is not asked when every day of the month already has a record (no repeated calls).
void test_aladhan_only_for_gaps() {
    auto s = boot([](Sim& x) {
        x.source.set(ProviderId::Imsakiyem, Mode::Failed);
        x.source.set(ProviderId::Emushaf, Mode::Failed);
    });
    const int first = s->source.count(ProviderId::Aladhan);
    TEST_ASSERT_TRUE(s->planner.on_manual(s->now));
    s->step();
    TEST_ASSERT_EQUAL_INT(first, s->source.count(ProviderId::Aladhan));
    TEST_ASSERT_EQUAL_STRING("covered", nvp::providers::skip_text(s->last.providers[static_cast<size_t>(ProviderId::Aladhan)].skip));
}

void test_no_coordinates_no_aladhan() {
    auto s = boot([](Sim& x) {
        x.has_coords = false;
        x.source.set(ProviderId::Imsakiyem, Mode::Failed);
        x.source.set(ProviderId::Emushaf, Mode::Failed);
    });
    TEST_ASSERT_EQUAL_INT(0, s->source.count(ProviderId::Aladhan));
    TEST_ASSERT_EQUAL_INT(0, s->store.count);  // no_data stays (04 §12 row 7)
}

}  // namespace

void run_runner_order_tests() {
    RUN_TEST(test_all_up_only_imsakiyem);
    RUN_TEST(test_short_imsakiyem_then_emushaf_never_aladhan);
    RUN_TEST(test_both_mirrors_down_then_aladhan);
    RUN_TEST(test_aladhan_never_overwrites_published_day);
    RUN_TEST(test_aladhan_only_for_gaps);
    RUN_TEST(test_no_coordinates_no_aladhan);
}
