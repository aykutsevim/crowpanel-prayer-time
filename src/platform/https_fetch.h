// nvp::providers::HttpGet on esp_http_client (spec 006 T016, research R3; FR-002, FR-003, SEC-01..SEC-03).
//
//   - certificate verification on: the built-in CA bundle (esp_crt_bundle_attach), with dates
//     (CONFIG_MBEDTLS_HAVE_TIME_DATE, OD5-1); tools/check_tls.py enforces it;
//   - User-Agent nvp::kUserAgent, Accept: application/json, Accept-Encoding: identity; no cookies;
//   - 15 s timeout, redirects not followed;
//   - the body is read in 1 kB chunks and passed on at once (esp_http_client_read decodes chunked
//     transfer, ESP-IDF 5.5.5 esp_http_client.c), so the answer is never held whole;
//   - refuses (outcome Refused) while the caller says the clock is not valid.
// Call from the network worker only (04 §10: one request at a time). Also records the lowest free
// internal heap seen during the request, for the log (R3 ⚠).
#pragma once

#include <cstdint>

#include "nvp/providers/http_get.h"

namespace platform::https_fetch {

class Client : public nvp::providers::HttpGet {
public:
    explicit Client(bool time_valid) : time_valid_(time_valid) {}
    nvp::providers::HttpResult get(const char* url, nvp::providers::HttpBody& body) override;

    uint32_t heap_min_internal() const { return heap_min_; }  // bytes, over every request of this client
    const char* last_error() const { return error_; }          // esp_err name of the last failure, or ""

private:
    bool time_valid_;
    uint32_t heap_min_ = UINT32_MAX;
    const char* error_ = "";
};

}  // namespace platform::https_fetch
