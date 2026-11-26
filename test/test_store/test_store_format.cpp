// Store file format (spec 006 T006, research R5, FR-008, FR-009): round trip, damage detection,
// lookups.
#include <unity.h>

#include <cstring>
#include <string>

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

DayRecord day(nvp::engine::DayNumber n, uint16_t shift = 0) {
    DayRecord r;
    r.date = nvp::engine::to_date(n);
    const uint16_t base[6] = {360, 450, 760, 910, 1050, 1135};
    for (int i = 0; i < 6; ++i) r.minutes[i] = static_cast<uint16_t>(base[i] + shift + (n % 7));
    r.hijri = {static_cast<uint8_t>(1 + n % 29), static_cast<uint8_t>(1 + n % 12), 1448, true};
    r.source = SourceId::Imsakiyem;
    return r;
}

Store* filled(uint16_t count, Date first) {
    static Store s;
    s = Store{};
    s.district_id = 9206;
    s.last_refresh_utc = 1795524049;
    s.count = count;
    nvp::engine::DayNumber n0 = nvp::engine::to_day_number(first);
    for (uint16_t i = 0; i < count; ++i) s.records[i] = day(n0 + i);
    return &s;
}

bool same(const DayRecord& a, const DayRecord& b) {
    return a.date == b.date && std::memcmp(a.minutes, b.minutes, sizeof a.minutes) == 0 &&
           a.hijri.present == b.hijri.present && a.hijri.day == b.hijri.day && a.hijri.month == b.hijri.month &&
           a.hijri.year == b.hijri.year && a.source == b.source;
}

Store g_loaded;

void test_round_trip_400() {
    nvp_test::FakeFs fs;
    Store* s = filled(400, Date{2026, 11, 23});
    s->records[5].hijri.present = false;
    s->records[5].hijri = {};
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SaveResult::Ok), static_cast<int>(nvp::store::save(fs, *s)));
    TEST_ASSERT_EQUAL_size_t(1, fs.files.size());  // no .new left
    TEST_ASSERT_EQUAL_size_t(32 + 400 * 20, fs.files["store_9206.bin"].size());
    TEST_ASSERT_EQUAL_size_t(0, fs.open_handles());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(LoadResult::Ok), static_cast<int>(nvp::store::load(fs, 9206, g_loaded)));
    TEST_ASSERT_EQUAL_UINT32(9206, g_loaded.district_id);
    TEST_ASSERT_EQUAL_INT64(1795524049, g_loaded.last_refresh_utc);
    TEST_ASSERT_EQUAL_UINT16(400, g_loaded.count);
    for (int i = 0; i < 400; ++i) TEST_ASSERT_TRUE(same(s->records[i], g_loaded.records[i]));
    TEST_ASSERT_FALSE(g_loaded.records[5].hijri.present);
    TEST_ASSERT_EQUAL_size_t(0, fs.open_handles());
}

