#include "nvp/providers/refresh.h"

#include <cstdio>
#include <cstring>

#include "nvp/providers/imsakiyem.h"

namespace nvp::providers {
namespace {

// 04-data-sources §3.1 and §11: the only host this flow contacts.
constexpr const char* kHost = "https://ezanvakti.imsakiyem.com";

// Feeds the body to the parser and keeps at most kMaxBatchRecords records.
class Collector : public HttpBody, public RecordSink {
public:
    Collector(ImsakiyemApi api, engine::DayRecord* out) : parser_(api, *this), out_(out) {}

    void on_status(int status) override { status_ = status; }
    bool on_chunk(const char* data, size_t n) override { return parser_.feed(data, n); }
    void on_record(const engine::DayRecord& r) override {
        if (kept_ < kMaxBatchRecords) out_[kept_++] = r;
    }

    void finish() { parser_.finish(); }
    const BatchInfo& info() const { return parser_.info(); }
    size_t kept() const { return kept_; }
    int status() const { return status_; }

private:
    ImsakiyemParser parser_;
    engine::DayRecord* out_;
    size_t kept_ = 0;
    int status_ = 0;
};

bool format_url(char* out, size_t cap, const char* fmt, uint32_t district, engine::Date a, engine::Date b) {
    int n = std::snprintf(out, cap, fmt, kHost, static_cast<unsigned long>(district), a.year, a.month, a.day, b.year,
                          b.month, b.day);
    return n > 0 && static_cast<size_t>(n) < cap;
}

engine::Date plus_days(engine::Date d, int32_t days) { return engine::to_date(engine::to_day_number(d) + days); }

// One provider attempt: fetch, parse while streaming, validate. Returns the records kept.
size_t attempt(HttpGet& http, ImsakiyemApi api, const RefreshRequest& req, engine::DayRecord* out, Attempt& a,
               bool& district_not_found) {
    char url[192];
    if (api == ImsakiyemApi::V2) {
        v2_url(req.district, req.today, url, sizeof url);
    } else {
        v1_url(req.district, req.today, url, sizeof url);
    }
    Collector c(api, out);
    a.tried = true;
    a.http = http.get(url, c);
    c.finish();
    a.records = c.info().records_seen;
    district_not_found = std::strcmp(c.info().error_code, "DISTRICT_NOT_FOUND") == 0;
    if (a.http.outcome != HttpOutcome::Done && a.http.outcome != HttpOutcome::Aborted) {
        a.reason = Reason::Network;
        return 0;
    }
    if (a.http.status != 200) {
        // A JSON error answer names its code (404 DISTRICT_NOT_FOUND); anything else is an HTTP failure.
        a.reason = c.info().error_code[0] != '\0' ? Reason::ErrorCode : Reason::Http;
        return 0;
    }
    a.reason = validate_batch(out, c.kept(), c.info(), req.district, req.today);
    return a.reason == Reason::Ok ? c.kept() : 0;
}

}  // namespace

const char* provider_text(Provider p) {
    switch (p) {
        case Provider::None: return "none";
        case Provider::ImsakiyemV2: return "imsakiyem v2";
        case Provider::ImsakiyemV1: return "imsakiyem v1";
    }
    return "?";
}

const char* http_outcome_text(HttpOutcome o) {
    switch (o) {
        case HttpOutcome::Done: return "done";
        case HttpOutcome::Refused: return "refused";
        case HttpOutcome::Connect: return "connect";
        case HttpOutcome::Tls: return "tls";
        case HttpOutcome::Timeout: return "timeout";
        case HttpOutcome::Aborted: return "aborted";
        case HttpOutcome::Error: return "error";
    }
    return "?";
}

bool v2_url(uint32_t district, engine::Date from, char* out, size_t cap) {
    return format_url(out, cap, "%s/api/v2/times?district=%lu&from=%04d-%02d-%02d&to=%04d-%02d-%02d", district, from,
                      plus_days(from, kV2Days));
}

bool v1_url(uint32_t district, engine::Date from, char* out, size_t cap) {
    return format_url(out, cap, "%s/api/prayer-times/%lu/range?startDate=%04d-%02d-%02d&endDate=%04d-%02d-%02d",
                      district, from, plus_days(from, kV1Days - 1));
}

RefreshResult refresh_with(HttpGet& http, store::FileSystem& fs, const RefreshRequest& req, store::Store& s,
                           RefreshScratch& scratch, SaveGate* gate) {
    RefreshResult result;
    bool not_found = false;

    result.provider = Provider::ImsakiyemV2;
    size_t n = attempt(http, ImsakiyemApi::V2, req, scratch.incoming, result.v2, not_found);
    result.reason = result.v2.reason;
    if (result.v2.reason != Reason::Ok && !not_found) {
        bool unused = false;
        result.provider = Provider::ImsakiyemV1;
        n = attempt(http, ImsakiyemApi::V1, req, scratch.incoming, result.v1, unused);
        result.reason = result.v1.reason;
    }
    if (result.reason != Reason::Ok) return result;  // rejected or failed: nothing changes (FR-007)

    // Merge into a copy; the caller's store changes only once the file is safely replaced.
    scratch.merged = s;
    scratch.merged.district_id = req.district;
    result.merge = store::merge(scratch.merged, scratch.incoming, n, req.now_utc, req.today);
    if (gate != nullptr) gate->before_save();
    result.save = store::save(fs, scratch.merged);
    if (result.save != store::SaveResult::Ok) return result;
    s = scratch.merged;
    result.stored = true;
    return result;
}

}  // namespace nvp::providers
