// Aladhan calendar parser on the recorded answers (spec 007 T019, research R2, FR-009).
#include <unity.h>

#include <cstdlib>
#include <string>
#include <vector>

#include "../support/fixture_file.h"
#include "nvp/engine/calendar.h"
#include "nvp/providers/aladhan.h"
#include "nvp/providers/imsakiyem.h"
#include "nvp/providers/validate.h"

using nvp::engine::Date;
using nvp::engine::DayRecord;
using nvp::providers::AladhanParser;
using nvp::providers::BatchInfo;
using nvp::providers::Reason;
using nvp::providers::RecordSink;

namespace {

const char* kNov = "aladhan/calendar_2026_11_9206.json";
const char* kDec = "aladhan/calendar_2026_12_9206.json";
const char* k503 = "aladhan/timings_by_address_503.json";
const Date kToday{2026, 11, 24};
const Date kNone{0, 0, 0};

struct Collect : RecordSink {
    std::vector<DayRecord> records;
    void on_record(const DayRecord& r) override { records.push_back(r); }
};

struct Parsed {
    std::vector<DayRecord> records;
    BatchInfo info;
};

Parsed parse(const std::string& body, Date from, Date to, size_t chunk = 4096) {
    Collect c;
    AladhanParser p(c, from, to);
    for (size_t at = 0; at < body.size(); at += chunk) {
        const size_t n = body.size() - at < chunk ? body.size() - at : chunk;
        if (!p.feed(body.data() + at, n)) break;
    }
    p.finish();
    return Parsed{c.records, p.info()};
}

void test_whole_months() {
    const Parsed nov = parse(nvp_test::read_fixture(kNov), kNone, kNone);
    const Parsed dec = parse(nvp_test::read_fixture(kDec), kNone, kNone);
    TEST_ASSERT_EQUAL_UINT32(30, nov.records.size());
    TEST_ASSERT_EQUAL_UINT32(31, dec.records.size());
    TEST_ASSERT_TRUE(nov.info.success);
}

// 24.11.2026: 06:08 07:36 12:40 15:13 17:34 18:56; Fajr, not Imsak (05:58); no Hijri date.
void test_record_mapping() {
    const Parsed p = parse(nvp_test::read_fixture(kNov), kNone, kNone);
    const DayRecord& r = p.records[23];
    TEST_ASSERT_TRUE(r.date == kToday);
    const uint16_t want[6] = {368, 456, 760, 913, 1054, 1136};
    TEST_ASSERT_EQUAL_UINT16_ARRAY(want, r.minutes, 6);
    TEST_ASSERT_FALSE(r.hijri.present);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::engine::SourceId::Aladhan), static_cast<int>(r.source));
}

void test_window_and_validation() {
    const Parsed whole = parse(nvp_test::read_fixture(kNov), kNone, kNone);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Reason::DateTooOld),
                          static_cast<int>(nvp::providers::validate_batch(whole.records.data(), whole.records.size(),
                                                                          whole.info, 9206, kToday)));
    const Parsed p = parse(nvp_test::read_fixture(kNov), Date{2026, 11, 23}, Date{2026, 11, 30});
    TEST_ASSERT_EQUAL_UINT32(8, p.records.size());
    TEST_ASSERT_EQUAL_UINT32(22, p.info.records_dropped);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Reason::Ok),
                          static_cast<int>(nvp::providers::validate_batch(p.records.data(), p.records.size(), p.info,
                                                                          9206, kToday)));
}

void test_one_byte_chunks_same() {
    const std::string body = nvp_test::read_fixture(kNov);
    const Parsed a = parse(body, Date{2026, 11, 23}, Date{2026, 11, 30});
    const Parsed b = parse(body, Date{2026, 11, 23}, Date{2026, 11, 30}, 1);
    TEST_ASSERT_EQUAL_UINT32(a.records.size(), b.records.size());
    for (size_t i = 0; i < a.records.size(); ++i) TEST_ASSERT_EQUAL_UINT16_ARRAY(a.records[i].minutes, b.records[i].minutes, 6);
}

void test_failures() {
    const Parsed e503 = parse(nvp_test::read_fixture(k503), Date{2026, 11, 23}, Date{2026, 11, 30});
    TEST_ASSERT_FALSE(e503.info.success);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Reason::NotSuccess),
                          static_cast<int>(nvp::providers::validate_batch(e503.records.data(), e503.records.size(),
                                                                          e503.info, 9206, kToday)));
    const Parsed html = parse("<html>Bad gateway</html>", Date{2026, 11, 23}, Date{2026, 11, 30});
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Reason::Malformed),
                          static_cast<int>(nvp::providers::validate_batch(html.records.data(), html.records.size(),
                                                                          html.info, 9206, kToday)));
}

// Every time within one minute of the published İmsakiyem table for the 38 overlapping days.
struct Published : RecordSink {
    std::vector<DayRecord> records;
    void on_record(const DayRecord& r) override { records.push_back(r); }
};

void test_within_a_minute_of_published() {
    Published pub;
    nvp::providers::ImsakiyemParser ip(nvp::providers::ImsakiyemApi::V2, pub);
    const std::string body = nvp_test::read_fixture("imsakiyem/v2_times_9206_2026-11-24_2026-12-31.json");
    ip.feed(body.data(), body.size());
    ip.finish();
    std::vector<DayRecord> cal = parse(nvp_test::read_fixture(kNov), kNone, kNone).records;
    const std::vector<DayRecord> dec = parse(nvp_test::read_fixture(kDec), kNone, kNone).records;
    cal.insert(cal.end(), dec.begin(), dec.end());
    int days = 0;
    for (const DayRecord& p : pub.records) {
        for (const DayRecord& c : cal) {
            if (!(c.date == p.date)) continue;
            ++days;
            for (int i = 0; i < 6; ++i) TEST_ASSERT_TRUE(std::abs(static_cast<int>(p.minutes[i]) - c.minutes[i]) <= 1);
        }
    }
    TEST_ASSERT_EQUAL_INT(38, days);
}

void test_url() {
    char url[200];
    TEST_ASSERT_TRUE(nvp::providers::aladhan_url(2026, 11, 3992063, 3285403, url, sizeof url));
    TEST_ASSERT_EQUAL_STRING(
        "https://api.aladhan.com/v1/calendar/2026/11?latitude=39.92063&longitude=32.85403&method=13&school=0"
        "&timezonestring=Europe/Istanbul",
        url);
}

}  // namespace

void run_aladhan_tests() {
    RUN_TEST(test_whole_months);
    RUN_TEST(test_record_mapping);
    RUN_TEST(test_window_and_validation);
    RUN_TEST(test_one_byte_chunks_same);
    RUN_TEST(test_failures);
    RUN_TEST(test_within_a_minute_of_published);
    RUN_TEST(test_url);
}
