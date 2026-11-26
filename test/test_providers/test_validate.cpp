// Batch validation on altered copies of the recorded v2 answer (spec 006 T005, research R4,
// FR-006, SC-005). The fixture files are never edited; every change is made in memory.
#include <unity.h>

#include <cstdio>
#include <string>
#include <vector>

#include "../support/fixture_file.h"
#include "nvp/engine/calendar.h"
#include "nvp/providers/imsakiyem.h"
#include "nvp/providers/validate.h"

using nvp::engine::Date;
using nvp::engine::DayRecord;
using nvp::providers::ImsakiyemApi;
using nvp::providers::ImsakiyemParser;
using nvp::providers::Reason;
using nvp::providers::RecordSink;
using nvp_test::replace_first;

namespace {

const Date kToday{2026, 11, 24};
const uint32_t kDistrict = 9206;

struct Collect : RecordSink {
    std::vector<DayRecord> records;
    void on_record(const DayRecord& r) override { records.push_back(r); }
};

Reason check(const std::string& body, Date today = kToday, ImsakiyemApi api = ImsakiyemApi::V2) {
    Collect c;
    ImsakiyemParser p(api, c);
    p.feed(body.data(), body.size());
    p.finish();
    return nvp::providers::validate_batch(c.records.data(), c.records.size(), p.info(), kDistrict, today);
}

std::string v2() {
    std::string s = nvp_test::read_fixture("imsakiyem/v2_times_9206_2026-11-24_2026-12-31.json");
    TEST_ASSERT_TRUE(!s.empty());
    return s;
}

void expect(Reason want, Reason got) {
    TEST_ASSERT_EQUAL_STRING(nvp::providers::reason_text(want), nvp::providers::reason_text(got));
}

std::string date_text(Date d) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02d", d.year, d.month, d.day);
    return buf;
}

void test_valid_answers() {
    expect(Reason::Ok, check(v2()));
    expect(Reason::Ok, check(nvp_test::read_fixture("imsakiyem/v2_times_9206_2026-11-24_2027-03-31.json")));
    expect(Reason::Ok, check(nvp_test::read_fixture("imsakiyem/v1_range_9206_2026-11-24_2026-12-23.json"), kToday,
                             ImsakiyemApi::V1));
    // Two days later the first two records lie behind today but within the 2-day allowance.
    expect(Reason::Ok, check(v2(), Date{2026, 11, 26}));
}

void test_truncated() {
    std::string s = v2();
    Reason r = check(s.substr(0, s.size() / 2));
    TEST_ASSERT_TRUE(r == Reason::Truncated || r == Reason::Malformed);
    expect(Reason::Truncated, check(s.substr(0, s.size() - 1)));  // only the final } missing
}

void test_reordered_is_ok() {  // order is not required, uniqueness is
    std::string s = v2();
    TEST_ASSERT_TRUE(replace_first(s, "\"date\":\"2026-11-24\"", "\"date\":\"PLACEHOLDER\""));
    TEST_ASSERT_TRUE(replace_first(s, "\"date\":\"2026-11-25\"", "\"date\":\"2026-11-24\""));
    TEST_ASSERT_TRUE(replace_first(s, "\"date\":\"PLACEHOLDER\"", "\"date\":\"2026-11-25\""));
    expect(Reason::Ok, check(s));
}

void test_duplicate_date() {
    std::string s = v2();
    TEST_ASSERT_TRUE(replace_first(s, "\"date\":\"2026-11-25\"", "\"date\":\"2026-11-24\""));
    expect(Reason::DuplicateDate, check(s));
}

void test_date_too_far() {
    std::string s = v2();
    Date far = nvp::engine::to_date(nvp::engine::to_day_number(kToday) + 401);
    TEST_ASSERT_TRUE(replace_first(s, "\"date\":\"2026-12-31\"", "\"date\":\"" + date_text(far) + "\""));
    expect(Reason::DateTooFar, check(s));
    // Exactly 400 days ahead is allowed.
    std::string ok = v2();
    Date edge = nvp::engine::to_date(nvp::engine::to_day_number(kToday) + 400);
    TEST_ASSERT_TRUE(replace_first(ok, "\"date\":\"2026-12-31\"", "\"date\":\"" + date_text(edge) + "\""));
    expect(Reason::Ok, check(ok));
}

void test_all_dates_old() {
    Reason r = check(v2(), Date{2027, 6, 1});
    TEST_ASSERT_TRUE(r == Reason::DateTooOld || r == Reason::NoFutureDate);
    // Within the 2-day allowance but nothing from today on.
    std::string s = v2();
    Reason r2 = check(s, Date{2027, 1, 1});
    TEST_ASSERT_TRUE(r2 == Reason::DateTooOld || r2 == Reason::NoFutureDate);
}