void test_header_layout() {
    nvp_test::FakeFs fs;
    Store* s = filled(2, Date{2026, 11, 24});
    nvp::store::save(fs, *s);
    const std::string& b = fs.files["store_9206.bin"];
    // 0 magic | 4 u16 version | 6 u16 count | 8 u32 district | 12 u32 crc | 16 i64 refresh
    // | 24 i32 first day | 28 i32 last day; the host is little-endian, like the device.
    TEST_ASSERT_EQUAL_size_t(32 + 2 * 20, b.size());
    TEST_ASSERT_EQUAL_MEMORY("NVPS", b.data(), 4);
    uint16_t version = 0, count = 0;
    std::memcpy(&version, b.data() + 4, 2);
    std::memcpy(&count, b.data() + 6, 2);
    TEST_ASSERT_EQUAL_UINT16(1, version);
    TEST_ASSERT_EQUAL_UINT16(2, count);
    uint32_t district = 0, crc = 0;
    std::memcpy(&district, b.data() + 8, 4);
    std::memcpy(&crc, b.data() + 12, 4);
    TEST_ASSERT_EQUAL_UINT32(9206, district);
    TEST_ASSERT_EQUAL_HEX32(nvp::store::crc32(0, b.data() + 32, 40), crc);
    int64_t refresh = 0;
    std::memcpy(&refresh, b.data() + 16, 8);
    TEST_ASSERT_EQUAL_INT64(1795524049, refresh);
    int32_t first = 0, last = 0;
    std::memcpy(&first, b.data() + 24, 4);
    std::memcpy(&last, b.data() + 28, 4);
    TEST_ASSERT_EQUAL_INT32(nvp::engine::to_day_number(Date{2026, 11, 24}), first);
    TEST_ASSERT_EQUAL_INT32(nvp::engine::to_day_number(Date{2026, 11, 25}), last);
    // First record: days since 2020-01-01, then the six times.
    uint16_t d0 = 0, imsak = 0;
    std::memcpy(&d0, b.data() + 32, 2);
    std::memcpy(&imsak, b.data() + 34, 2);
    TEST_ASSERT_EQUAL_INT(nvp::engine::to_day_number(Date{2026, 11, 24}) - nvp::engine::to_day_number(Date{2020, 1, 1}),
                          d0);
    TEST_ASSERT_EQUAL_UINT16(s->records[0].minutes[0], imsak);
}

void test_crc_known_value() {  // IEEE CRC-32 check value
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, nvp::store::crc32(0, "123456789", 9));
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, nvp::store::crc32(nvp::store::crc32(0, "1234", 4), "56789", 5));
}

void save_two(nvp_test::FakeFs& fs) {
    Store* s = filled(10, Date{2026, 11, 24});
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SaveResult::Ok), static_cast<int>(nvp::store::save(fs, *s)));
}

void expect_load(nvp_test::FakeFs& fs, LoadResult want) {
    LoadResult got = nvp::store::load(fs, 9206, g_loaded);
    TEST_ASSERT_EQUAL_STRING(nvp::store::load_result_text(want), nvp::store::load_result_text(got));
    if (want != LoadResult::Ok) TEST_ASSERT_EQUAL_UINT16(0, g_loaded.count);  // never returned as data
}

void test_damage_detected() {  // FR-009
    {
        nvp_test::FakeFs fs;
        save_two(fs);
        fs.files["store_9206.bin"][32 + 45] ^= 0x01;  // a flipped bit in a record
        expect_load(fs, LoadResult::BadCrc);
    }
    {
        nvp_test::FakeFs fs;
        save_two(fs);
        fs.files["store_9206.bin"][26] ^= 0x01;  // the header's first day, outside the CRC
        expect_load(fs, LoadResult::BadCrc);
    }
    {
        nvp_test::FakeFs fs;
        save_two(fs);
        fs.files["store_9206.bin"][0] = 'X';
        expect_load(fs, LoadResult::BadMagic);
    }
    {
        nvp_test::FakeFs fs;
        save_two(fs);
        fs.files["store_9206.bin"][4] = 2;  // version 2
        expect_load(fs, LoadResult::BadVersion);
    }
    {
        nvp_test::FakeFs fs;
        save_two(fs);
        fs.files["store_9206.bin"].resize(32 + 20 * 9 + 7);  // a short file
        expect_load(fs, LoadResult::Truncated);
        fs.files["store_9206.bin"].resize(20);  // shorter than the header
        expect_load(fs, LoadResult::Truncated);
    }
    {
        nvp_test::FakeFs fs;
        expect_load(fs, LoadResult::Missing);
    }
    {
        nvp_test::FakeFs fs;
        save_two(fs);
        fs.files["store_9541.bin"] = fs.files["store_9206.bin"];  // a copy under another name
        LoadResult r = nvp::store::load(fs, 9541, g_loaded);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(LoadResult::WrongDistrict), static_cast<int>(r));
    }
}

