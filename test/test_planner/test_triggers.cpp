// Refresh triggers over simulated weeks (spec 007 T009; FR-004, FR-005, FR-006, SC-001, research R5).
#include <unity.h>

#include <memory>

#include "../support/scripted_source.h"

using nvp::providers::Trigger;
using nvp_test::Date;
using nvp_test::Sim;

namespace {

const Date kDay0{2026, 11, 26};

std::unique_ptr<Sim> make() { return std::unique_ptr<Sim>(new Sim()); }

// A device that refreshed fully `days_ago` days ago (İmsakiyem publishes a year).
void settled(Sim& s, int hh, int mm) {
    s.start(kDay0, hh, mm);
    s.advance_minutes(1);  // the boot refresh
    TEST_ASSERT_EQUAL_INT(1, s.refreshes);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Trigger::Boot), static_cast<int>(s.last_trigger));
}

void test_boot_with_empty_store_refreshes() {
    auto s = make();
    settled(*s, 10, 0);
    TEST_ASSERT_EQUAL_STRING("complete", nvp::store::refresh_outcome_text(s->last.outcome));
    TEST_ASSERT_TRUE(s->last.days_ahead >= 30);
}

void test_boot_with_fresh_full_store_does_not_refresh() {
    auto s = make();
    settled(*s, 10, 0);
    // Restart one day later: today covered, 364 days ahead, last success 1 day ago.
    s->now += 86400;
    s->planner.on_boot();
    const int before = s->refreshes;
    s->advance_minutes(5);
    TEST_ASSERT_EQUAL_INT(before, s->refreshes);
}

void test_boot_conditions() {
    // Under 30 days ahead.
    {
        auto s = make();
        s->source.set_last(nvp_test::ProviderId::Imsakiyem, nvp_test::plus_days(kDay0, 20));
        s->source.set_last(nvp_test::ProviderId::Emushaf, nvp_test::plus_days(kDay0, 20));
        settled(*s, 10, 0);
        s->now += 3600;
        s->planner.on_boot();
        const int before = s->refreshes;
        s->advance_minutes(1);
        TEST_ASSERT_EQUAL_INT(before + 1, s->refreshes);
    }
    // Last success more than 7 days ago.
    {
        auto s = make();
        settled(*s, 10, 0);
        s->now += 8 * 86400 + 3600;
        s->planner.on_boot();
        const int before = s->refreshes;
        s->advance_minutes(1);
        TEST_ASSERT_EQUAL_INT(before + 1, s->refreshes);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(Trigger::Boot), static_cast<int>(s->last_trigger));
    }
}

void test_no_refresh_without_clock_location_or_link() {
    auto s = make();
    s->clock_valid = false;
    s->start(kDay0, 10, 0);
    s->advance_minutes(10);
    TEST_ASSERT_EQUAL_INT(0, s->refreshes);
    s->clock_valid = true;
    s->has_location = false;
    s->advance_minutes(10);
    TEST_ASSERT_EQUAL_INT(0, s->refreshes);
    s->has_location = true;
    s->link(false);
    s->advance_minutes(10);
    TEST_ASSERT_EQUAL_INT(0, s->refreshes);
    s->link(true);  // the boot refresh still waits and now runs
    s->advance_minutes(1);
    TEST_ASSERT_EQUAL_INT(1, s->refreshes);
}

void test_setup_and_location_change_refresh_at_once() {
    auto s = make();
    settled(*s, 10, 0);
    s->planner.on_setup_done();
    s->advance_minutes(1);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Trigger::Setup), static_cast<int>(s->last_trigger));
    s->planner.on_location_changed();
    s->advance_minutes(1);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Trigger::LocationChanged), static_cast<int>(s->last_trigger));
    TEST_ASSERT_EQUAL_INT(3, s->refreshes);
}

// Exactly one refresh in the 8 days after a success at 03:00: the weekly one at 03:41 of day 7.
void test_weekly_once_at_night_minute() {
    auto s = make();
    settled(*s, 3, 0);
    s->advance_minutes(8 * 1440);
    s->print_log("weekly:");
    TEST_ASSERT_EQUAL_INT(2, s->refreshes);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Trigger::Weekly), static_cast<int>(s->last_trigger));
    TEST_ASSERT_EQUAL_STRING("2026-12-03 03:41 weekly: complete, 1 requests, 364 days ahead", s->log.back().c_str());
    TEST_ASSERT_TRUE(s->total_requests <= 2);
}

