// A submission of the setup page: POST /api/setup (NET-04; spec 004 FR-012, contracts/portal-http.md).
// Pure C++, no heap. The password is held here only until the attempt ends (wipe()).
#pragma once

#include <cstddef>
#include <cstdint>

#include "nvp/net/places.h"

namespace nvp::net {

inline constexpr size_t kMaxSubmissionBody = 512;

struct Submission {
    char ssid[33] = {};   // 1..32 bytes, any bytes but NUL
    char pass[64] = {};   // empty (open network) or 8..63 bytes; never logged or served (NET-08)
    Location location;
};

enum class SubmitError : uint8_t { Ok, Ssid, Password, Location };

// Decodes and bounds-checks a form body. It does not check the location against the place list;
// the caller does that with place_lookup(). `out` is wiped first.
SubmitError parse_submission(const char* body, size_t len, Submission& out);

// "ssid", "password", "location" (the error field of a 400 answer); "" for Ok.
const char* submit_error_name(SubmitError e);

// Overwrites every byte, so no password stays in memory after an attempt.
void wipe(Submission& s);

}  // namespace nvp::net
