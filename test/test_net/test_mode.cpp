// Mode machine (spec 004 FR-001..FR-003, MODE-02..MODE-06, NET-01; data-model.md table, research R9).
// The full (mode x event) grid is checked against the table below. Written before the code.
#include <unity.h>

#include <cstddef>
#include <cstdio>
#include <initializer_list>

#include "nvp/net/mode.h"

using namespace nvp::net;

namespace {

Context ctx(bool silence_ok, bool has_wifi, bool has_location) {
    Context c;
    c.silence_ok = silence_ok;
    c.has_wifi = has_wifi;
    c.has_location = has_location;
    return c;
}

const Context kFull = ctx(true, true, true);
const Context kNoLoc = ctx(true, true, false);

constexpr Mode kModes[] = {Mode::Start,  Mode::Setup,      Mode::Connecting, Mode::Grace,
                           Mode::RunJoining, Mode::RunUp, Mode::RunBackoff, Mode::RunNoRadio};
constexpr Event kEvents[] = {Event::Boot,        Event::PhoneJoined,  Event::PhoneLeft,  Event::SubmitValid,
                             Event::SubmitInvalid, Event::TryStart,   Event::TryOk,      Event::TryFail,
                             Event::TryTimeout,  Event::GraceElapsed, Event::JoinOk,     Event::JoinFail,
                             Event::LinkLost,    Event::BackoffElapsed, Event::StartOver, Event::DevSetup,
                             Event::DevCancel};

struct Row {
    Mode from;
    Event event;
    Mode to;
    Action action;
};

// The data-model.md table with context kFull (Wi-Fi and location stored, silence ok).
const Row kTable[] = {
    {Mode::Start, Event::Boot, Mode::RunJoining, kActStartJoin | kActShowMain},
    {Mode::Setup, Event::PhoneJoined, Mode::Setup, kActFooter},
    {Mode::Setup, Event::PhoneLeft, Mode::Setup, kActFooter},
    {Mode::Setup, Event::SubmitValid, Mode::Connecting, kActAnswer202 | kActScheduleTry | kActShowConnecting},
    {Mode::Setup, Event::SubmitInvalid, Mode::Setup, kActAnswer400},
    {Mode::Setup, Event::StartOver, Mode::Setup, kActEraseWifi},
    {Mode::Setup, Event::DevCancel, Mode::RunJoining, kActStopAp | kActStartJoin | kActShowMain},
    {Mode::Connecting, Event::SubmitValid, Mode::Connecting, kActAnswer409},
    {Mode::Connecting, Event::SubmitInvalid, Mode::Connecting, kActAnswer409},
    {Mode::Connecting, Event::TryStart, Mode::Connecting, kActStartTry},
    {Mode::Connecting, Event::TryOk, Mode::Grace, kActStoreSubmitted | kActStartGrace | kActShowConnected},
    {Mode::Connecting, Event::TryFail, Mode::Setup,
     kActDiscardSubmitted | kActDisconnect | kActShowSetup | kActFooter},
    {Mode::Connecting, Event::TryTimeout, Mode::Setup,
     kActDiscardSubmitted | kActDisconnect | kActShowSetup | kActFooter},
    {Mode::Grace, Event::SubmitValid, Mode::Grace, kActAnswer409},
    {Mode::Grace, Event::SubmitInvalid, Mode::Grace, kActAnswer409},
    {Mode::Grace, Event::GraceElapsed, Mode::RunUp, kActStopAp | kActResetBackoff | kActShowMain},
    {Mode::RunJoining, Event::JoinOk, Mode::RunUp, kActResetBackoff},
    {Mode::RunJoining, Event::JoinFail, Mode::RunBackoff, kActStartBackoff},
    {Mode::RunUp, Event::LinkLost, Mode::RunBackoff, kActStartBackoff},
    {Mode::RunBackoff, Event::BackoffElapsed, Mode::RunJoining, kActStartJoin},
    {Mode::RunJoining, Event::StartOver, Mode::Setup,
     kActEraseWifi | kActDisconnect | kActScan | kActStartAp | kActShowSetup},
    {Mode::RunUp, Event::StartOver, Mode::Setup,
     kActEraseWifi | kActDisconnect | kActScan | kActStartAp | kActShowSetup},
    {Mode::RunBackoff, Event::StartOver, Mode::Setup,
     kActEraseWifi | kActDisconnect | kActScan | kActStartAp | kActShowSetup},
    {Mode::RunJoining, Event::DevSetup, Mode::Setup, kActDisconnect | kActScan | kActStartAp | kActShowSetup},
    {Mode::RunUp, Event::DevSetup, Mode::Setup, kActDisconnect | kActScan | kActStartAp | kActShowSetup},
    {Mode::RunBackoff, Event::DevSetup, Mode::Setup, kActDisconnect | kActScan | kActStartAp | kActShowSetup},
    {Mode::RunNoRadio, Event::StartOver, Mode::RunNoRadio, kActEraseWifi},
    {Mode::RunNoRadio, Event::DevSetup, Mode::RunNoRadio, kActRefused},
};

const Row* find(Mode m, Event e) {
    for (const Row& r : kTable) {
        if (r.from == m && r.event == e) return &r;
    }
    return nullptr;
}

}  // namespace

