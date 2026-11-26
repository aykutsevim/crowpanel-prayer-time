#include "nvp/providers/http_source.h"

#include <cstdio>
#include <cstring>

#include "nvp/providers/aladhan.h"
#include "nvp/providers/emushaf.h"
#include "nvp/providers/imsakiyem.h"
#include "nvp/providers/refresh.h"
#include "nvp/providers/validate.h"

namespace nvp::providers {
namespace {

engine::Date plus_days(engine::Date d, int32_t n) { return engine::to_date(engine::to_day_number(d) + n); }

// Feeds the body to a parser and keeps at most `cap` records; stops a body over 512 kB.
template <typename Parser>
class Collector : public HttpBody, public RecordSink {
public:
    Collector(engine::DayRecord* out, size_t cap) : out_(out), cap_(cap) {}
    Parser* parser = nullptr;

    void on_status(int status) override { status_ = status; }
    bool on_chunk(const char* data, size_t n) override {
        bytes_ += static_cast<uint32_t>(n);
        if (bytes_ > kMaxBodyBytes) {
            too_big_ = true;
            return false;
        }
        return parser->feed(data, n);
    }
    void on_record(const engine::DayRecord& r) override {
        if (kept_ < cap_) out_[kept_++] = r;
    }
    size_t kept() const { return kept_; }
    bool too_big() const { return too_big_; }

private:
    engine::DayRecord* out_;
    size_t cap_;
    size_t kept_ = 0;
    int status_ = 0;
    uint32_t bytes_ = 0;
    bool too_big_ = false;
};

// Common tail: transport, status, size, validation.
template <typename C>
FetchResult finish(const HttpResult& h, const C& c, const BatchInfo& info, const FetchRequest& req,
                   engine::DayRecord* out) {
    FetchResult r;
    r.requests = 1;
    r.http_status = h.status;
    if (c.too_big()) {
        r.reason = Reason::Malformed;
        return r;
    }
    if (h.outcome != HttpOutcome::Done && h.outcome != HttpOutcome::Aborted) {
        r.reason = Reason::Network;
        return r;
    }
    if (h.status == 429 || h.status == 503) {
        r.outcome = FetchOutcome::RateLimited;
        r.reason = Reason::Http;
        return r;
    }
    if (h.status != 200) {
        if (std::strcmp(info.error_code, "DISTRICT_NOT_FOUND") == 0) {
            r.outcome = FetchOutcome::NotFound;
            r.reason = Reason::ErrorCode;
        } else {
            r.reason = info.error_code[0] != '\0' ? Reason::ErrorCode : Reason::Http;
        }
        return r;
    }
    r.reason = validate_batch(out, c.kept(), info, req.district, req.today);
    if (r.reason == Reason::Ok) {
        r.outcome = FetchOutcome::Ok;
        r.count = c.kept();
    }
    return r;
}

}  // namespace

FetchResult HttpSource::fetch(const FetchRequest& req, engine::DayRecord* out, size_t cap) {
    switch (req.provider) {
        case ProviderId::Imsakiyem: return imsakiyem(req, out, cap);
        case ProviderId::Emushaf: return emushaf(req, out, cap);
        case ProviderId::Aladhan: return aladhan(req, out, cap);
        default: break;
    }
    FetchResult r;
    r.reason = Reason::Network;
    return r;
}

FetchResult HttpSource::imsakiyem(const FetchRequest& req, engine::DayRecord* out, size_t cap) {
    uint8_t requests = 0;
    FetchResult r;
    for (int api = 0; api < 2; ++api) {
        const ImsakiyemApi which = api == 0 ? ImsakiyemApi::V2 : ImsakiyemApi::V1;
        if (which == ImsakiyemApi::V2) {
            v2_url(req.district, req.from, url_, sizeof url_);
        } else {
            v1_url(req.district, req.from, url_, sizeof url_);
        }
        Collector<ImsakiyemParser> c(out, cap);
        ImsakiyemParser p(which, c);
        c.parser = &p;
        const HttpResult h = http_.get(url_, c);
        p.finish();
        r = finish(h, c, p.info(), req, out);
        ++requests;
        r.requests = requests;
        // v1 only after a v2 failure that is not "district not found" or "slow down" (spec 006).
        if (r.outcome == FetchOutcome::Ok || r.outcome == FetchOutcome::NotFound ||
            r.outcome == FetchOutcome::RateLimited || !req.allow_fallback)
            break;
    }
    return r;
}

FetchResult HttpSource::emushaf(const FetchRequest& req, engine::DayRecord* out, size_t cap) {
    std::snprintf(url_, sizeof url_, "https://ezanvakti.emushaf.net/vakitler/%lu", static_cast<unsigned long>(req.district));
    Collector<EmushafParser> c(out, cap);
    EmushafParser p(c, plus_days(req.today, -1));
    c.parser = &p;
    const HttpResult h = http_.get(url_, c);
    p.finish();
    return finish(h, c, p.info(), req, out);
}

FetchResult HttpSource::aladhan(const FetchRequest& req, engine::DayRecord* out, size_t cap) {
    if (!req.has_coords) {
        FetchResult r;
        r.reason = Reason::Network;
        r.requests = 0;
        return r;
    }
    aladhan_url(req.from.year, req.from.month, req.lat_e5, req.lon_e5, url_, sizeof url_);
    const engine::Date yesterday = plus_days(req.today, -1);
    const engine::Date from =
        engine::to_day_number(req.from) < engine::to_day_number(yesterday) ? yesterday : req.from;
    Collector<AladhanParser> c(out, cap);
    AladhanParser p(c, from, req.to);
    c.parser = &p;
    const HttpResult h = http_.get(url_, c);
    p.finish();
    return finish(h, c, p.info(), req, out);
}

}  // namespace nvp::providers
