// The HTTP-backed source on the recorded answers (spec 007 T030; FR-009, FR-010, research R1, R2).
#include <unity.h>

#include <map>
#include <string>
#include <vector>

#include "../support/fake_fs.h"
#include "../support/fixture_file.h"
#include "nvp/providers/http_source.h"
#include "nvp/providers/runner.h"

using nvp::engine::Date;
using nvp::engine::DayRecord;
using nvp::providers::FetchOutcome;
using nvp::providers::FetchRequest;
using nvp::providers::FetchResult;
using nvp::providers::HttpBody;
using nvp::providers::HttpOutcome;
using nvp::providers::HttpResult;
using nvp::providers::ProviderId;
using nvp::providers::Reason;

namespace {

const Date kToday{2026, 11, 24};

struct Answer {
    HttpOutcome outcome = HttpOutcome::Done;
    int status = 200;
    std::string body;
};

// Answers by the first registered URL fragment the request contains.
struct FakeHttp : nvp::providers::HttpGet {
    std::vector<std::pair<std::string, Answer>> routes;
    std::vector<std::string> urls;
    void route(const std::string& fragment, Answer a) { routes.push_back({fragment, a}); }
    HttpResult get(const char* url, HttpBody& body) override {
        urls.push_back(url);
        HttpResult r;
        for (const auto& p : routes) {
            if (std::string(url).find(p.first) == std::string::npos) continue;
            const Answer& a = p.second;
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
        r.outcome = HttpOutcome::Connect;
        return r;
    }
};

Answer ok(const char* fixture, int status = 200) {
    Answer a;
    a.status = status;
    a.body = nvp_test::read_fixture(fixture);
    return a;
}

FetchRequest request(ProviderId p) {
    FetchRequest r;
    r.provider = p;
    r.district = 9206;
    r.today = kToday;
    r.from = kToday;
    r.to = nvp::engine::to_date(nvp::engine::to_day_number(kToday) + 365);
    r.has_coords = true;
    r.lat_e5 = 3992063;
    r.lon_e5 = 3285403;
    return r;
}

std::vector<DayRecord> g_out(400);

void test_imsakiyem_v2() {
    FakeHttp http;
    http.route("/api/v2/times", ok("imsakiyem/v2_times_9206_2026-11-24_2026-12-31.json"));
    nvp::providers::HttpSource src(http);
    const FetchResult r = src.fetch(request(ProviderId::Imsakiyem), g_out.data(), g_out.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FetchOutcome::Ok), static_cast<int>(r.outcome));
    TEST_ASSERT_EQUAL_UINT32(38, r.count);
    TEST_ASSERT_EQUAL_UINT8(1, r.requests);
}

void test_imsakiyem_v1_fallback_and_lite() {
    FakeHttp http;
    Answer down;
    down.outcome = HttpOutcome::Timeout;
    http.route("/api/v2/times", down);
    http.route("/api/prayer-times/9206/range", ok("imsakiyem/v1_range_9206_2026-11-24_2026-12-23.json"));
    nvp::providers::HttpSource src(http);
    FetchResult r = src.fetch(request(ProviderId::Imsakiyem), g_out.data(), g_out.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FetchOutcome::Ok), static_cast<int>(r.outcome));
    TEST_ASSERT_EQUAL_UINT8(2, r.requests);
    TEST_ASSERT_EQUAL_UINT32(30, r.count);
    FetchRequest lite = request(ProviderId::Imsakiyem);
    lite.allow_fallback = false;
    r = src.fetch(lite, g_out.data(), g_out.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FetchOutcome::Failed), static_cast<int>(r.outcome));
    TEST_ASSERT_EQUAL_UINT8(1, r.requests);
}

void test_imsakiyem_not_found_and_rate_limited() {
    FakeHttp http;
    http.route("/api/v2/times", ok("imsakiyem/v2_times_district_not_found.json", 404));
    nvp::providers::HttpSource src(http);
    FetchResult r = src.fetch(request(ProviderId::Imsakiyem), g_out.data(), g_out.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FetchOutcome::NotFound), static_cast<int>(r.outcome));
    TEST_ASSERT_EQUAL_UINT32(1, http.urls.size());  // v1 not tried
    FakeHttp busy;
    Answer a;
    a.status = 429;
    a.body = "{\"success\":false}";
    busy.route("/api/v2/times", a);
    nvp::providers::HttpSource src2(busy);
    r = src2.fetch(request(ProviderId::Imsakiyem), g_out.data(), g_out.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FetchOutcome::RateLimited), static_cast<int>(r.outcome));
    TEST_ASSERT_EQUAL_UINT32(1, busy.urls.size());
}