void test_no_future_date() {
    // Keep only the first record, and set today one day after it.
    std::string s = v2();
    size_t start = s.find("{\"date\":\"2026-11-25\"");
    size_t end = s.find("]},\"meta\"");
    TEST_ASSERT_TRUE(start != std::string::npos && end != std::string::npos);
    s.erase(start - 1, end - (start - 1));  // drop ",{...},{...}" up to the closing ]
    expect(Reason::NoFutureDate, check(s, Date{2026, 11, 25}));
}

void test_bad_record() {  // ENG-01
    std::string s = v2();
    TEST_ASSERT_TRUE(replace_first(s, "\"ogle\":\"12:40\"", "\"ogle\":\"07:00\""));
    expect(Reason::BadRecord, check(s));
    std::string t = v2();
    TEST_ASSERT_TRUE(replace_first(t, "\"imsak\":\"06:08\"", "\"imsak\":\"25:00\""));
    expect(Reason::BadRecord, check(t));
    std::string missing = v2();
    TEST_ASSERT_TRUE(replace_first(missing, "\"yatsi\":\"18:57\",", ""));
    expect(Reason::BadRecord, check(missing));
}

void test_bad_date() {
    std::string s = v2();
    TEST_ASSERT_TRUE(replace_first(s, "\"date\":\"2026-11-30\"", "\"date\":\"2026-11-31\""));
    expect(Reason::BadDate, check(s));
    std::string m = v2();
    TEST_ASSERT_TRUE(replace_first(m, "\"date\":\"2026-11-30\",", ""));
    expect(Reason::BadDate, check(m));
}

void test_empty() {
    std::string s = v2();
    size_t start = s.find("\"times\":[");
    size_t end = s.find("]},\"meta\"");
    s.replace(start, end - start + 1, "\"times\":[]");
    expect(Reason::Empty, check(s));
}

void test_wrong_district() {
    std::string s = v2();
    TEST_ASSERT_TRUE(replace_first(s, "\"district_id\":\"9206\"", "\"district_id\":\"9205\""));
    expect(Reason::WrongDistrict, check(s));
    std::string none = v2();
    TEST_ASSERT_TRUE(replace_first(none, "\"district_id\":\"9206\",", ""));
    expect(Reason::WrongDistrict, check(none));
}

void test_not_success() {
    std::string s = v2();
    TEST_ASSERT_TRUE(replace_first(s, "\"success\":true", "\"success\":false"));
    expect(Reason::NotSuccess, check(s));
    expect(Reason::ErrorCode, check(nvp_test::read_fixture("imsakiyem/v2_times_district_not_found.json")));
}

void test_wrong_source() {  // constitution II: only Diyanet-published times are stored
    std::string s = v2();
    TEST_ASSERT_TRUE(replace_first(s, "\"type\":\"diyanet\"", "\"type\":\"copied_from_center\""));
    expect(Reason::WrongSource, check(s));
}

void test_too_many() {
    std::vector<DayRecord> none;
    nvp::providers::BatchInfo info;
    info.success_seen = info.success = true;
    info.district_seen = true;
    info.district = kDistrict;
    info.source_diyanet = true;
    info.records_array_seen = true;
    info.complete = true;
    info.records_seen = 401;
    expect(Reason::TooMany, nvp::providers::validate_batch(none.data(), 0, info, kDistrict, kToday));
}

void test_long_needed_field() {
    std::string s = v2();
    TEST_ASSERT_TRUE(replace_first(s, "\"imsak\":\"06:08\"", "\"imsak\":\"" + std::string(70, '0') + "\""));
    expect(Reason::Malformed, check(s));
    // A long text the parser does not need is fine.
    std::string ok = v2();
    TEST_ASSERT_TRUE(replace_first(ok, "\"provider\":\"", "\"provider\":\"" + std::string(100, 'x')));
    expect(Reason::Ok, check(ok));
}

}  // namespace

void run_validate_tests() {
    RUN_TEST(test_valid_answers);
    RUN_TEST(test_truncated);
    RUN_TEST(test_reordered_is_ok);
    RUN_TEST(test_duplicate_date);
    RUN_TEST(test_date_too_far);
    RUN_TEST(test_all_dates_old);
    RUN_TEST(test_no_future_date);
    RUN_TEST(test_bad_record);
    RUN_TEST(test_bad_date);
    RUN_TEST(test_empty);
    RUN_TEST(test_wrong_district);
    RUN_TEST(test_not_success);
    RUN_TEST(test_wrong_source);
    RUN_TEST(test_too_many);
    RUN_TEST(test_long_needed_field);
}
