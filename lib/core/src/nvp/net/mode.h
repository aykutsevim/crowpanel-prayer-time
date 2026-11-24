// The device's mode machine: setup, connecting and running (03-functional-spec.md §1, MODE-02..MODE-06,
// NET-01; spec 004 FR-001..FR-003, data-model.md, research R9). It replaces the diagnostic Wi-Fi
// machine of Slice 1. A pure, table-driven transition function: src/app delivers events (timers
// included) and performs the returned actions. Pure C++, no SDK headers (constitution VI).
#pragma once

#include <cstdint>

namespace nvp::net {

enum class Mode : uint8_t {
    Start,       // before Boot
    Setup,       // device network up, waiting for the page (no timeout, MODE-03)
    Connecting,  // trying the submitted details, device network still up (MODE-04)
    Grace,       // connected; device network closes after 5 s
    RunJoining,  // running: joining the stored network
    RunUp,       // running: connected
    RunBackoff,  // running: waiting before the next join (5 s .. 5 min, NET-01)
    RunNoRadio,  // running without radio: the silence step failed (spec 001 FR-001a)
};

enum class Event : uint8_t {
    Boot,
    PhoneJoined,
    PhoneLeft,
    SubmitValid,
    SubmitInvalid,
    TryStart,      // 1 s after answering 202 (research R2)
    TryOk,         // the station got an address
    TryFail,       // the attempt failed before 20 s
    TryTimeout,    // 20 s passed
    GraceElapsed,  // 5 s after success
    JoinOk,
    JoinFail,
    LinkLost,
    BackoffElapsed,
    StartOver,  // "Baştan kur" or `wifi forget` (MODE-06)
    DevSetup,   // `wifi setup` or the diagnostic button
    DevCancel,  // `wifi cancel`
};

struct Context {
    bool silence_ok = false;    // the silence step succeeded (spec 001 FR-001a)
    bool has_wifi = false;      // Wi-Fi details stored
    bool has_location = false;  // location stored
};

// Bit flags, in the order the platform performs them.
using Action = uint32_t;
inline constexpr Action kActNone = 0;
inline constexpr Action kActEraseWifi = 1u << 0;         // erase wifi/ssid and wifi/pass only
inline constexpr Action kActDisconnect = 1u << 1;        // station off
inline constexpr Action kActDiscardSubmitted = 1u << 2;  // zero the submission
inline constexpr Action kActScan = 1u << 3;              // one scan, before the AP starts (R3)
inline constexpr Action kActStartAp = 1u << 4;           // AP + DNS + portal
inline constexpr Action kActStopAp = 1u << 5;            // portal + DNS + AP
inline constexpr Action kActAnswer202 = 1u << 6;
inline constexpr Action kActAnswer400 = 1u << 7;
inline constexpr Action kActAnswer409 = 1u << 8;
inline constexpr Action kActScheduleTry = 1u << 9;       // TryStart in 1 s
inline constexpr Action kActStartTry = 1u << 10;         // station with the submission; 20 s timer
inline constexpr Action kActStoreSubmitted = 1u << 11;   // location, then wifi/pass, then wifi/ssid
inline constexpr Action kActStartGrace = 1u << 12;       // 5 s timer
inline constexpr Action kActStartJoin = 1u << 13;        // station with the stored details
inline constexpr Action kActStartBackoff = 1u << 14;
inline constexpr Action kActResetBackoff = 1u << 15;
inline constexpr Action kActShowSetup = 1u << 16;
inline constexpr Action kActShowConnecting = 1u << 17;
inline constexpr Action kActShowConnected = 1u << 18;
inline constexpr Action kActShowMain = 1u << 19;
inline constexpr Action kActFooter = 1u << 20;           // setup footer changed
inline constexpr Action kActRefused = 1u << 21;

struct Step {
    Mode next;
    Action action;
};

Step mode_step(Mode current, Event event, const Context& ctx);

bool ap_up(Mode mode);              // Setup, Connecting, Grace
bool is_run(Mode mode);             // RunJoining, RunUp, RunBackoff, RunNoRadio
const char* mode_name(Mode mode);   // "start", "setup", "connecting", "connected", "run", "run"...

}  // namespace nvp::net