void test_emushaf() {
    FakeHttp http;
    http.route("ezanvakti.emushaf.net/vakitler/9206", ok("emushaf/vakitler_9206_2026-11-24.json"));
    nvp::providers::HttpSource src(http);
    FetchRequest req = request(ProviderId::Emushaf);
    const FetchResult r = src.fetch(req, g_out.data(), g_out.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FetchOutcome::Ok), static_cast<int>(r.outcome));
    TEST_ASSERT_EQUAL_UINT32(28, r.count);  // 23.11 .. 20.12
    TEST_ASSERT_EQUAL_STRING("https://ezanvakti.emushaf.net/vakitler/9206", src.last_url());
}

void test_emushaf_502_page() {
    FakeHttp http;
    http.route("vakitler", ok("emushaf/vakitler_999999_502.txt", 502));
    nvp::providers::HttpSource src(http);
    const FetchResult r = src.fetch(request(ProviderId::Emushaf), g_out.data(), g_out.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FetchOutcome::Failed), static_cast<int>(r.outcome));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Reason::Http), static_cast<int>(r.reason));
}

void test_aladhan_month() {
    FakeHttp http;
    http.route("calendar/2026/11", ok("aladhan/calendar_2026_11_9206.json"));
    nvp::providers::HttpSource src(http);
    FetchRequest req = request(ProviderId::Aladhan);
    req.from = Date{2026, 11, 1};
    req.to = Date{2026, 11, 30};
    const FetchResult r = src.fetch(req, g_out.data(), g_out.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FetchOutcome::Ok), static_cast<int>(r.outcome));
    TEST_ASSERT_EQUAL_UINT32(8, r.count);  // 23.11 .. 30.11
    TEST_ASSERT_TRUE(std::string(src.last_url()).find("latitude=39.92063&longitude=32.85403&method=13") !=
                     std::string::npos);
}

void test_body_over_512_kb() {
    FakeHttp http;
    Answer a;
    a.body = "[" + std::string(600 * 1024, ' ') + "]";
    http.route("vakitler", a);
    nvp::providers::HttpSource src(http);
    const FetchResult r = src.fetch(request(ProviderId::Emushaf), g_out.data(), g_out.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FetchOutcome::Failed), static_cast<int>(r.outcome));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Reason::Malformed), static_cast<int>(r.reason));
}

// End to end: the runner over the HTTP source, İmsakiyem down, emushaf answering.
nvp::providers::RunScratch g_scratch;
nvp::store::Store g_store;

void test_runner_over_http() {
    FakeHttp http;
    Answer down;
    down.outcome = HttpOutcome::Connect;
    http.route("ezanvakti.imsakiyem.com", down);
    http.route("ezanvakti.emushaf.net/vakitler/9206", ok("emushaf/vakitler_9206_2026-11-24.json"));
    nvp::providers::HttpSource src(http);
    nvp_test::FakeFs fs;
    nvp::store::RefreshState state;
    g_store = nvp::store::Store{};
    g_store.district_id = 9206;
    nvp::providers::RunInput in;
    in.district = 9206;
    in.today = kToday;
    in.now_utc = 1795524049;
    in.has_coords = true;
    in.lat_e5 = 3992063;
    in.lon_e5 = 3285403;
    const nvp::providers::RunResult r = nvp::providers::refresh_run(src, fs, in, g_store, g_scratch, state);
    TEST_ASSERT_TRUE(r.saved);
    TEST_ASSERT_TRUE(r.diyanet_stored);
    TEST_ASSERT_EQUAL_UINT8(3, r.requests);  // v2, v1, emushaf
    TEST_ASSERT_TRUE(r.providers[static_cast<size_t>(ProviderId::Emushaf)].plausibility.checked);
    TEST_ASSERT_TRUE(r.providers[static_cast<size_t>(ProviderId::Emushaf)].plausibility.ok);
    TEST_ASSERT_EQUAL_INT(26, r.days_ahead);  // 24.11 .. 20.12
    TEST_ASSERT_EQUAL_INT(static_cast<int>(nvp::store::RefreshOutcome::Partial), static_cast<int>(r.outcome));
}

}  // namespace

void run_http_source_tests() {
    RUN_TEST(test_imsakiyem_v2);
    RUN_TEST(test_imsakiyem_v1_fallback_and_lite);
    RUN_TEST(test_imsakiyem_not_found_and_rate_limited);
    RUN_TEST(test_emushaf);
    RUN_TEST(test_emushaf_502_page);
    RUN_TEST(test_aladhan_month);
    RUN_TEST(test_body_over_512_kb);
    RUN_TEST(test_runner_over_http);
}
