// Edge cases of spec 007 (T023): location change mid-refresh, malformed data, a slow source, and
// a restart in the middle of the retry schedule (FR-004, FR-010, FR-014, analysis U7).
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

// Says stop before the second request (the location changed while İmsakiyem was answering).
class StopAfterFirst : public nvp::providers::RunControl {
public:
    int asked = 0;
    bool should_continue() override { return ++asked <= 1; }
};

void test_location_change_mid_refresh() {
    std::unique_ptr<Sim> s(new Sim());
    s->source.set(ProviderId::Imsakiyem, Mode::Failed);  // so that emushaf would be next
    s->start(kDay0, 10, 0);
    nvp::providers::RunInput in;
    in.district = 9206;
    in.today = kDay0;
    in.now_utc = s->now;
    StopAfterFirst stop;
    const nvp::providers::RunResult r =
        nvp::providers::refresh_run(s->source, s->fs, in, s->store, s->scratch, s->state, &stop);
    TEST_ASSERT_TRUE(r.abandoned);
    TEST_ASSERT_FALSE(r.saved);
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(s->source.calls.size()));
    TEST_ASSERT_EQUAL_UINT16(0, s->store.count);
    TEST_ASSERT_FALSE(s->fs.exists("store_9206.bin"));
    // The new district refreshes at once.
    s->source.set(ProviderId::Imsakiyem, Mode::Ok);
    s->district = 9541;
    s->store = nvp::store::Store{};
    s->store.district_id = 9541;
    s->planner.on_location_changed();
    s->advance_minutes(1);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Trigger::LocationChanged), static_cast<int>(s->last_trigger));
    TEST_ASSERT_TRUE(s->fs.exists("store_9541.bin"));
}

void test_malformed_mirror_other_used() {
    std::unique_ptr<Sim> s(new Sim());
    s->source.set(ProviderId::Imsakiyem, Mode::Malformed);
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::providers::Reason::Malformed),
                          static_cast<int>(s->last.providers[static_cast<size_t>(ProviderId::Imsakiyem)].result.reason));
    const nvp::engine::DayRecord* r = nvp::store::find(s->store, kDay0);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::engine::SourceId::Emushaf), static_cast<int>(r->source));
}

void test_slow_source_counts_as_failure() {
    std::unique_ptr<Sim> s(new Sim());
    s->source.set(ProviderId::Imsakiyem, Mode::Timeout);
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);
    TEST_ASSERT_EQUAL_UINT8(1, s->state.providers[static_cast<size_t>(ProviderId::Imsakiyem)].failures);
    TEST_ASSERT_EQUAL_INT(1, s->source.count(ProviderId::Emushaf));
}

// The state is saved after a refresh; a restarted device continues the same retry schedule.
void test_restart_keeps_retry_schedule() {
    std::unique_ptr<Sim> s(new Sim());
    s->has_coords = false;
    s->source.set(ProviderId::Imsakiyem, Mode::Failed);
    s->source.set(ProviderId::Emushaf, Mode::Failed);
    s->start(kDay0, 10, 0);
    s->advance_minutes(6);  // boot, then the 5-minute retry
    TEST_ASSERT_EQUAL_INT(2, s->refreshes);
    TEST_ASSERT_EQUAL_UINT8(2, s->state.retry_step);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::store::SaveResult::Ok),
                          static_cast<int>(nvp::store::save_refresh_state(s->fs, s->state)));

    std::unique_ptr<Sim> t(new Sim());
    t->fs = s->fs;
    t->has_coords = false;
    t->source.set(ProviderId::Imsakiyem, Mode::Failed);
    t->source.set(ProviderId::Emushaf, Mode::Failed);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::store::LoadResult::Ok),
                          static_cast<int>(nvp::store::load_refresh_state(t->fs, t->state)));
    TEST_ASSERT_TRUE(t->state == s->state);
    TEST_ASSERT_EQUAL_UINT8(2, t->state.retry_step);
    TEST_ASSERT_EQUAL_INT64(s->state.next_retry_utc, t->state.next_retry_utc);
}

// FR-016: both mirrors implausible (shifted by an hour) and agreeing: published data kept, the
// coordinates marked suspect and persisted in the state; the screen is not calculated.
void test_two_mirrors_agree_but_implausible() {
    std::unique_ptr<Sim> s(new Sim());
    s->source.behaviour[static_cast<size_t>(ProviderId::Imsakiyem)].hour_shift = 1;
    s->source.behaviour[static_cast<size_t>(ProviderId::Emushaf)].hour_shift = 1;
    s->start(kDay0, 10, 0);
    s->advance_minutes(1);
    TEST_ASSERT_TRUE(s->last.coords_suspect_set);
    TEST_ASSERT_TRUE(s->state.coords_suspect);
    const nvp::engine::DayRecord* r = nvp::store::find(s->store, kDay0);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_UINT8(40, nvp::engine::rank(r->source));
    TEST_ASSERT_EQUAL_INT(0, s->source.count(ProviderId::Aladhan));
    // The next refresh does not check plausibility any more (the coordinates are suspect).
    TEST_ASSERT_TRUE(s->planner.on_manual(s->now));
    s->step();
    TEST_ASSERT_FALSE(s->last.providers[static_cast<size_t>(ProviderId::Imsakiyem)].plausibility.checked);
}

// Only one mirror implausible, or both implausible but disagreeing: rejected.
void test_implausible_mirrors_rejected() {
    {
        std::unique_ptr<Sim> s(new Sim());
        s->source.behaviour[static_cast<size_t>(ProviderId::Imsakiyem)].hour_shift = 1;
        s->start(kDay0, 10, 0);
        s->advance_minutes(1);
        TEST_ASSERT_FALSE(s->state.coords_suspect);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::providers::Reason::Implausible),
                              static_cast<int>(s->last.providers[static_cast<size_t>(ProviderId::Imsakiyem)].result.reason));
        const nvp::engine::DayRecord* r = nvp::store::find(s->store, kDay0);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::engine::SourceId::Emushaf), static_cast<int>(r->source));
    }
    {
        std::unique_ptr<Sim> s(new Sim());
        s->source.behaviour[static_cast<size_t>(ProviderId::Imsakiyem)].hour_shift = 1;
        s->source.behaviour[static_cast<size_t>(ProviderId::Emushaf)].hour_shift = 2;
        s->start(kDay0, 10, 0);
        s->advance_minutes(1);
        TEST_ASSERT_FALSE(s->state.coords_suspect);
        const nvp::engine::DayRecord* r = nvp::store::find(s->store, kDay0);
        TEST_ASSERT_NOT_NULL(r);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::engine::SourceId::Aladhan), static_cast<int>(r->source));
    }
}

}  // namespace

void run_edge_case_tests() {
    RUN_TEST(test_two_mirrors_agree_but_implausible);
    RUN_TEST(test_implausible_mirrors_rejected);
    RUN_TEST(test_location_change_mid_refresh);
    RUN_TEST(test_malformed_mirror_other_used);
    RUN_TEST(test_slow_source_counts_as_failure);
    RUN_TEST(test_restart_keeps_retry_schedule);
}
