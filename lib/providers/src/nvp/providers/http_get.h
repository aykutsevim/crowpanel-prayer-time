// The abstract HTTP fetch the refresh flow talks to (spec 006 research R3). Implemented on the
// device by src/platform/https_fetch (esp_http_client, CA bundle, certificate dates) and in the
// host tests by a fake that returns recorded answers. Pure C++.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvp::providers {

class HttpBody {
public:
    virtual ~HttpBody() = default;
    // The HTTP status, once, before the body.
    virtual void on_status(int status) = 0;
    // The next body bytes. Return false to stop reading (the outcome is then Aborted).
    virtual bool on_chunk(const char* data, size_t n) = 0;
};

enum class HttpOutcome : uint8_t {
    Done,     // a status and the whole body were read
    Refused,  // not started: the clock is not valid, or another request is running
    Connect,  // DNS or TCP failed
    Tls,      // the TLS handshake or a certificate check failed
    Timeout,
    Aborted,  // the body callback asked to stop
    Error,    // anything else
};

const char* http_outcome_text(HttpOutcome o);

struct HttpResult {
    HttpOutcome outcome = HttpOutcome::Error;
    int status = 0;      // set once a status was received
    uint32_t bytes = 0;  // body bytes delivered
};

class HttpGet {
public:
    virtual ~HttpGet() = default;
    virtual HttpResult get(const char* url, HttpBody& body) = 0;
};

}  // namespace nvp::providers
