// İmsakiyem parsers on the recorded answers (spec 006 T004, research R1, R2, FR-001, FR-012).
#include <unity.h>

#include <string>
#include <vector>

#include "../support/fixture_file.h"
#include "nvp/engine/calendar.h"
#include "nvp/providers/imsakiyem.h"

using nvp::engine::DayRecord;
using nvp::engine::SourceId;
using nvp::providers::BatchInfo;
using nvp::providers::ImsakiyemApi;
using nvp::providers::ImsakiyemParser;
using nvp::providers::RecordSink;

namespace {

const char* kV2Short = "imsakiyem/v2_times_9206_2026-11-24_2026-12-31.json";
const char* kV2Long = "imsakiyem/v2_times_9206_2026-11-24_2027-03-31.json";
const char* kV2NotFound = "imsakiyem/v2_times_district_not_found.json";
const char* kV1 = "imsakiyem/v1_range_9206_2026-11-24_2026-12-23.json";

struct Collect : RecordSink {
    std::vector<DayRecord> records;
    void on_record(const DayRecord& r) override { records.push_back(r); }
};

struct Parsed {
    std::vector<DayRecord> records;
    BatchInfo info;
};

Parsed parse(ImsakiyemApi api, const std::string& body, size_t chunk) {
    Collect c;
    ImsakiyemParser p(api, c);
    for (size_t at = 0; at < body.size(); at += chunk) {
        size_t n = body.size() - at < chunk ? body.size() - at : chunk;
        if (!p.feed(body.data() + at, n)) break;
    }
    p.finish();
    return Parsed{c.records, p.info()};
}

std::string load(const char* path) {
    std::string s = nvp_test::read_fixture(path);
    TEST_ASSERT_TRUE_MESSAGE(!s.empty(), path);
    return s;
}

bool same(const DayRecord& a, const DayRecord& b) {
    if (a.date != b.date || a.source != b.source) return false;
    for (int i = 0; i < 6; ++i)
        if (a.minutes[i] != b.minutes[i]) return false;
    return a.hijri.present == b.hijri.present && a.hijri.day == b.hijri.day && a.hijri.month == b.hijri.month &&
           a.hijri.year == b.hijri.year;
}

uint16_t hm(int h, int m) { return static_cast<uint16_t>(h * 60 + m); }

void test_v2_short() {  // R1: 38 records, first 2026-11-24, last 2026-12-31
    Parsed p = parse(ImsakiyemApi::V2, load(kV2Short), 100000);
    TEST_ASSERT_TRUE(p.info.complete);
    TEST_ASSERT_EQUAL_size_t(38, p.records.size());
    TEST_ASSERT_EQUAL_UINT32(38, p.info.records_seen);
    TEST_ASSERT_TRUE(p.info.success_seen);
    TEST_ASSERT_TRUE(p.info.success);
    TEST_ASSERT_TRUE(p.info.district_seen);
    TEST_ASSERT_EQUAL_UINT32(9206, p.info.district);
    TEST_ASSERT_FALSE(p.info.district_mixed);
    TEST_ASSERT_TRUE(p.info.source_diyanet);
    TEST_ASSERT_TRUE(p.info.records_array_seen);
    TEST_ASSERT_FALSE(p.info.field_overflow);

    const DayRecord& r = p.records.front();
    TEST_ASSERT_TRUE(r.date == (nvp::engine::Date{2026, 11, 24}));
    const uint16_t want[6] = {hm(6, 8), hm(7, 36), hm(12, 40), hm(15, 12), hm(17, 34), hm(18, 57)};
    TEST_ASSERT_EQUAL_UINT16_ARRAY(want, r.minutes, 6);
    TEST_ASSERT_TRUE(r.hijri.present);
    TEST_ASSERT_EQUAL_UINT8(15, r.hijri.day);
    TEST_ASSERT_EQUAL_UINT8(6, r.hijri.month);
    TEST_ASSERT_EQUAL_UINT16(1448, r.hijri.year);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SourceId::Imsakiyem), static_cast<int>(r.source));
    TEST_ASSERT_TRUE(nvp::engine::valid(r));
    TEST_ASSERT_TRUE(p.records.back().date == (nvp::engine::Date{2026, 12, 31}));
    for (const DayRecord& x : p.records) TEST_ASSERT_TRUE(nvp::engine::valid(x));
}

void test_v2_long() {  // R1: 2027 already published, 128 records
    Parsed p = parse(ImsakiyemApi::V2, load(kV2Long), 1024);
    TEST_ASSERT_TRUE(p.info.complete);
    TEST_ASSERT_EQUAL_size_t(128, p.records.size());
    TEST_ASSERT_TRUE(p.records.back().date == (nvp::engine::Date{2027, 3, 31}));
}

void test_v2_not_found() {  // R1: 404 with DISTRICT_NOT_FOUND
    Parsed p = parse(ImsakiyemApi::V2, load(kV2NotFound), 7);
    TEST_ASSERT_TRUE(p.info.complete);
    TEST_ASSERT_TRUE(p.info.success_seen);
    TEST_ASSERT_FALSE(p.info.success);
    TEST_ASSERT_EQUAL_STRING("DISTRICT_NOT_FOUND", p.info.error_code);
    TEST_ASSERT_EQUAL_size_t(0, p.records.size());
    TEST_ASSERT_FALSE(p.info.records_array_seen);
}

