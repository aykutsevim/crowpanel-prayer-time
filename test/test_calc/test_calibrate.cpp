// Calibration, the computed Hijri date and the device's own days (spec 007 T026; FR-018, SC-004,
// research R3, R8).
#include <unity.h>

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "../support/fixture_file.h"
#include "nvp/calc/calibrate.h"
#include "nvp/calc/prayer_times.h"
#include "nvp/providers/imsakiyem.h"
#include "nvp/store/store.h"

using nvp::calc::CalibrationResult;
using nvp::engine::Date;
using nvp::engine::DayRecord;

namespace {

const nvp::calc::Coordinates kAnkara{39.92063, 32.85403};

struct Collect : nvp::providers::RecordSink {
    std::vector<DayRecord> records;
    void on_record(const DayRecord& r) override { records.push_back(r); }
};

std::vector<DayRecord> published() {
    Collect c;
    nvp::providers::ImsakiyemParser p(nvp::providers::ImsakiyemApi::V2, c);
    const std::string body = nvp_test::read_fixture("imsakiyem/v2_times_9206_2026-11-24_2027-03-31.json");
    p.feed(body.data(), body.size());
    p.finish();
    return c.records;
}

void test_needs_fourteen_days() {
    const std::vector<DayRecord> pub = published();
    CalibrationResult r = nvp::calc::calibrate(pub.data(), 13, kAnkara);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CalibrationResult::Status::TooFewDays), static_cast<int>(r.status));
    r = nvp::calc::calibrate(pub.data(), 14, kAnkara);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CalibrationResult::Status::Ok), static_cast<int>(r.status));
    TEST_ASSERT_EQUAL_UINT8(14, r.days);
}

void test_calculated_records_ignored() {
    std::vector<DayRecord> v = published();
    for (size_t i = 0; i < 20; ++i) v[i].source = nvp::engine::SourceId::Aladhan;
    const CalibrationResult r = nvp::calc::calibrate(v.data(), 30, kAnkara);
    TEST_ASSERT_EQUAL_UINT8(10, r.days);  // only the 10 rank-40 records
}

void test_offset_beyond_ten_rejected() {
    std::vector<DayRecord> v = published();
    for (DayRecord& r : v) r.minutes[5] = static_cast<uint16_t>(r.minutes[5] + 14);
    const CalibrationResult r = nvp::calc::calibrate(v.data(), v.size(), kAnkara);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CalibrationResult::Status::OffsetTooLarge), static_cast<int>(r.status));
    TEST_ASSERT_EQUAL_INT8(5, r.bad_prayer);  // yatsi
    TEST_ASSERT_TRUE(r.bad_offset >= 13);
}

void test_hijri_with_offset() {
    const Date d{2026, 11, 24};
    const nvp::engine::HijriDate t = nvp::calc::tabular_hijri(d);
    std::printf("tabular Hijri of 2026-11-24: %u/%u/%u (Diyanet 15/6/1448)\n", t.day, t.month, t.year);
    nvp::engine::HijriDate dy{15, 6, 1448, true};
    const int8_t off = nvp::calc::hijri_offset_for(d, dy);
    const nvp::engine::HijriDate h = nvp::calc::computed_hijri(d, off);
    TEST_ASSERT_EQUAL_UINT8(15, h.day);
    TEST_ASSERT_EQUAL_UINT8(6, h.month);
    TEST_ASSERT_EQUAL_UINT16(1448, h.year);
    // On the device calculated days follow the last published day, at most 8 days (fill_local).
    // For each published day, learn the offset from it and predict the 7 days after it. Month
    // lengths differ between Diyanet's table and the arithmetic calendar (Diyanet's Cemaziyelahir
    // 1448 has 30 days, the tabular 29), so a prediction across a month end can be a day off.
    const std::vector<DayRecord> pub = published();
    int right = 0, total = 0;
    for (size_t i = 0; i + 8 < pub.size(); ++i) {
        const int8_t o = nvp::calc::hijri_offset_for(pub[i].date, pub[i].hijri);
        for (size_t k = 1; k <= 7; ++k) {
            const DayRecord& r = pub[i + k];
            const nvp::engine::HijriDate c = nvp::calc::computed_hijri(r.date, o);
            ++total;
            if (c.day == r.hijri.day && c.month == r.hijri.month && c.year == r.hijri.year) ++right;
        }
    }
    std::printf("computed Hijri, the 7 days after a published day: %d of %d equal to Diyanet's\n", right, total);
    TEST_ASSERT_TRUE(right * 100 >= total * 85);
}

