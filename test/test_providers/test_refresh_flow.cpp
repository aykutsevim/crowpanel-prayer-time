// The refresh flow on the host (spec 006 T014a, FR-005..FR-009, SC-005, analysis D1): a fake HTTP
// returns the recorded answers or altered copies, a fake file system holds the store.
#include <unity.h>

#include <string>
#include <vector>

#include "../support/fake_fs.h"
#include "../support/fixture_file.h"
#include "nvp/providers/refresh.h"

using nvp::engine::Date;
using nvp::providers::HttpBody;
using nvp::providers::HttpGet;
using nvp::providers::HttpOutcome;
using nvp::providers::HttpResult;
using nvp::providers::Provider;
using nvp::providers::Reason;
using nvp::providers::RefreshRequest;
using nvp::providers::RefreshResult;
using nvp::providers::RefreshScratch;
using nvp::store::Store;

namespace {

const Date kToday{2026, 11, 24};
const int64_t kNow = 1795524049;

struct Answer {
    HttpOutcome outcome = HttpOutcome::Done;
    int status = 200;
    std::string body;
};

struct FakeHttp : HttpGet {
    Answer v2, v1;
    std::vector<std::string> urls;
    size_t chunk = 1024;
    HttpResult get(const char* url, HttpBody& body) override {
        urls.push_back(url);
        const Answer& a = std::string(url).find("/api/v2/") != std::string::npos ? v2 : v1;
        HttpResult r;
        if (a.outcome != HttpOutcome::Done) {
            r.outcome = a.outcome;
            return r;
        }
        r.status = a.status;
        body.on_status(a.status);
        for (size_t at = 0; at < a.body.size(); at += chunk) {
            size_t n = a.body.size() - at < chunk ? a.body.size() - at : chunk;
            if (!body.on_chunk(a.body.data() + at, n)) {
                r.outcome = HttpOutcome::Aborted;
                return r;
            }
            r.bytes += static_cast<uint32_t>(n);
        }
        r.outcome = HttpOutcome::Done;
        return r;
    }
};

struct CountingGate : nvp::providers::SaveGate {
    int calls = 0;
    void before_save() override { ++calls; }
};

RefreshScratch g_scratch;
Store g_store, g_loaded;

std::string v2_body() { return nvp_test::read_fixture("imsakiyem/v2_times_9206_2026-11-24_2026-12-31.json"); }
std::string v1_body() { return nvp_test::read_fixture("imsakiyem/v1_range_9206_2026-11-24_2026-12-23.json"); }

RefreshRequest req() {
    RefreshRequest r;
    r.district = 9206;
    r.today = kToday;
    r.now_utc = kNow;
    return r;
}

void empty_store() {
    g_store = Store{};
    g_store.district_id = 9206;
}

void test_urls() {  // R1
    char buf[160];
    TEST_ASSERT_TRUE(nvp::providers::v2_url(9206, kToday, buf, sizeof buf));
    TEST_ASSERT_EQUAL_STRING(
        "https://ezanvakti.imsakiyem.com/api/v2/times?district=9206&from=2026-11-24&to=2027-11-24", buf);
    TEST_ASSERT_TRUE(nvp::providers::v1_url(9206, kToday, buf, sizeof buf));
    TEST_ASSERT_EQUAL_STRING(
        "https://ezanvakti.imsakiyem.com/api/prayer-times/9206/range?startDate=2026-11-24&endDate=2026-12-23", buf);
    TEST_ASSERT_FALSE(nvp::providers::v2_url(9206, kToday, buf, 20));
}

void test_v2_ok_stores() {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.body = v2_body();
    CountingGate gate;
    empty_store();
    RefreshResult r = nvp::providers::refresh_with(http, fs, req(), g_store, g_scratch, &gate);
    TEST_ASSERT_TRUE(r.stored);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Provider::ImsakiyemV2), static_cast<int>(r.provider));
    TEST_ASSERT_EQUAL_size_t(1, http.urls.size());
    TEST_ASSERT_EQUAL_UINT16(38, g_store.count);
    TEST_ASSERT_EQUAL_INT64(kNow, g_store.last_refresh_utc);
    TEST_ASSERT_EQUAL_INT(1, gate.calls);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::store::LoadResult::Ok),
                          static_cast<int>(nvp::store::load(fs, 9206, g_loaded)));
    TEST_ASSERT_EQUAL_UINT16(38, g_loaded.count);
    TEST_ASSERT_EQUAL_UINT32(38, r.v2.records);
    TEST_ASSERT_FALSE(r.v1.tried);
}

// A rejected v2 answer and a failing v1 leave the store and its file exactly as they were.
void expect_unchanged_after(const std::string& v2, Reason want_v2) {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.body = v2_body();
    empty_store();
    TEST_ASSERT_TRUE(nvp::providers::refresh_with(http, fs, req(), g_store, g_scratch).stored);
    const std::string file_before = fs.files["store_9206.bin"];
    const uint16_t count_before = g_store.count;
    const uint16_t first_before = g_store.records[0].minutes[0];

    http.urls.clear();
    http.v2.body = v2;
    http.v1.outcome = HttpOutcome::Timeout;
    RefreshResult r = nvp::providers::refresh_with(http, fs, req(), g_store, g_scratch);
    TEST_ASSERT_FALSE(r.stored);
    TEST_ASSERT_EQUAL_STRING(nvp::providers::reason_text(want_v2), nvp::providers::reason_text(r.v2.reason));
    TEST_ASSERT_TRUE(r.v1.tried);
    TEST_ASSERT_EQUAL_STRING(nvp::providers::reason_text(Reason::Network), nvp::providers::reason_text(r.v1.reason));
    TEST_ASSERT_EQUAL_UINT16(count_before, g_store.count);
    TEST_ASSERT_EQUAL_UINT16(first_before, g_store.records[0].minutes[0]);
    TEST_ASSERT_TRUE(file_before == fs.files["store_9206.bin"]);
    TEST_ASSERT_EQUAL_size_t(1, fs.files.size());
}