// Under 30 days ahead: once a night until fixed.
void test_low_coverage_nightly_until_fixed() {
    auto s = make();
    s->source.set_last(nvp_test::ProviderId::Imsakiyem, nvp_test::plus_days(kDay0, 20));
    s->source.set_last(nvp_test::ProviderId::Emushaf, nvp_test::plus_days(kDay0, 20));
    settled(*s, 10, 0);
    s->advance_minutes(3 * 1440);
    TEST_ASSERT_EQUAL_INT(1 + 3, s->refreshes);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Trigger::LowCoverage), static_cast<int>(s->last_trigger));
    // The provider publishes the next months: the next night fixes it, then no more.
    s->source.behaviour[static_cast<size_t>(nvp_test::ProviderId::Imsakiyem)].has_last = false;
    s->advance_minutes(3 * 1440);
    s->print_log("low coverage:");
    TEST_ASSERT_EQUAL_INT(1 + 3 + 1, s->refreshes);
}

void test_reconnect_only_when_needed_and_spaced() {
    // Full store: a reconnection does nothing.
    {
        auto s = make();
        settled(*s, 10, 0);
        s->link(false);
        s->advance_minutes(30);
        s->link(true);
        s->advance_minutes(5);
        TEST_ASSERT_EQUAL_INT(1, s->refreshes);
    }
    // Under 30 days: refresh on reconnection, at most once per 10 minutes.
    {
        auto s = make();
        s->source.set_last(nvp_test::ProviderId::Imsakiyem, nvp_test::plus_days(kDay0, 20));
        s->source.set_last(nvp_test::ProviderId::Emushaf, nvp_test::plus_days(kDay0, 20));
        settled(*s, 10, 0);
        for (int i = 0; i < 3; ++i) {  // three outages 3 minutes apart
            s->link(false);
            s->advance_minutes(1);
            s->link(true);
            s->advance_minutes(2);
        }
        TEST_ASSERT_EQUAL_INT(2, s->refreshes);  // boot + the first reconnection
        s->advance_minutes(10);
        s->link(false);
        s->advance_minutes(1);
        s->link(true);
        s->advance_minutes(1);
        TEST_ASSERT_EQUAL_INT(3, s->refreshes);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(Trigger::Reconnect), static_cast<int>(s->last_trigger));
    }
}

void test_manual_once_per_minute() {
    auto s = make();
    settled(*s, 10, 0);
    TEST_ASSERT_TRUE(s->planner.on_manual(s->now));
    s->advance_minutes(0);
    TEST_ASSERT_TRUE(s->step());
    TEST_ASSERT_FALSE(s->planner.on_manual(s->now + 30));
    TEST_ASSERT_TRUE(s->planner.on_manual(s->now + 60));
}

void test_night_minute_from_mac() {
    const uint8_t a[6] = {0x24, 0x58, 0x7C, 0xD1, 0x22, 0x10};
    const uint8_t b[6] = {0x24, 0x58, 0x7C, 0xD1, 0x22, 0x11};
    const uint16_t ma = nvp::providers::night_minute_of(a), mb = nvp::providers::night_minute_of(b);
    TEST_ASSERT_TRUE(ma >= 180 && ma < 300);
    TEST_ASSERT_TRUE(mb >= 180 && mb < 300);
    TEST_ASSERT_NOT_EQUAL(ma, mb);
}

void test_next_scheduled_is_in_the_night_window() {
    auto s = make();
    settled(*s, 10, 0);
    const int64_t next = s->planner.next_scheduled_utc(s->view());
    const int64_t local = next + Sim::kOffset;
    const int minute = static_cast<int>((local % 86400) / 60);
    TEST_ASSERT_EQUAL_INT(221, minute);
    TEST_ASSERT_TRUE(next - s->state.last_success_utc >= 7 * 86400);
    TEST_ASSERT_TRUE(next - s->state.last_success_utc < 8 * 86400);
}

}  // namespace

void run_trigger_tests() {
    RUN_TEST(test_boot_with_empty_store_refreshes);
    RUN_TEST(test_boot_with_fresh_full_store_does_not_refresh);
    RUN_TEST(test_boot_conditions);
    RUN_TEST(test_no_refresh_without_clock_location_or_link);
    RUN_TEST(test_setup_and_location_change_refresh_at_once);
    RUN_TEST(test_weekly_once_at_night_minute);
    RUN_TEST(test_low_coverage_nightly_until_fixed);
    RUN_TEST(test_reconnect_only_when_needed_and_spaced);
    RUN_TEST(test_manual_once_per_minute);
    RUN_TEST(test_night_minute_from_mac);
    RUN_TEST(test_next_scheduled_is_in_the_night_window);
}
