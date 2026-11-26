// Merge rules of 04-data-sources §4 (spec 006 T007, research R6, FR-008).
#include <unity.h>

#include <cstdio>
#include <vector>

#include "nvp/engine/calendar.h"
#include "nvp/store/merge.h"

using nvp::engine::Date;
using nvp::engine::DayNumber;
using nvp::engine::DayRecord;
using nvp::engine::SourceId;
using nvp::store::MergeStats;
using nvp::store::Store;

namespace {

const Date kToday{2026, 11, 24};
const int64_t kT0 = 1795524049;

DayRecord day(Date d, SourceId src, uint16_t shift = 0) {
    DayRecord r;
    r.date = d;
    const uint16_t base[6] = {368, 456, 760, 912, 1054, 1137};
    for (int i = 0; i < 6; ++i) r.minutes[i] = static_cast<uint16_t>(base[i] + shift);
    r.hijri = {15, 6, 1448, true};
    r.source = src;
    return r;
}

Date plus(Date d, int days) { return nvp::engine::to_date(nvp::engine::to_day_number(d) + days); }

Store g_store;

void reset(int days_from_today, int count, SourceId src, int64_t refresh = kT0) {
    g_store = Store{};
    g_store.district_id = 9206;
    g_store.last_refresh_utc = refresh;
    for (int i = 0; i < count; ++i) g_store.records[i] = day(plus(kToday, days_from_today + i), src);
    g_store.count = static_cast<uint16_t>(count);
}

bool sorted_unique() {
    for (uint16_t i = 1; i < g_store.count; ++i)
        if (nvp::engine::to_day_number(g_store.records[i - 1].date) >= nvp::engine::to_day_number(g_store.records[i].date))
            return false;
    return true;
}

void test_into_empty_store() {
    reset(0, 0, SourceId::Imsakiyem);
    std::vector<DayRecord> in;
    for (int i = 4; i >= 0; --i) in.push_back(day(plus(kToday, i), SourceId::Imsakiyem));  // unsorted
    MergeStats m = nvp::store::merge(g_store, in.data(), in.size(), kT0 + 10, kToday);
    TEST_ASSERT_EQUAL_UINT16(5, m.added);
    TEST_ASSERT_EQUAL_UINT16(5, g_store.count);
    TEST_ASSERT_TRUE(sorted_unique());
    TEST_ASSERT_EQUAL_INT64(kT0 + 10, g_store.last_refresh_utc);
}

void test_higher_rank_replaces() {
    reset(0, 3, SourceId::Aladhan);  // rank 20
    DayRecord in = day(plus(kToday, 1), SourceId::Imsakiyem, 2);
    MergeStats m = nvp::store::merge(g_store, &in, 1, kT0 - 100, kToday);  // even from an older fetch
    TEST_ASSERT_EQUAL_UINT16(1, m.replaced);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SourceId::Imsakiyem), static_cast<int>(g_store.records[1].source));
    TEST_ASSERT_EQUAL_UINT16(in.minutes[0], g_store.records[1].minutes[0]);
}

void test_equal_rank_later_fetch_replaces() {
    reset(0, 3, SourceId::Imsakiyem);
    DayRecord in = day(plus(kToday, 2), SourceId::Imsakiyem, 1);
    MergeStats m = nvp::store::merge(g_store, &in, 1, kT0 + 3600, kToday);
    TEST_ASSERT_EQUAL_UINT16(1, m.replaced);
    TEST_ASSERT_EQUAL_UINT16(in.minutes[3], g_store.records[2].minutes[3]);
    TEST_ASSERT_EQUAL_UINT16(0, m.mismatches);  // same source: an update, not a mismatch
}

void test_equal_rank_older_fetch_kept() {
    reset(0, 3, SourceId::Imsakiyem);
    DayRecord in = day(plus(kToday, 2), SourceId::Imsakiyem, 1);
    uint16_t before = g_store.records[2].minutes[3];
    MergeStats m = nvp::store::merge(g_store, &in, 1, kT0 - 3600, kToday);
    TEST_ASSERT_EQUAL_UINT16(0, m.replaced);
    TEST_ASSERT_EQUAL_UINT16(1, m.kept);
    TEST_ASSERT_EQUAL_UINT16(before, g_store.records[2].minutes[3]);
    TEST_ASSERT_EQUAL_INT64(kT0, g_store.last_refresh_utc);  // the store keeps its newer time
}

void test_lower_rank_never_replaces() {
    reset(0, 3, SourceId::Imsakiyem);
    DayRecord in = day(plus(kToday, 0), SourceId::Local, 9);  // rank 10
    MergeStats m = nvp::store::merge(g_store, &in, 1, kT0 + 9999, kToday);
    TEST_ASSERT_EQUAL_UINT16(0, m.replaced);
    TEST_ASSERT_EQUAL_UINT16(1, m.kept);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SourceId::Imsakiyem), static_cast<int>(g_store.records[0].source));
}