void test_rejection_leaves_store_and_file() {  // FR-007, SC-005
    std::string wrong = v2_body();
    nvp_test::replace_first(wrong, "\"district_id\":\"9206\"", "\"district_id\":\"9205\"");
    expect_unchanged_after(wrong, Reason::WrongDistrict);
    std::string bad = v2_body();
    nvp_test::replace_first(bad, "\"ogle\":\"12:40\"", "\"ogle\":\"07:00\"");
    expect_unchanged_after(bad, Reason::BadRecord);
    std::string cut = v2_body();
    expect_unchanged_after(cut.substr(0, cut.size() - 5), Reason::Truncated);
}

void test_district_not_found_skips_v1() {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.status = 404;
    http.v2.body = nvp_test::read_fixture("imsakiyem/v2_times_district_not_found.json");
    http.v1.body = v1_body();
    empty_store();
    RefreshResult r = nvp::providers::refresh_with(http, fs, req(), g_store, g_scratch);
    TEST_ASSERT_FALSE(r.stored);
    TEST_ASSERT_EQUAL_size_t(1, http.urls.size());
    TEST_ASSERT_FALSE(r.v1.tried);
    TEST_ASSERT_EQUAL_STRING(nvp::providers::reason_text(Reason::ErrorCode), nvp::providers::reason_text(r.reason));
    TEST_ASSERT_EQUAL_size_t(0, fs.files.size());
}

void test_v2_network_error_falls_back_to_v1() {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.outcome = HttpOutcome::Tls;
    http.v1.body = v1_body();
    empty_store();
    RefreshResult r = nvp::providers::refresh_with(http, fs, req(), g_store, g_scratch);
    TEST_ASSERT_TRUE(r.stored);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Provider::ImsakiyemV1), static_cast<int>(r.provider));
    TEST_ASSERT_EQUAL_size_t(2, http.urls.size());
    TEST_ASSERT_EQUAL_STRING(nvp::providers::reason_text(Reason::Network), nvp::providers::reason_text(r.v2.reason));
    TEST_ASSERT_EQUAL_UINT16(30, g_store.count);
}

void test_v2_other_status_falls_back() {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.status = 503;
    http.v2.body = "<html>busy</html>";
    http.v1.body = v1_body();
    empty_store();
    RefreshResult r = nvp::providers::refresh_with(http, fs, req(), g_store, g_scratch);
    TEST_ASSERT_TRUE(r.stored);
    TEST_ASSERT_EQUAL_STRING(nvp::providers::reason_text(Reason::Http), nvp::providers::reason_text(r.v2.reason));
    TEST_ASSERT_EQUAL_INT(503, r.v2.http.status);
}

void test_v1_wrong_district_rejected() {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.outcome = HttpOutcome::Timeout;
    http.v1.body = v1_body();
    RefreshRequest other = req();
    other.district = 9541;
    g_store = Store{};
    g_store.district_id = 9541;
    RefreshResult r = nvp::providers::refresh_with(http, fs, other, g_store, g_scratch);
    TEST_ASSERT_FALSE(r.stored);
    TEST_ASSERT_EQUAL_STRING(nvp::providers::reason_text(Reason::WrongDistrict),
                             nvp::providers::reason_text(r.v1.reason));
    TEST_ASSERT_EQUAL_UINT16(0, g_store.count);
}

void test_save_failure_leaves_store() {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.body = v2_body();
    empty_store();
    fs.fail_once_at(2);  // the open of the .new file (after the exists() check)
    RefreshResult r = nvp::providers::refresh_with(http, fs, req(), g_store, g_scratch);
    TEST_ASSERT_FALSE(r.stored);
    TEST_ASSERT_TRUE(r.save != nvp::store::SaveResult::Ok);
    TEST_ASSERT_EQUAL_UINT16(0, g_store.count);
}

void test_merge_keeps_yesterday() {  // a second refresh the next day keeps yesterday's record
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.body = v2_body();
    empty_store();
    nvp::providers::refresh_with(http, fs, req(), g_store, g_scratch);
    RefreshRequest next = req();
    next.today = Date{2026, 11, 26};
    next.now_utc = kNow + 2 * 86400;
    RefreshResult r = nvp::providers::refresh_with(http, fs, next, g_store, g_scratch);
    TEST_ASSERT_TRUE(r.stored);
    // 2026-11-24 is pruned (before yesterday), 2026-11-25 kept as yesterday.
    TEST_ASSERT_TRUE(g_store.records[0].date == (Date{2026, 11, 25}));
    TEST_ASSERT_EQUAL_UINT16(37, g_store.count);
}

}  // namespace

void run_refresh_flow_tests() {
    RUN_TEST(test_urls);
    RUN_TEST(test_v2_ok_stores);
    RUN_TEST(test_rejection_leaves_store_and_file);
    RUN_TEST(test_district_not_found_skips_v1);
    RUN_TEST(test_v2_network_error_falls_back_to_v1);
    RUN_TEST(test_v2_other_status_falls_back);
    RUN_TEST(test_v1_wrong_district_rejected);
    RUN_TEST(test_save_failure_leaves_store);
    RUN_TEST(test_merge_keeps_yesterday);
}
