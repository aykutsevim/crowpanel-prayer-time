// Classifying a failed connection attempt from the station's disconnect reasons (NET-05; spec 004
// FR-016, research R4). Reason numbers are ESP-IDF 5.5.5's wifi_err_reason_t; src/platform passes
// them through raw, so this file needs no SDK header.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvp::net {

enum class FailureClass : uint8_t { None, WrongPassword, NotFound, Other };

// `reasons` in the order they arrived during the 20 s window (may be empty: a timeout with no event).
// `seen_in_scan`: the network name was in the last scan.
FailureClass classify_failure(const uint16_t* reasons, size_t count, bool seen_in_scan);

const char* failure_name(FailureClass c);  // "none", "wrong_password", "not_found", "other"

}  // namespace nvp::net