// Research R3 split: calibrate on the first 30 published days, test on the next 98.
void test_ankara_calibrated_median_within_a_minute() {
    const std::vector<DayRecord> pub = published();
    const CalibrationResult r = nvp::calc::calibrate(pub.data(), 30, kAnkara);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CalibrationResult::Status::Ok), static_cast<int>(r.status));
    nvp::store::Calibration cal;
    cal.valid = true;
    cal.district = 9206;
    for (int k = 0; k < 6; ++k) cal.offsets[k] = r.offsets[k];
    const char* names[6] = {"imsak", "gunes", "ogle", "ikindi", "aksam", "yatsi"};
    for (int k = 0; k < 6; ++k) {
        std::vector<int> before, after;
        for (size_t i = 30; i < pub.size(); ++i) {
            uint16_t raw[6];
            TEST_ASSERT_TRUE(nvp::calc::prayer_times_local(kAnkara, pub[i].date, nvp::calc::turkey(), 10800, raw));
            DayRecord loc;
            TEST_ASSERT_TRUE(nvp::calc::local_record(pub[i].date, kAnkara, &cal, 9206, loc));
            before.push_back(static_cast<int>(pub[i].minutes[k]) - raw[k]);
            after.push_back(static_cast<int>(pub[i].minutes[k]) - loc.minutes[k]);
        }
        std::sort(before.begin(), before.end());
        std::sort(after.begin(), after.end());
        const int mb = before[before.size() / 2], ma = after[after.size() / 2];
        std::printf("ankara %-6s offset %+d  before: median %+d (%+d..%+d)  after: median %+d (%+d..%+d)\n", names[k],
                    r.offsets[k], mb, before.front(), before.back(), ma, after.front(), after.back());
        TEST_ASSERT_TRUE(ma >= -1 && ma <= 1);
    }
}

void test_fill_local_never_replaces() {
    nvp::store::Store s;
    s.district_id = 9206;
    const Date today{2026, 11, 26};
    TEST_ASSERT_EQUAL_UINT32(8, nvp::calc::fill_local(s, today, kAnkara, nullptr));
    TEST_ASSERT_EQUAL_UINT16(8, s.count);
    const DayRecord* r = nvp::store::find(s, today);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::engine::SourceId::Local), static_cast<int>(r->source));
    TEST_ASSERT_TRUE(r->hijri.present);
    TEST_ASSERT_TRUE(nvp::engine::valid(*r));
    // A published today and tomorrow stay; only the other 6 days are added.
    nvp::store::Store t;
    t.district_id = 9206;
    const std::vector<DayRecord> pub = published();  // from 2026-11-24
    t.records[0] = pub[2];
    t.records[1] = pub[3];
    t.count = 2;
    TEST_ASSERT_EQUAL_UINT32(6, nvp::calc::fill_local(t, today, kAnkara, nullptr));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::engine::SourceId::Imsakiyem), static_cast<int>(nvp::store::find(t, today)->source));
    for (uint16_t i = 1; i < t.count; ++i)
        TEST_ASSERT_TRUE(nvp::engine::to_day_number(t.records[i - 1].date) < nvp::engine::to_day_number(t.records[i].date));
}

}  // namespace

void run_calibrate_tests() {
    RUN_TEST(test_needs_fourteen_days);
    RUN_TEST(test_calculated_records_ignored);
    RUN_TEST(test_offset_beyond_ten_rejected);
    RUN_TEST(test_hijri_with_offset);
    RUN_TEST(test_ankara_calibrated_median_within_a_minute);
    RUN_TEST(test_fill_local_never_replaces);
}