void test_rank40_mismatch_counted_once() {
    reset(0, 3, SourceId::Imsakiyem);
    std::vector<DayRecord> in = {day(plus(kToday, 1), SourceId::Emushaf, 1), day(plus(kToday, 2), SourceId::Emushaf, 0),
                                 day(plus(kToday, 0), SourceId::Emushaf, 2)};
    MergeStats m = nvp::store::merge(g_store, in.data(), in.size(), kT0 + 60, kToday);
    TEST_ASSERT_EQUAL_UINT16(2, m.mismatches);  // day 1 and day 0 differ; day 2 agrees
    TEST_ASSERT_TRUE(m.has_mismatch);
    // The first mismatch reports both time sets.
    TEST_ASSERT_TRUE(m.mismatch_stored.date == plus(kToday, 1));
    TEST_ASSERT_EQUAL_UINT16(day(kToday, SourceId::Imsakiyem).minutes[0], m.mismatch_stored.minutes[0]);
    TEST_ASSERT_EQUAL_UINT16(day(kToday, SourceId::Imsakiyem, 1).minutes[0], m.mismatch_incoming.minutes[0]);
}

void test_prune_before_yesterday() {
    reset(-5, 10, SourceId::Imsakiyem);  // today-5 .. today+4
    DayRecord in[2] = {day(plus(kToday, -3), SourceId::Imsakiyem), day(plus(kToday, 5), SourceId::Imsakiyem)};
    MergeStats m = nvp::store::merge(g_store, in, 2, kT0 + 1, kToday);
    // Stored today-5..today-2 (4) and the incoming today-3 (1) are pruned; yesterday is kept.
    TEST_ASSERT_EQUAL_UINT16(5, m.pruned);
    TEST_ASSERT_EQUAL_UINT16(1, m.added);
    TEST_ASSERT_TRUE(g_store.records[0].date == plus(kToday, -1));
    TEST_ASSERT_TRUE(g_store.records[g_store.count - 1].date == plus(kToday, 5));
    TEST_ASSERT_EQUAL_UINT16(7, g_store.count);
    TEST_ASSERT_TRUE(sorted_unique());
}

void test_at_most_400_oldest_dropped() {
    reset(-1, 400, SourceId::Imsakiyem);  // yesterday .. today+398, full
    std::vector<DayRecord> in;
    for (int i = 399; i < 404; ++i) in.push_back(day(plus(kToday, i), SourceId::Imsakiyem));
    MergeStats m = nvp::store::merge(g_store, in.data(), in.size(), kT0 + 1, kToday);
    TEST_ASSERT_EQUAL_UINT16(400, g_store.count);
    TEST_ASSERT_EQUAL_UINT16(5, m.added);
    TEST_ASSERT_EQUAL_UINT16(5, m.dropped);
    TEST_ASSERT_TRUE(g_store.records[0].date == plus(kToday, 4));
    TEST_ASSERT_TRUE(g_store.records[399].date == plus(kToday, 403));
    TEST_ASSERT_TRUE(sorted_unique());
}

}  // namespace

// Spec 007 T003 (FR-002, SC-006): every pair of the ranks 10, 20, 40. A lower rank never replaces a
// higher one, whatever the fetch time; a higher rank always replaces; equal ranks follow spec 006.
void test_rank_pairs_spec007() {
    const SourceId srcs[3] = {SourceId::Local, SourceId::Aladhan, SourceId::Imsakiyem};
    for (int a = 0; a < 3; ++a) {
        for (int b = 0; b < 3; ++b) {
            for (int later = 0; later < 2; ++later) {
                reset(0, 3, srcs[a]);
                DayRecord in = day(plus(kToday, 1), srcs[b], 5);
                const int64_t fetch = later ? kT0 + 3600 : kT0 - 3600;
                nvp::store::merge(g_store, &in, 1, fetch, kToday);
                const uint8_t ra = nvp::engine::rank(srcs[a]), rb = nvp::engine::rank(srcs[b]);
                const bool replaced = g_store.records[1].source == srcs[b] && g_store.records[1].minutes[0] == 368 + 5;
                char msg[64];
                std::snprintf(msg, sizeof msg, "stored rank %u, incoming %u, later %d", ra, rb, later);
                if (rb < ra) TEST_ASSERT_FALSE_MESSAGE(replaced, msg);
                if (rb > ra) TEST_ASSERT_TRUE_MESSAGE(replaced, msg);
                if (rb == ra) TEST_ASSERT_EQUAL_MESSAGE(later != 0, replaced, msg);
            }
        }
    }
}

// A stored rank-40 day and a much newer rank-10 one: the stored day stays (DATA-05).
void test_newer_local_never_replaces_diyanet() {
    reset(0, 2, SourceId::Emushaf);
    DayRecord in = day(kToday, SourceId::Local, 9);
    MergeStats m = nvp::store::merge(g_store, &in, 1, kT0 + 365 * 86400, kToday);
    TEST_ASSERT_EQUAL_UINT16(0, m.replaced);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SourceId::Emushaf), static_cast<int>(g_store.records[0].source));
    TEST_ASSERT_EQUAL_UINT16(368, g_store.records[0].minutes[0]);
}

void run_merge_tests() {
    RUN_TEST(test_rank_pairs_spec007);
    RUN_TEST(test_newer_local_never_replaces_diyanet);
    RUN_TEST(test_into_empty_store);
    RUN_TEST(test_higher_rank_replaces);
    RUN_TEST(test_equal_rank_later_fetch_replaces);
    RUN_TEST(test_equal_rank_older_fetch_kept);
    RUN_TEST(test_lower_rank_never_replaces);
    RUN_TEST(test_rank40_mismatch_counted_once);
    RUN_TEST(test_prune_before_yesterday);
    RUN_TEST(test_at_most_400_oldest_dropped);
}
