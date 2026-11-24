#include "nvp/diag/wifi_states.h"

#include <cstring>

namespace nvp::diag {
namespace {

bool in_setup(WifiState s) { return s == WifiState::SetupAP || s == WifiState::Trying; }

WifiStep stay(WifiState s) { return {s, kActNone}; }

}  // namespace

WifiStep wifi_step(WifiState current, WifiState previous, WifiEvent event, const WifiContext& ctx) {
    const bool can_join = ctx.silence_ok && ctx.has_stored;
    switch (event) {
        case WifiEvent::Boot:
            if (current == WifiState::Idle && can_join) return {WifiState::Joining, kActStartJoin};
            return stay(current);
        case WifiEvent::JoinOk:
            if (current == WifiState::Joining) return {WifiState::Connected, kActResetBackoff};
            return stay(current);
        case WifiEvent::JoinFail:
            if (current == WifiState::Joining) return {WifiState::Backoff, kActStartBackoff};
            return stay(current);
        case WifiEvent::LinkLost:
            if (current == WifiState::Connected) return {WifiState::Backoff, kActStartBackoff};
            return stay(current);
        case WifiEvent::BackoffElapsed:
            if (current != WifiState::Backoff) return stay(current);
            if (can_join) return {WifiState::Joining, kActStartJoin};
            return {WifiState::Idle, kActNone};
        case WifiEvent::SetupConfirmed:
            if (in_setup(current)) return stay(current);
            if (!ctx.silence_ok) return {current, kActRefused};
            return {WifiState::SetupAP, kActStartAp};
        case WifiEvent::Submit:
            if (current == WifiState::SetupAP) return {WifiState::Trying, kActTrySubmitted};
            return stay(current);
        case WifiEvent::TryOk:
            if (current == WifiState::Trying) {
                return {WifiState::Connected, kActStoreSubmitted | kActStopApAfterGrace | kActResetBackoff};
            }
            return stay(current);
        case WifiEvent::TryFail:
            if (current == WifiState::Trying) return {WifiState::SetupAP, kActDiscardSubmitted};
            return stay(current);
        case WifiEvent::Cancel:
            if (!in_setup(current)) return stay(current);
            if (previous != WifiState::Idle && can_join) {
                return {WifiState::Joining, kActStopAp | kActDiscardSubmitted | kActStartJoin};
            }
            return {WifiState::Idle, kActStopAp | kActDiscardSubmitted};
        case WifiEvent::Forget:
            if (in_setup(current)) return stay(current);
            return {WifiState::Idle, kActEraseStored | kActDisconnect | kActResetBackoff};
    }
    return stay(current);
}

bool valid_credentials(const char* ssid, const char* password) {
    if (ssid == nullptr || password == nullptr) return false;
    const size_t s = std::strlen(ssid);
    const size_t p = std::strlen(password);
    return s >= 1 && s <= 32 && p <= 63;
}

}  // namespace nvp::diag