void test_v1_equals_v2() {  // R1: v1 and v2 agree for 30 days
    Parsed v1 = parse(ImsakiyemApi::V1, load(kV1), 64);
    Parsed v2 = parse(ImsakiyemApi::V2, load(kV2Short), 64);
    TEST_ASSERT_TRUE(v1.info.complete);
    TEST_ASSERT_TRUE(v1.info.success);
    TEST_ASSERT_EQUAL_UINT32(9206, v1.info.district);
    TEST_ASSERT_FALSE(v1.info.district_mixed);
    TEST_ASSERT_TRUE(v1.info.source_diyanet);
    TEST_ASSERT_EQUAL_size_t(30, v1.records.size());
    for (size_t i = 0; i < 30; ++i) TEST_ASSERT_TRUE_MESSAGE(same(v1.records[i], v2.records[i]), "v1 != v2");
}

void test_one_byte_chunks() {  // R2
    const char* files[] = {kV2Short, kV1};
    const ImsakiyemApi apis[] = {ImsakiyemApi::V2, ImsakiyemApi::V1};
    for (int f = 0; f < 2; ++f) {
        std::string body = load(files[f]);
        Parsed whole = parse(apis[f], body, body.size());
        Parsed bytes = parse(apis[f], body, 1);
        TEST_ASSERT_TRUE(bytes.info.complete);
        TEST_ASSERT_EQUAL_size_t(whole.records.size(), bytes.records.size());
        for (size_t i = 0; i < whole.records.size(); ++i) TEST_ASSERT_TRUE(same(whole.records[i], bytes.records[i]));
    }
}

void test_v1_mixed_districts() {
    std::string body = load(kV1);
    // The second record's district object: rename its id.
    size_t first = body.find("\"district_id\":{\"_id\":\"9206\"");
    size_t second = body.find("\"district_id\":{\"_id\":\"9206\"", first + 1);
    TEST_ASSERT_TRUE(second != std::string::npos);
    body.replace(second, std::string("\"district_id\":{\"_id\":\"9206\"").size(), "\"district_id\":{\"_id\":\"9205\"");
    Parsed p = parse(ImsakiyemApi::V1, body, 1024);
    TEST_ASSERT_TRUE(p.info.district_mixed);
}

void test_bad_fields_make_invalid_records() {  // a field that cannot be read is never guessed
    std::string body = load(kV2Short);
    TEST_ASSERT_TRUE(nvp_test::replace_first(body, "\"imsak\":\"06:08\"", "\"imsak\":\"25:00\""));
    TEST_ASSERT_TRUE(nvp_test::replace_first(body, "\"date\":\"2026-11-25\"", "\"date\":\"2026-11-31\""));
    Parsed p = parse(ImsakiyemApi::V2, body, 1024);
    TEST_ASSERT_EQUAL_UINT16(nvp::providers::kBadMinutes, p.records[0].minutes[0]);
    TEST_ASSERT_FALSE(nvp::engine::valid(p.records[0]));
    TEST_ASSERT_FALSE(nvp::engine::valid(p.records[1].date));
}

void test_helpers() {
    TEST_ASSERT_EQUAL_UINT16(0, nvp::providers::parse_hhmm("00:00", 5));
    TEST_ASSERT_EQUAL_UINT16(1439, nvp::providers::parse_hhmm("23:59", 5));
    TEST_ASSERT_EQUAL_UINT16(nvp::providers::kBadMinutes, nvp::providers::parse_hhmm("24:00", 5));
    TEST_ASSERT_EQUAL_UINT16(nvp::providers::kBadMinutes, nvp::providers::parse_hhmm("6:08", 4));
    TEST_ASSERT_EQUAL_UINT16(nvp::providers::kBadMinutes, nvp::providers::parse_hhmm("06:60", 5));
    nvp::engine::Date d;
    TEST_ASSERT_TRUE(nvp::providers::parse_iso_date("2028-02-29", 10, false, d));
    TEST_ASSERT_FALSE(nvp::providers::parse_iso_date("2027-02-29", 10, false, d));
    TEST_ASSERT_TRUE(nvp::providers::parse_iso_date("2026-11-24T00:00:00.000Z", 24, true, d));
    TEST_ASSERT_FALSE(nvp::providers::parse_iso_date("2026-11-24T00:00:00.000Z", 24, false, d));
    TEST_ASSERT_FALSE(nvp::providers::parse_iso_date("2026/11/24", 10, false, d));
}

}  // namespace

void run_imsakiyem_tests() {
    RUN_TEST(test_v2_short);
    RUN_TEST(test_v2_long);
    RUN_TEST(test_v2_not_found);
    RUN_TEST(test_v1_equals_v2);
    RUN_TEST(test_one_byte_chunks);
    RUN_TEST(test_v1_mixed_districts);
    RUN_TEST(test_bad_fields_make_invalid_records);
    RUN_TEST(test_helpers);
}
