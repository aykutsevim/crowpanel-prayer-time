// Network worker: the device's modes (setup, connecting, running; 03 §1), Wi-Fi, the device's own
// network with DNS and portal, and the diagnostic stress mode (spec 004 FR-001..FR-020, spec 001
// Stage 3). It runs on core 0 (spec 001 research R8); the screens and serial console on core 1 talk
// to it only through requests and a lock-protected snapshot. No SDK headers here (constitution VI):
// it drives src/platform through its interfaces and nvp::net::mode_step for every decision.
#pragma once

#include "nvp/diag/report.h"
#include "nvp/diag/stress.h"
#include "nvp/net/failure.h"
#include "nvp/net/mode.h"
#include "nvp/net/places.h"

namespace app::net {

struct Snapshot {
    // --- Product (spec 004) ---
    nvp::net::Mode mode = nvp::net::Mode::Start;
    uint8_t phones = 0;                                   // joined to the device's network
    nvp::net::FailureClass failure = nvp::net::FailureClass::None;  // last attempt, until the next
    char ap_ssid[33] = {};
    char attempt_ssid[33] = {};                           // the submitted network name (no password)
    bool has_location = false;
    nvp::net::Location location;
    bool wifi_up = false;
    char ssid[33] = {};                                   // the stored network
    int8_t rssi = 0;
    char ip[16] = {};
    // --- Diagnostic screens and report (spec 001) ---
    nvp::diag::WifiLine line;        // names and address only
    bool hotspot_open = false;
    char hotspot_password[9] = {};   // device screen only (FR-023)
    char setup_message[64] = {};
    bool scan_completed = false;     // at least one Wi-Fi scan finished
    bool stress_running = false;
    bool stress_paused = false;
    bool stress_has_run = false;
    nvp::diag::StressCounters stress;
    // --- Spec 006 (FR-013, T022): how long Wi-Fi has been down in the running mode; 0 when up ---
    uint32_t wifi_down_ms = 0;
};

// Starts the worker. With the silence step failed it never starts the radio (spec 001 FR-001a).
void start(bool silence_ok);

// Requests from the screens or the serial console. Each returns false when refused.
bool request_setup();       // `wifi setup`, diagnostic button: DevSetup (stored details kept)
void request_cancel();      // `wifi cancel`: DevCancel
void request_start_over();  // "Baştan kur", `wifi forget`: StartOver (MODE-06)
inline void request_forget() { request_start_over(); }
bool request_stress(bool on);

// Diagnostic one-shots on the network core (spec 001 stack decision log, 2026-11-23).
void request_test_write();
bool request_test_scan();   // refused while silence is not acknowledged (radio)
bool request_test_https();  // same
bool request_test_mix();    // same: a scan plus a write at its start

// Spec 005 (research R1, R2, R11).
bool request_time_sync();   // ask SNTP now; false when not connected
void request_ntp(bool on);  // `ntp on|off`; off also restarts the fallback timer

Snapshot snapshot();

}  // namespace app::net
