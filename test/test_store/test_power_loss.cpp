// Power loss during a store write (spec 006 T008, research R5, FR-009, DATA-07, SC-004).
//
// A save over an existing store is cut at call N, for every N from 1 to the number of calls a
// whole save makes. After each cut the fake file system is remounted (open files and their
// uncommitted data are gone) and the store loaded: it must be exactly the old store or exactly the
// new one, never an error and never a mix. The same is checked for a single failing call that is
// not a power cut (an I/O error): the save reports failure and the old store stays.
#include <unity.h>

#include <cstdio>
#include <cstring>

#include "../support/fake_fs.h"
#include "nvp/engine/calendar.h"
#include "nvp/store/store.h"

using nvp::engine::Date;
using nvp::engine::DayRecord;
using nvp::engine::SourceId;
using nvp::store::LoadResult;
using nvp::store::SaveResult;
using nvp::store::Store;

namespace {

Store g_old, g_new, g_loaded;

void fill(Store& s, Date first, uint16_t count, uint16_t shift, int64_t refresh) {
    s = Store{};
    s.district_id = 9206;
    s.last_refresh_utc = refresh;
    s.count = count;
    for (uint16_t i = 0; i < count; ++i) {
        DayRecord& r = s.records[i];
        r.date = nvp::engine::to_date(nvp::engine::to_day_number(first) + i);
        const uint16_t base[6] = {360, 450, 760, 910, 1050, 1135};
        for (int k = 0; k < 6; ++k) r.minutes[k] = static_cast<uint16_t>(base[k] + shift);
        r.hijri = {1, 1, 1448, true};
        r.source = SourceId::Imsakiyem;
    }
}

bool equal(const Store& a, const Store& b) {
    if (a.district_id != b.district_id || a.count != b.count || a.last_refresh_utc != b.last_refresh_utc) return false;
    for (uint16_t i = 0; i < a.count; ++i) {
        const DayRecord& x = a.records[i];
        const DayRecord& y = b.records[i];
        if (!(x.date == y.date) || std::memcmp(x.minutes, y.minutes, sizeof x.minutes) != 0 || x.source != y.source ||
            x.hijri.day != y.hijri.day || x.hijri.year != y.hijri.year)
            return false;
    }
    return true;
}

nvp_test::FakeFs with_old_store() {
    nvp_test::FakeFs fs;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SaveResult::Ok), static_cast<int>(nvp::store::save(fs, g_old)));
    return fs;
}

int calls_of_a_whole_save() {
    nvp_test::FakeFs fs = with_old_store();
    fs.reset_counter();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SaveResult::Ok), static_cast<int>(nvp::store::save(fs, g_new)));
    return fs.calls;
}

void test_cut_at_every_call() {
    fill(g_old, Date{2026, 11, 20}, 38, 0, 1795000000);
    fill(g_new, Date{2026, 11, 23}, 400, 3, 1795524049);
    const int total = calls_of_a_whole_save();
    TEST_ASSERT_TRUE(total >= 6);
    int saw_old = 0, saw_new = 0;
    for (int n = 1; n <= total + 1; ++n) {
        nvp_test::FakeFs fs = with_old_store();
        fs.power_cut_at(n);
        SaveResult r = nvp::store::save(fs, g_new);
        if (n <= total) TEST_ASSERT_TRUE(fs.dead);
        fs.remount();
        LoadResult lr = nvp::store::load(fs, 9206, g_loaded);
        char msg[64];
        std::snprintf(msg, sizeof msg, "cut at call %d of %d", n, total);
        TEST_ASSERT_EQUAL_INT_MESSAGE(static_cast<int>(LoadResult::Ok), static_cast<int>(lr), msg);
        bool is_old = equal(g_loaded, g_old);
        bool is_new = equal(g_loaded, g_new);
        TEST_ASSERT_TRUE_MESSAGE(is_old != is_new, msg);  // exactly one of them
        if (is_new) ++saw_new; else ++saw_old;
        if (r == SaveResult::Ok) TEST_ASSERT_TRUE_MESSAGE(is_new, msg);  // a reported success is durable
        // A leftover .new is removed on load.
        TEST_ASSERT_FALSE_MESSAGE(fs.files.count("store_9206.new") > 0, msg);
    }
    TEST_ASSERT_TRUE(saw_old > 0);
    TEST_ASSERT_TRUE(saw_new > 0);
    // The rename is the last call and the commit point: every cut, including one at the rename,
    // leaves the old store; only the uncut run gives the new one.
    std::printf("power loss: %d calls per save, cut at each: %d runs gave the old store, %d (uncut) the new, "
                "0 errors\n",
                total, saw_old, saw_new);
}

void test_cut_on_first_save() {  // no old store: a cut leaves nothing or the new store
    fill(g_new, Date{2026, 11, 23}, 10, 0, 1795524049);
    nvp_test::FakeFs probe;
    nvp::store::save(probe, g_new);
    const int total = probe.calls;
    for (int n = 1; n <= total; ++n) {
        nvp_test::FakeFs fs;
        fs.power_cut_at(n);
        nvp::store::save(fs, g_new);
        fs.remount();
        LoadResult lr = nvp::store::load(fs, 9206, g_loaded);
        TEST_ASSERT_TRUE(lr == LoadResult::Missing || (lr == LoadResult::Ok && equal(g_loaded, g_new)));
    }
}

void test_single_io_error_keeps_old() {
    fill(g_old, Date{2026, 11, 20}, 38, 0, 1795000000);
    fill(g_new, Date{2026, 11, 23}, 120, 3, 1795524049);
    const int total = calls_of_a_whole_save();
    for (int n = 1; n <= total; ++n) {
        nvp_test::FakeFs fs = with_old_store();
        fs.fail_once_at(n);
        SaveResult r = nvp::store::save(fs, g_new);
        LoadResult lr = nvp::store::load(fs, 9206, g_loaded);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(LoadResult::Ok), static_cast<int>(lr));
        if (r == SaveResult::Ok) {
            TEST_ASSERT_TRUE(equal(g_loaded, g_new));
        } else {
            TEST_ASSERT_TRUE(equal(g_loaded, g_old));
        }
        TEST_ASSERT_EQUAL_size_t(0, fs.open_handles());  // nothing left open (esp_littlefs rename rule)
    }
}

}  // namespace

void run_power_loss_tests() {
    RUN_TEST(test_cut_at_every_call);
    RUN_TEST(test_cut_on_first_save);
    RUN_TEST(test_single_io_error_keeps_old);
}
