// One encrypted request for stress mode (research R10; FR-010, SEC-01, SEC-02, SEC-03).
// Certificate verification is always on: esp_http_client with the built-in CA bundle
// (esp_crt_bundle_attach; tools/check_tls.py enforces it). Call from the network worker only.
#pragma once

#include <cstdint>

namespace platform::https {

struct Result {
    bool ok = false;         // HTTP 200 with a body of 4 kB or less
    int status = 0;          // HTTP status, or a negative client error
    uint32_t bytes = 0;
    uint32_t duration_ms = 0;
    char reason[48] = {};    // short, for the counters and the log
};

// GET https://ezanvakti.imsakiyem.com/api/v2/locations/countries/2 (one allowed host, §11).
// Certificate dates are checked (spec 005 OD5-1), so it refuses with "time not valid" while the
// app's clock is not valid; the worker passes that flag in (no app header here, R3).
Result probe(bool time_valid);

}  // namespace platform::https
