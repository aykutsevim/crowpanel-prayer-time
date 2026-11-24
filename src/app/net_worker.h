// Network worker for the diagnostic mode (Stage 3 of spec 001): Wi-Fi join and setup, the setup
// page, and stress mode. It runs on core 0 (research R8); the screen and serial console on core 1
// talk to it only through requests and a lock-protected snapshot. No SDK headers here
// (constitution VI): it drives src/platform through its interfaces.
#pragma once

#include "nvp/diag/report.h"
#include "nvp/diag/stress.h"
#include "nvp/diag/wifi_states.h"

namespace app::net {

struct Snapshot {
    nvp::diag::WifiState state = nvp::diag::WifiState::Idle;
    nvp::diag::WifiLine line;        // for the report and the screens: names and address only
    bool hotspot_open = false;
    char hotspot_password[9] = {};   // device screen only (FR-020, FR-024)
    char setup_message[64] = {};
    bool scan_completed = false;     // at least one Wi-Fi scan finished
    bool stress_running = false;
    bool stress_paused = false;
    bool stress_has_run = false;
    nvp::diag::StressCounters stress;
};

// Starts the worker. With silence not acknowledged it never starts the radio (FR-001a).
void start(bool silence_ok);

// Requests from the screen or the serial console. Each returns false when refused.
bool request_setup();    // open the hotspot (after the on-screen confirmation)
void request_cancel();   // close the hotspot, keep the stored details
void request_forget();   // erase the stored details and disconnect
bool request_stress(bool on);

// Diagnostic one-shots on the network core (stack decision log, 2026-11-23).
void request_test_write();
bool request_test_scan();   // refused while silence is not acknowledged (radio)
bool request_test_https();  // same
bool request_test_mix();    // same: a scan plus a write at its start

Snapshot snapshot();

}  // namespace app::net
