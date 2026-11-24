// Wi-Fi state machine of the diagnostic mode, as a pure transition function (data-model.md
// WifiState; FR-001a, FR-019..FR-023, MODE-04, NET-01). The platform runs the actions.
#pragma once

#include <cstdint>

namespace nvp::diag {

enum class WifiState : uint8_t { Idle, Joining, Connected, Backoff, SetupAP, Trying };

enum class WifiEvent : uint8_t {
    Boot,            // start-up: join if details are stored
    JoinOk,
    JoinFail,
    LinkLost,
    BackoffElapsed,
    SetupConfirmed,  // "Start Wi-Fi setup" confirmed on screen, or `wifi setup`
    Submit,          // the setup page sent details
    TryOk,
    TryFail,
    Cancel,          // "Cancel setup" or `wifi cancel`
    Forget,          // "Forget Wi-Fi" confirmed on screen, or `wifi forget`
};

// Bit flags: what the platform must do for a transition.
using WifiAction = uint32_t;
inline constexpr WifiAction kActNone = 0;
inline constexpr WifiAction kActStartJoin = 1u << 0;        // join with the stored details
inline constexpr WifiAction kActStartBackoff = 1u << 1;     // wait Backoff::next_delay_ms()
inline constexpr WifiAction kActResetBackoff = 1u << 2;
inline constexpr WifiAction kActStartAp = 1u << 3;
inline constexpr WifiAction kActStopAp = 1u << 4;
inline constexpr WifiAction kActStopApAfterGrace = 1u << 5;  // 5 s grace (contracts/setup-portal.md)
inline constexpr WifiAction kActTrySubmitted = 1u << 6;      // try the submitted details, up to 20 s
inline constexpr WifiAction kActStoreSubmitted = 1u << 7;
inline constexpr WifiAction kActDiscardSubmitted = 1u << 8;
inline constexpr WifiAction kActEraseStored = 1u << 9;
inline constexpr WifiAction kActDisconnect = 1u << 10;
inline constexpr WifiAction kActRefused = 1u << 11;          // silence not acknowledged (FR-001a)

struct WifiContext {
    bool silence_ok = false;  // the silence step was acknowledged
    bool has_stored = false;  // network details are stored on the device
};

struct WifiStep {
    WifiState next;
    WifiAction action;
};

// `previous` is the state before the hotspot was opened; Cancel returns towards it.
WifiStep wifi_step(WifiState current, WifiState previous, WifiEvent event, const WifiContext& ctx);

// Network name 1..32 bytes, password 0..63 bytes (empty for an open network).
bool valid_credentials(const char* ssid, const char* password);

}  // namespace nvp::diag
