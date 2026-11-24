#include "nvp/net/mode.h"

namespace nvp::net {
namespace {

Step stay(Mode m) { return {m, kActNone}; }

constexpr Action kOpenSetup = kActScan | kActStartAp | kActShowSetup;

Step from_run(Mode m, Event e, const Context& c) {
    switch (e) {
        case Event::JoinOk:
            if (m == Mode::RunJoining) return {Mode::RunUp, kActResetBackoff};
            break;
        case Event::JoinFail:
            if (m == Mode::RunJoining) return {Mode::RunBackoff, kActStartBackoff};
            break;
        case Event::LinkLost:
            if (m == Mode::RunUp) return {Mode::RunBackoff, kActStartBackoff};
            break;
        case Event::BackoffElapsed:
            if (m == Mode::RunBackoff) return {Mode::RunJoining, kActStartJoin};
            break;
        case Event::StartOver:
            if (m == Mode::RunNoRadio || !c.silence_ok) return {m, kActEraseWifi};
            return {Mode::Setup, kActEraseWifi | kActDisconnect | kOpenSetup};
        case Event::DevSetup:
            if (m == Mode::RunNoRadio || !c.silence_ok) return {m, kActRefused};
            return {Mode::Setup, kActDisconnect | kOpenSetup};
        default:
            break;
    }
    return stay(m);
}

}  // namespace

Step mode_step(Mode m, Event e, const Context& c) {
    switch (m) {
        case Mode::Start:
            if (e != Event::Boot) return stay(m);
            if (!c.silence_ok) return {Mode::RunNoRadio, kActShowMain};
            if (c.has_wifi && c.has_location) return {Mode::RunJoining, kActStartJoin | kActShowMain};
            return {Mode::Setup, kOpenSetup};
        case Mode::Setup:
            switch (e) {
                case Event::PhoneJoined:
                case Event::PhoneLeft:
                    return {m, kActFooter};
                case Event::SubmitValid:
                    return {Mode::Connecting, kActAnswer202 | kActScheduleTry | kActShowConnecting};
                case Event::SubmitInvalid:
                    return {m, kActAnswer400};
                case Event::StartOver:
                    return {m, kActEraseWifi};
                case Event::DevCancel:
                    if (c.has_wifi && c.has_location) {
                        return {Mode::RunJoining, kActStopAp | kActStartJoin | kActShowMain};
                    }
                    return {m, kActRefused};
                default:
                    return stay(m);
            }
        case Mode::Connecting:
            switch (e) {
                case Event::SubmitValid:
                case Event::SubmitInvalid:
                    return {m, kActAnswer409};
                case Event::TryStart:
                    return {m, kActStartTry};
                case Event::TryOk:
                    return {Mode::Grace, kActStoreSubmitted | kActStartGrace | kActShowConnected};
                case Event::TryFail:
                case Event::TryTimeout:
                    return {Mode::Setup, kActDiscardSubmitted | kActDisconnect | kActShowSetup | kActFooter};
                default:
                    return stay(m);
            }
        case Mode::Grace:
            switch (e) {
                case Event::SubmitValid:
                case Event::SubmitInvalid:
                    return {m, kActAnswer409};
                case Event::GraceElapsed:
                    return {Mode::RunUp, kActStopAp | kActResetBackoff | kActShowMain};
                default:
                    return stay(m);
            }
        case Mode::RunJoining:
        case Mode::RunUp:
        case Mode::RunBackoff:
        case Mode::RunNoRadio:
            return from_run(m, e, c);
    }
    return stay(m);
}

bool ap_up(Mode m) { return m == Mode::Setup || m == Mode::Connecting || m == Mode::Grace; }

bool is_run(Mode m) {
    return m == Mode::RunJoining || m == Mode::RunUp || m == Mode::RunBackoff || m == Mode::RunNoRadio;
}

const char* mode_name(Mode m) {
    switch (m) {
        case Mode::Start: return "start";
        case Mode::Setup: return "setup";
        case Mode::Connecting: return "connecting";
        case Mode::Grace: return "connected";
        case Mode::RunJoining:
        case Mode::RunUp:
        case Mode::RunBackoff:
        case Mode::RunNoRadio: return "run";
    }
    return "?";
}

}  // namespace nvp::net
