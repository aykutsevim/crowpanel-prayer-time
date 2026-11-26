// The refresh flow on the host, over the HTTP source (spec 006 T014a, FR-005..FR-009, SC-005; moved
// onto spec 007's refresh_run and HttpSource by T046): a fake HTTP returns the recorded answers or
// altered copies, a fake file system holds the store. Coordinates are off, so Aladhan is never asked.
#include <unity.h>

#include <string>
#include <vector>

#include "../support/fake_fs.h"
#include "../support/fixture_file.h"
#include "nvp/providers/http_source.h"
#include "nvp/providers/refresh.h"
#include "nvp/providers/runner.h"

using nvp::engine::Date;
using nvp::providers::HttpBody;
using nvp::providers::HttpOutcome;
using nvp::providers::HttpResult;
using nvp::providers::ProviderId;
using nvp::providers::Reason;
using nvp::providers::RunResult;
using nvp::store::Store;

namespace {

const Date kToday{2026, 11, 24};
const int64_t kNow = 1795524049;

struct Answer {
    HttpOutcome outcome = HttpOutcome::Done;
    int status = 200;
    std::string body;
};

// v2 answers on /api/v2/, v1 on /api/prayer-times/, emushaf fails (connection) unless set.
struct FakeHttp : nvp::providers::HttpGet {
    Answer v2, v1, emushaf;
    std::vector<std::string> urls;
    FakeHttp() { emushaf.outcome = HttpOutcome::Connect; }
    int imsakiyem_requests() const {
        int n = 0;
        for (const std::string& u : urls) n += u.find("imsakiyem") != std::string::npos ? 1 : 0;
        return n;
    }
    HttpResult get(const char* url, HttpBody& body) override {
        urls.push_back(url);
        const std::string u(url);
        const Answer& a = u.find("/api/v2/") != std::string::npos ? v2 : u.find("emushaf") != std::string::npos ? emushaf : v1;
        HttpResult r;
        if (a.outcome != HttpOutcome::Done) {
            r.outcome = a.outcome;
            return r;
        }
        r.status = a.status;
        body.on_status(a.status);
        for (size_t at = 0; at < a.body.size(); at += 1024) {
            const size_t n = a.body.size() - at < 1024 ? a.body.size() - at : 1024;
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

struct CountingGate : nvp::providers::RunControl {
    int calls = 0;
    void before_save() override { ++calls; }
};

nvp::providers::RunScratch g_scratch;
Store g_store, g_loaded;
nvp::store::RefreshState g_state;

std::string v2_body() { return nvp_test::read_fixture("imsakiyem/v2_times_9206_2026-11-24_2026-12-31.json"); }
std::string v1_body() { return nvp_test::read_fixture("imsakiyem/v1_range_9206_2026-11-24_2026-12-23.json"); }

nvp::providers::RunInput input(uint32_t district = 9206, Date today = kToday, int64_t now = kNow) {
    nvp::providers::RunInput in;
    in.district = district;
    in.today = today;
    in.now_utc = now;
    return in;
}

void empty_store(uint32_t district = 9206) {
    g_store = Store{};
    g_store.district_id = district;
    g_state = nvp::store::RefreshState{};
}

RunResult run(FakeHttp& http, nvp_test::FakeFs& fs, const nvp::providers::RunInput& in, CountingGate* gate = nullptr) {
    nvp::providers::HttpSource src(http);
    return nvp::providers::refresh_run(src, fs, in, g_store, g_scratch, g_state, gate);
}

const nvp::providers::ProviderReport& ims(const RunResult& r) { return r.providers[static_cast<size_t>(ProviderId::Imsakiyem)]; }

void test_urls() {  // R1; spec 007 T045: the runner asks from yesterday
    char buf[160];
    TEST_ASSERT_TRUE(nvp::providers::v2_url(9206, kToday, buf, sizeof buf));
    TEST_ASSERT_EQUAL_STRING(
        "https://ezanvakti.imsakiyem.com/api/v2/times?district=9206&from=2026-11-24&to=2027-11-24", buf);
    TEST_ASSERT_TRUE(nvp::providers::v1_url(9206, kToday, buf, sizeof buf));
    TEST_ASSERT_EQUAL_STRING(
        "https://ezanvakti.imsakiyem.com/api/prayer-times/9206/range?startDate=2026-11-24&endDate=2026-12-23", buf);
    TEST_ASSERT_FALSE(nvp::providers::v2_url(9206, kToday, buf, 20));
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.body = v2_body();
    empty_store();
    run(http, fs, input());
    TEST_ASSERT_EQUAL_STRING(
        "https://ezanvakti.imsakiyem.com/api/v2/times?district=9206&from=2026-11-23&to=2027-11-23", http.urls[0].c_str());
}

void test_v2_ok_stores() {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.body = v2_body();
    CountingGate gate;
    empty_store();
    const RunResult r = run(http, fs, input(), &gate);
    TEST_ASSERT_TRUE(r.saved);
    TEST_ASSERT_EQUAL_size_t(1, http.urls.size());
    TEST_ASSERT_EQUAL_UINT16(38, g_store.count);
    TEST_ASSERT_EQUAL_INT64(kNow, g_store.last_refresh_utc);
    TEST_ASSERT_EQUAL_INT(1, gate.calls);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::store::LoadResult::Ok),
                          static_cast<int>(nvp::store::load(fs, 9206, g_loaded)));
    TEST_ASSERT_EQUAL_UINT16(38, g_loaded.count);
    TEST_ASSERT_EQUAL_UINT8(1, ims(r).result.requests);
}

// A rejected v2 answer, a failing v1 and a failing emushaf leave the store and its file exactly as
// they were (FR-007, SC-005); expect_reason() checks the reason of the v2 rejection itself.
void expect_unchanged_after(const std::string& v2) {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.body = v2_body();
    empty_store();
    TEST_ASSERT_TRUE(run(http, fs, input()).saved);
    const std::string file_before = fs.files["store_9206.bin"];
    const uint16_t count_before = g_store.count;
    const uint16_t first_before = g_store.records[0].minutes[0];

    http.urls.clear();
    http.v2.body = v2;
    http.v1.outcome = HttpOutcome::Timeout;
    const RunResult r = run(http, fs, input());
    TEST_ASSERT_FALSE(r.saved);
    TEST_ASSERT_EQUAL_INT(2, http.imsakiyem_requests());  // v2 rejected, then v1
    TEST_ASSERT_EQUAL_STRING(nvp::providers::reason_text(Reason::Network), nvp::providers::reason_text(ims(r).result.reason));
    TEST_ASSERT_EQUAL_UINT16(count_before, g_store.count);
    TEST_ASSERT_EQUAL_UINT16(first_before, g_store.records[0].minutes[0]);
    TEST_ASSERT_TRUE(file_before == fs.files["store_9206.bin"]);
    TEST_ASSERT_EQUAL_size_t(1, fs.files.size());  // the store only (refresh.bin is the caller's)
}

// The reason of the rejected v2 itself, with v1 off (lite): one request, that reason.
void expect_reason(const std::string& v2, Reason want) {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.body = v2;
    empty_store();
    nvp::providers::RunInput in = input();
    in.lite = true;
    const RunResult r = run(http, fs, in);
    TEST_ASSERT_EQUAL_INT(1, http.imsakiyem_requests());
    TEST_ASSERT_EQUAL_STRING(nvp::providers::reason_text(want), nvp::providers::reason_text(ims(r).result.reason));
    TEST_ASSERT_EQUAL_UINT16(0, g_store.count);
}

void test_rejection_leaves_store_and_file() {
    std::string wrong = v2_body();
    nvp_test::replace_first(wrong, "\"district_id\":\"9206\"", "\"district_id\":\"9205\"");
    std::string bad = v2_body();
    nvp_test::replace_first(bad, "\"ogle\":\"12:40\"", "\"ogle\":\"07:00\"");
    const std::string full = v2_body();
    const std::string cut = full.substr(0, full.size() - 5);
    expect_unchanged_after(wrong);
    expect_unchanged_after(bad);
    expect_unchanged_after(cut);
    expect_reason(wrong, Reason::WrongDistrict);
    expect_reason(bad, Reason::BadRecord);
    expect_reason(cut, Reason::Truncated);
}

void test_district_not_found_skips_v1() {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.status = 404;
    http.v2.body = nvp_test::read_fixture("imsakiyem/v2_times_district_not_found.json");
    http.v1.body = v1_body();
    empty_store();
    const RunResult r = run(http, fs, input());
    TEST_ASSERT_FALSE(r.saved);
    TEST_ASSERT_EQUAL_INT(1, http.imsakiyem_requests());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::providers::FetchOutcome::NotFound), static_cast<int>(ims(r).result.outcome));
    TEST_ASSERT_EQUAL_size_t(0, fs.files.size());
}

