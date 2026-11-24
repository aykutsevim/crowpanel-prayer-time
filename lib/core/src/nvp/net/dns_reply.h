// The captive portal's DNS answer (NET-03; spec 004 FR-011, contracts/portal-http.md, research R1, R14).
// Pure wire-format work; src/platform/dns_responder owns the socket.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvp::net {

// Builds the answer to one DNS query in `q`:
//   A query     -> one A record `ip`, TTL 0;
//   other types -> NOERROR with no answers.
// Returns 0 (send nothing) for responses, more than one question, malformed or truncated
// packets, compression pointers in the question, or when `cap` is too small. The answer is never
// longer than the query plus 16 bytes (no amplification).
size_t dns_reply(const uint8_t* q, size_t n, const uint8_t ip[4], uint8_t* out, size_t cap);

}  // namespace nvp::net
