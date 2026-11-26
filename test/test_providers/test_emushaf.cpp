// emushaf parser on the recorded answers (spec 007 T015, research R1, FR-009).
#include <unity.h>

#include <string>
#include <vector>

#include "../support/fixture_file.h"
#include "nvp/engine/calendar.h"
#include "nvp/providers/emushaf.h"
#include "nvp/providers/imsakiyem.h"
#include "nvp/providers/validate.h"

using nvp::engine::Date;
using nvp::engine::DayRecord;
using nvp::providers::BatchInfo;
using nvp::providers::EmushafParser;
using nvp::providers::Reason;
using nvp::providers::RecordSink;

namespace {

const char* kVakitler = "emushaf/vakitler_9206_2026-11-24.json";
const char* k502 = "emushaf/vakitler_999999_502.txt";
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

Parsed parse(const std::string& body, Date keep_from, size_t chunk) {
    Collect c;
    EmushafParser p(c, keep_from);
    for (size_t at = 0; at < body.size(); at += chunk) {
        const size_t n = body.size() - at < chunk ? body.size() - at : chunk;
        if (!p.feed(body.data() + at, n)) break;
    }
    p.finish();
    return Parsed{c.records, p.info()};
}

Reason validate(const Parsed& p) {
    return nvp::providers::validate_batch(p.records.data(), p.records.size(), p.info, 9206, kToday);
}

void test_whole_answer() {
    const std::string body = nvp_test::read_fixture(kVakitler);
    TEST_ASSERT_FALSE(body.empty());
    const Parsed p = parse(body, kNone, 4096);
    TEST_ASSERT_EQUAL_UINT32(32, p.records.size());
    TEST_ASSERT_TRUE(p.records.front().date == (Date{2026, 11, 19}));
    TEST_ASSERT_TRUE(p.records.back().date == (Date{2026, 12, 20}));
    TEST_ASSERT_TRUE(p.info.complete);
    TEST_ASSERT_TRUE(p.info.success);
}

// Ankara, 24.11.2026: the same as İmsakiyem (06:08 07:36 12:40 15:12 17:34 18:57, 15/6/1448).
void test_record_equals_imsakiyem() {
    const Parsed p = parse(nvp_test::read_fixture(kVakitler), kNone, 4096);
    const DayRecord* r = nullptr;
    for (const DayRecord& x : p.records)
        if (x.date == kToday) r = &x;
    TEST_ASSERT_NOT_NULL(r);
    const uint16_t want[6] = {368, 456, 760, 912, 1054, 1137};
    TEST_ASSERT_EQUAL_UINT16_ARRAY(want, r->minutes, 6);
    TEST_ASSERT_TRUE(r->hijri.present);
    TEST_ASSERT_EQUAL_UINT8(15, r->hijri.day);
    TEST_ASSERT_EQUAL_UINT8(6, r->hijri.month);
    TEST_ASSERT_EQUAL_UINT16(1448, r->hijri.year);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::engine::SourceId::Emushaf), static_cast<int>(r->source));
}

// Records before yesterday are dropped before validation; the rest validates (research R1).
void test_drop_before_yesterday_then_valid() {
    const Parsed whole = parse(nvp_test::read_fixture(kVakitler), kNone, 4096);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Reason::DateTooOld), static_cast<int>(validate(whole)));
    const Parsed p = parse(nvp_test::read_fixture(kVakitler), Date{2026, 11, 23}, 4096);
    TEST_ASSERT_EQUAL_UINT32(28, p.records.size());
    TEST_ASSERT_EQUAL_UINT32(4, p.info.records_dropped);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Reason::Ok), static_cast<int>(validate(p)));
}

void test_one_byte_chunks_same() {
    const std::string body = nvp_test::read_fixture(kVakitler);
    const Parsed a = parse(body, Date{2026, 11, 23}, 4096);
    const Parsed b = parse(body, Date{2026, 11, 23}, 1);
    TEST_ASSERT_EQUAL_UINT32(a.records.size(), b.records.size());
    for (size_t i = 0; i < a.records.size(); ++i) {
        TEST_ASSERT_TRUE(a.records[i].date == b.records[i].date);
        TEST_ASSERT_EQUAL_UINT16_ARRAY(a.records[i].minutes, b.records[i].minutes, 6);
    }
}

// The 502 page for an unknown district is not JSON: a parse failure, not "district not found".
void test_502_page_is_malformed() {
    const Parsed p = parse(nvp_test::read_fixture(k502), Date{2026, 11, 23}, 4096);
    TEST_ASSERT_EQUAL_UINT32(0, p.records.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Reason::Malformed), static_cast<int>(validate(p)));
}

void test_empty_array() {
    const Parsed p = parse("[]", Date{2026, 11, 23}, 4096);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Reason::Empty), static_cast<int>(validate(p)));
}

void test_bad_time_is_bad_record() {
    std::string body = nvp_test::read_fixture(kVakitler);
    // The Öğle of 24.11.2026 (a kept record) becomes 25:00.
    const size_t at = body.find("\"Ogle\":\"12:40\"", body.find("24.11.2026"));
    TEST_ASSERT_TRUE(at != std::string::npos);
    body.replace(at, 14, "\"Ogle\":\"25:00\"");
    const Parsed p = parse(body, Date{2026, 11, 23}, 4096);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Reason::BadRecord), static_cast<int>(validate(p)));
}

}  // namespace

void run_emushaf_tests() {
    RUN_TEST(test_whole_answer);
    RUN_TEST(test_record_equals_imsakiyem);
    RUN_TEST(test_drop_before_yesterday_then_valid);
    RUN_TEST(test_one_byte_chunks_same);
    RUN_TEST(test_502_page_is_malformed);
    RUN_TEST(test_empty_array);
    RUN_TEST(test_bad_time_is_bad_record);
}