void test_v2_network_error_falls_back_to_v1() {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.outcome = HttpOutcome::Tls;
    http.v1.body = v1_body();
    empty_store();
    const RunResult r = run(http, fs, input());
    TEST_ASSERT_TRUE(r.saved);
    TEST_ASSERT_EQUAL_INT(2, http.imsakiyem_requests());
    TEST_ASSERT_EQUAL_UINT16(30, g_store.count);
}

// Another status falls back to v1; 429 and 503 do not (spec 007 FR-011: a cool-down instead).
void test_v2_other_status_falls_back() {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.status = 500;
    http.v2.body = "<html>error</html>";
    http.v1.body = v1_body();
    empty_store();
    RunResult r = run(http, fs, input());
    TEST_ASSERT_TRUE(r.saved);
    TEST_ASSERT_EQUAL_INT(2, http.imsakiyem_requests());
    FakeHttp busy;
    busy.v2.status = 503;
    busy.v2.body = "<html>busy</html>";
    busy.v1.body = v1_body();
    empty_store();
    r = run(busy, fs, input());
    TEST_ASSERT_EQUAL_INT(1, busy.imsakiyem_requests());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::providers::FetchOutcome::RateLimited), static_cast<int>(ims(r).result.outcome));
}

void test_v1_wrong_district_rejected() {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.outcome = HttpOutcome::Timeout;
    http.v1.body = v1_body();
    empty_store(9541);
    const RunResult r = run(http, fs, input(9541));
    TEST_ASSERT_FALSE(r.saved);
    TEST_ASSERT_EQUAL_STRING(nvp::providers::reason_text(Reason::WrongDistrict), nvp::providers::reason_text(ims(r).result.reason));
    TEST_ASSERT_EQUAL_UINT16(0, g_store.count);
}

void test_save_failure_leaves_store() {
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.body = v2_body();
    empty_store();
    fs.fail_once_at(2);  // the open of the .new file (after the exists() check)
    const RunResult r = run(http, fs, input());
    TEST_ASSERT_FALSE(r.saved);
    TEST_ASSERT_TRUE(r.save != nvp::store::SaveResult::Ok);
    TEST_ASSERT_EQUAL_UINT16(0, g_store.count);
    TEST_ASSERT_FALSE(r.diyanet_stored);
}

void test_merge_keeps_yesterday() {  // a second refresh the next day keeps yesterday's record
    nvp_test::FakeFs fs;
    FakeHttp http;
    http.v2.body = v2_body();
    empty_store();
    run(http, fs, input());
    const RunResult r = run(http, fs, input(9206, Date{2026, 11, 26}, kNow + 2 * 86400));
    TEST_ASSERT_TRUE(r.saved);
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