void test_find_and_covers() {
    Store* s = filled(38, Date{2026, 11, 24});  // 2026-11-24 .. 2026-12-31
    TEST_ASSERT_NULL(nvp::store::find(*s, Date{2026, 11, 23}));
    TEST_ASSERT_NOT_NULL(nvp::store::find(*s, Date{2026, 11, 24}));
    TEST_ASSERT_TRUE(nvp::store::find(*s, Date{2026, 12, 31})->date == (Date{2026, 12, 31}));
    TEST_ASSERT_NULL(nvp::store::find(*s, Date{2027, 1, 1}));
    TEST_ASSERT_TRUE(nvp::store::covers(*s, Date{2026, 12, 1}));
    TEST_ASSERT_EQUAL_UINT16(37, nvp::store::days_after(*s, Date{2026, 11, 24}));
    TEST_ASSERT_EQUAL_UINT16(0, nvp::store::days_after(*s, Date{2026, 12, 31}));
    Store empty;
    TEST_ASSERT_NULL(nvp::store::find(empty, Date{2026, 11, 24}));
}

void test_store_ending_today() {  // spec edge case, analysis C2
    Store* s = filled(3, Date{2026, 11, 22});  // 22, 23, 24
    TEST_ASSERT_TRUE(nvp::store::covers(*s, Date{2026, 11, 24}));
    TEST_ASSERT_NULL(nvp::store::find(*s, Date{2026, 11, 25}));
    TEST_ASSERT_EQUAL_UINT16(0, nvp::store::days_after(*s, Date{2026, 11, 24}));
}

// Spec 007 T035: the days of cache_ending are consecutive; a gap ends them.
void test_consecutive_days_after() {
    Store* s = filled(10, Date{2026, 11, 22});  // 22 .. 01.12
    TEST_ASSERT_EQUAL_UINT16(7, nvp::store::consecutive_days_after(*s, Date{2026, 11, 24}));
    TEST_ASSERT_EQUAL_UINT16(0, nvp::store::consecutive_days_after(*s, Date{2026, 12, 1}));
    // Remove 27.11: only 25 and 26 count.
    for (uint16_t i = 5; i + 1 < s->count; ++i) s->records[i] = s->records[i + 1];
    --s->count;
    TEST_ASSERT_EQUAL_UINT16(2, nvp::store::consecutive_days_after(*s, Date{2026, 11, 24}));
    TEST_ASSERT_EQUAL_UINT16(6, nvp::store::days_after(*s, Date{2026, 11, 24}));
}

void test_file_names() {
    char buf[32];
    TEST_ASSERT_TRUE(nvp::store::store_file_name(9206, false, buf, sizeof buf));
    TEST_ASSERT_EQUAL_STRING("store_9206.bin", buf);
    TEST_ASSERT_TRUE(nvp::store::store_file_name(100059, true, buf, sizeof buf));
    TEST_ASSERT_EQUAL_STRING("store_100059.new", buf);
    TEST_ASSERT_FALSE(nvp::store::store_file_name(9206, false, buf, 8));
}

void test_empty_store_not_saved() {
    nvp_test::FakeFs fs;
    Store s;
    s.district_id = 9206;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SaveResult::Empty), static_cast<int>(nvp::store::save(fs, s)));
    TEST_ASSERT_EQUAL_size_t(0, fs.files.size());
}

}  // namespace

void run_store_format_tests() {
    RUN_TEST(test_consecutive_days_after);
    RUN_TEST(test_round_trip_400);
    RUN_TEST(test_header_layout);
    RUN_TEST(test_crc_known_value);
    RUN_TEST(test_damage_detected);
    RUN_TEST(test_find_and_covers);
    RUN_TEST(test_store_ending_today);
    RUN_TEST(test_file_names);
    RUN_TEST(test_empty_store_not_saved);
}
