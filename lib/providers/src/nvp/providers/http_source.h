// The Source over an HttpGet: URLs, streaming parsers and validation for İmsakiyem (v2, then v1),
// emushaf and Aladhan (spec 007 T030; FR-009, FR-010, research R1, R2; 04-data-sources §3, §5.3,
// §11). Pure C++; the device gives it the HTTPS client, the tests a fake.
//
// Failure mapping: no answer -> Failed (Network); HTTP 429 or 503 -> RateLimited; İmsakiyem 404
// DISTRICT_NOT_FOUND -> NotFound (v1 is then not tried); another status -> Failed (Http); a body over
// 512 kB -> Failed (Malformed); a batch that does not validate -> Failed with its reason.
#pragma once
#include <cstddef>
#include <cstdint>

#include "nvp/providers/http_get.h"
#include "nvp/providers/source.h"

namespace nvp::providers {

inline constexpr uint32_t kMaxBodyBytes = 512u * 1024u;

class HttpSource : public Source {
public:
    explicit HttpSource(HttpGet& http) : http_(http) {}
    FetchResult fetch(const FetchRequest& req, engine::DayRecord* out, size_t cap) override;
    const char* last_url() const { return url_; }

private:
    FetchResult imsakiyem(const FetchRequest& req, engine::DayRecord* out, size_t cap);
    FetchResult emushaf(const FetchRequest& req, engine::DayRecord* out, size_t cap);
    FetchResult aladhan(const FetchRequest& req, engine::DayRecord* out, size_t cap);

    HttpGet& http_;
    char url_[224] = {};
};

}  // namespace nvp::providers
