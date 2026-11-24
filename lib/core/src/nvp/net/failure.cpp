#include "nvp/net/failure.h"

namespace nvp::net {
namespace {

// 2 AUTH_EXPIRE, 14 MIC_FAILURE, 15 4WAY_HANDSHAKE_TIMEOUT, 202 AUTH_FAIL, 204 HANDSHAKE_TIMEOUT.
bool password_reason(uint16_t r) { return r == 2 || r == 14 || r == 15 || r == 202 || r == 204; }

// 201 NO_AP_FOUND, 210..212 NO_AP_FOUND_* (compatible security, auth-mode threshold, RSSI threshold).
bool not_found_reason(uint16_t r) { return r == 201 || (r >= 210 && r <= 212); }

}  // namespace

FailureClass classify_failure(const uint16_t* reasons, size_t count, bool seen_in_scan) {
    if (reasons == nullptr || count == 0) return FailureClass::Other;
    const uint16_t last = reasons[count - 1];
    if (not_found_reason(last)) return FailureClass::NotFound;
    if (password_reason(last) && seen_in_scan) {
        size_t n = 0;
        for (size_t i = 0; i < count; ++i) n += password_reason(reasons[i]) ? 1 : 0;
        if (n >= 2) return FailureClass::WrongPassword;  // once alone may be a weak signal (R4)
    }
    return FailureClass::Other;
}

const char* failure_name(FailureClass c) {
    switch (c) {
        case FailureClass::None: return "none";
        case FailureClass::WrongPassword: return "wrong_password";
        case FailureClass::NotFound: return "not_found";
        case FailureClass::Other: return "other";
    }
    return "other";
}

}  // namespace nvp::net