static void test_full_grid_matches_table() {
    for (Mode m : kModes) {
        for (Event e : kEvents) {
            const Step s = mode_step(m, e, kFull);
            const Row* r = find(m, e);
            char msg[64];
            snprintf(msg, sizeof msg, "%s + event %d", mode_name(m), static_cast<int>(e));
            if (r != nullptr) {
                TEST_ASSERT_EQUAL_INT_MESSAGE(static_cast<int>(r->to), static_cast<int>(s.next), msg);
                TEST_ASSERT_EQUAL_HEX32_MESSAGE(r->action, s.action, msg);
            } else {  // unlisted pairs: unchanged, no action
                TEST_ASSERT_EQUAL_INT_MESSAGE(static_cast<int>(m), static_cast<int>(s.next), msg);
                TEST_ASSERT_EQUAL_HEX32_MESSAGE(kActNone, s.action, msg);
            }
        }
    }
}

static void test_boot_combinations() {  // MODE-02, OD4-1, analysis U1
    Step s = mode_step(Mode::Start, Event::Boot, kFull);
    TEST_ASSERT_TRUE(s.next == Mode::RunJoining);
    for (const Context& c : {ctx(true, true, false), ctx(true, false, true), ctx(true, false, false)}) {
        s = mode_step(Mode::Start, Event::Boot, c);
        TEST_ASSERT_TRUE(s.next == Mode::Setup);
        TEST_ASSERT_EQUAL_HEX32(kActScan | kActStartAp | kActShowSetup, s.action);
    }
    // The silence step failed: no radio and no setup, whatever is stored (spec 001 FR-001a).
    for (bool w : {false, true}) {
        for (bool l : {false, true}) {
            s = mode_step(Mode::Start, Event::Boot, ctx(false, w, l));
            TEST_ASSERT_TRUE(s.next == Mode::RunNoRadio);
            TEST_ASSERT_EQUAL_HEX32(kActShowMain, s.action);
        }
    }
}

static void test_scan_comes_before_ap() {  // research R3: the scan action precedes the AP start
    TEST_ASSERT_TRUE(kActScan < kActStartAp);
}

static void test_run_never_starts_ap_by_itself() {  // MODE-05
    const Event own[] = {Event::Boot, Event::JoinOk, Event::JoinFail, Event::LinkLost, Event::BackoffElapsed,
                         Event::PhoneJoined, Event::PhoneLeft, Event::TryFail, Event::TryTimeout};
    const Mode runs[] = {Mode::RunJoining, Mode::RunUp, Mode::RunBackoff, Mode::RunNoRadio};
    for (Mode m : runs) {
        for (Event e : own) {
            for (const Context& c : {kFull, kNoLoc}) {
                TEST_ASSERT_FALSE(mode_step(m, e, c).action & kActStartAp);
            }
        }
    }
}

static void test_setup_has_no_timer() {  // MODE-03: no time event moves Setup
    for (Event e : {Event::TryTimeout, Event::GraceElapsed, Event::BackoffElapsed, Event::TryStart}) {
        const Step s = mode_step(Mode::Setup, e, kNoLoc);
        TEST_ASSERT_TRUE(s.next == Mode::Setup);
        TEST_ASSERT_EQUAL_HEX32(kActNone, s.action);
    }
}

static void test_cancel_needs_both_details() {
    const Step s = mode_step(Mode::Setup, Event::DevCancel, kNoLoc);
    TEST_ASSERT_TRUE(s.next == Mode::Setup);
    TEST_ASSERT_EQUAL_HEX32(kActRefused, s.action);
}

static void test_dev_setup_refused_without_silence() {
    const Step s = mode_step(Mode::RunUp, Event::DevSetup, ctx(false, true, true));
    TEST_ASSERT_TRUE(s.next == Mode::RunUp);
    TEST_ASSERT_EQUAL_HEX32(kActRefused, s.action);
}

static void test_ap_up_and_names() {
    TEST_ASSERT_TRUE(ap_up(Mode::Setup));
    TEST_ASSERT_TRUE(ap_up(Mode::Connecting));
    TEST_ASSERT_TRUE(ap_up(Mode::Grace));
    TEST_ASSERT_FALSE(ap_up(Mode::RunUp));
    TEST_ASSERT_FALSE(ap_up(Mode::RunNoRadio));
    TEST_ASSERT_EQUAL_STRING("setup", mode_name(Mode::Setup));
    TEST_ASSERT_EQUAL_STRING("connecting", mode_name(Mode::Connecting));
    TEST_ASSERT_EQUAL_STRING("connected", mode_name(Mode::Grace));
    TEST_ASSERT_EQUAL_STRING("run", mode_name(Mode::RunUp));
}

void run_mode_tests() {
    RUN_TEST(test_full_grid_matches_table);
    RUN_TEST(test_boot_combinations);
    RUN_TEST(test_scan_comes_before_ap);
    RUN_TEST(test_run_never_starts_ap_by_itself);
    RUN_TEST(test_setup_has_no_timer);
    RUN_TEST(test_cancel_needs_both_details);
    RUN_TEST(test_dev_setup_refused_without_silence);
    RUN_TEST(test_ap_up_and_names);
}
