// Every second of a day and past the next midnight (SC-003), purity (FR-019) and no allocation
// (FR-023, SC-007, research R7).
#include <unity.h>

#include <cstddef>
#include <cstring>

#include "helpers.h"

using namespace engine_test;

extern std::size_t g_allocations;  // counted by test_main.cpp

namespace {

bool same(const MainView& a, const MainView& b) {
    if (a.has_data != b.has_data || a.approx_bounds != b.approx_bounds || a.period != b.period ||
        a.start != b.start || a.end != b.end || a.next != b.next || a.next_is_tomorrow != b.next_is_tomorrow ||
        a.remaining_s != b.remaining_s || a.elapsed_s != b.elapsed_s || a.fraction != b.fraction ||
        a.soon != b.soon || a.entered != b.entered || a.now_second_of_day != b.now_second_of_day ||
        a.period_name != b.period_name || a.countdown_label != b.countdown_label) {
        return false;
    }
    const char* const texts[][2] = {{a.clock_hhmm, b.clock_hhmm},   {a.clock_ss, b.clock_ss},
                                    {a.date_text, b.date_text},     {a.hijri_text, b.hijri_text},
                                    {a.start_text, b.start_text},   {a.end_text, b.end_text},
                                    {a.countdown_text, b.countdown_text}};
    for (const auto& t : texts) {
        if (std::strcmp(t[0], t[1]) != 0) return false;
    }
    for (int k = 0; k < kVakitCount; ++k) {
        const Column& x = a.columns[k];
        const Column& y = b.columns[k];
        if (x.state != y.state || x.minutes != y.minutes || x.seconds_until != y.seconds_until ||
            std::strcmp(x.time_text, y.time_text) != 0 || std::strcmp(x.note, y.note) != 0 || x.name != y.name) {
            return false;
        }
    }
    for (int i = 0; i < 3; ++i) {
        if (a.kerahat[i].shown != b.kerahat[i].shown || a.kerahat[i].start_s != b.kerahat[i].start_s ||
            a.kerahat[i].end_s != b.kerahat[i].end_s) {
            return false;
        }
    }
    return true;
}

bool is_vakit_second(const DayRecord& r, int32_t second) {
    if (second == 0) return true;  // midnight
    for (int k = 0; k < kVakitCount; ++k) {
        if (r.minutes[k] * 60 == second) return true;
    }
    return false;
}

const DayRecord kDayAfter{Date{2026, 11, 19}, {378, 468, 774, 925, 1069, 1152}, HijriDate{9, 6, 1448, true},
                          SourceId::Imsakiyem};

// One sweep: `seconds` seconds of `today`, checking the invariants against the previous second.
void sweep(const DayRecord* yesterday, const DayRecord* today, const DayRecord* tomorrow, int32_t seconds,
           MainView& prev, bool& have_prev) {
    MainView v;
    MainView again;
    for (int32_t s = 0; s < seconds; ++s) {
        Input in;
        in.now = Moment{today->date, s};
        in.yesterday = yesterday;
        in.today = today;
        in.tomorrow = tomorrow;
        evaluate(in, v);
        evaluate(in, again);
        if (!same(v, again)) TEST_FAIL_MESSAGE("two evaluations of the same input differ (FR-019)");

        const LocalSeconds now = static_cast<LocalSeconds>(to_day_number(today->date)) * kSecondsPerDay + s;
        if (!(v.start <= now && now < v.end)) TEST_FAIL_MESSAGE("start <= now < end violated");
        if (v.remaining_s + v.elapsed_s != v.end - v.start) TEST_FAIL_MESSAGE("remaining + elapsed != length");
        int next = 0;
        int current = 0;
        for (const Column& c : v.columns) {
            if (c.state == ColumnState::Next) ++next;
            if (c.state == ColumnState::Current) ++current;
        }
        if (next != 1 || current > 1) TEST_FAIL_MESSAGE("not exactly one Next and at most one Current");

        if (have_prev) {
            bool changed = prev.period != v.period || prev.next != v.next;
            for (int k = 0; k < kVakitCount; ++k) changed = changed || prev.columns[k].state != v.columns[k].state;
            if (changed && !is_vakit_second(*today, s)) TEST_FAIL_MESSAGE("state changed between vakits");
            if (prev.start == v.start && prev.remaining_s - 1 != v.remaining_s) {
                TEST_FAIL_MESSAGE("countdown did not drop by one second");
            }
        }
        prev = v;
        have_prev = true;
    }
}

}  // namespace

static void test_full_day_and_next_morning() {  // SC-003, SC-007
    MainView prev;
    bool have_prev = false;
    const std::size_t before = g_allocations;
    sweep(&fixture::kYesterday, &fixture::kToday, &fixture::kTomorrow, kSecondsPerDay, prev, have_prev);
    sweep(&fixture::kToday, &fixture::kTomorrow, &kDayAfter, 7 * 3600, prev, have_prev);
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(before), static_cast<uint32_t>(g_allocations));
}

static void test_allocation_counter_works() {  // guards the guard
    const std::size_t before = g_allocations;
    int* p = new int(1);
    delete p;
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(before + 1), static_cast<uint32_t>(g_allocations));
}

void run_sweep_tests() {
    RUN_TEST(test_allocation_counter_works);
    RUN_TEST(test_full_day_and_next_morning);
}
