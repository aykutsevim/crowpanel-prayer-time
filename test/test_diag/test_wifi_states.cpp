// Wi-Fi state machine (tasks T063; data-model.md WifiState; FR-001a, FR-019..FR-023).
// Written before the code.
#include <unity.h>

#include <cstring>

#include "nvp/diag/wifi_states.h"

using namespace nvp::diag;

namespace {

WifiContext ctx(bool silence_ok, bool has_stored) {
    WifiContext c;
    c.silence_ok = silence_ok;
    c.has_stored = has_stored;
    return c;
}

}  // namespace

static void test_boot_with_details_joins() {
    const WifiStep s = wifi_step(WifiState::Idle, WifiState::Idle, WifiEvent::Boot, ctx(true, true));
    TEST_ASSERT_TRUE(s.next == WifiState::Joining);
    TEST_ASSERT_TRUE(s.action & kActStartJoin);
}

static void test_boot_without_details_stays_idle() {
    const WifiStep s = wifi_step(WifiState::Idle, WifiState::Idle, WifiEvent::Boot, ctx(true, false));
    TEST_ASSERT_TRUE(s.next == WifiState::Idle);
    TEST_ASSERT_EQUAL_UINT32(kActNone, s.action);
}

static void test_join_results_and_backoff() {
    TEST_ASSERT_TRUE(wifi_step(WifiState::Joining, WifiState::Idle, WifiEvent::JoinOk, ctx(true, true)).next ==
                     WifiState::Connected);
    const WifiStep fail = wifi_step(WifiState::Joining, WifiState::Idle, WifiEvent::JoinFail, ctx(true, true));
    TEST_ASSERT_TRUE(fail.next == WifiState::Backoff);
    TEST_ASSERT_TRUE(fail.action & kActStartBackoff);
    const WifiStep lost = wifi_step(WifiState::Connected, WifiState::Idle, WifiEvent::LinkLost, ctx(true, true));
    TEST_ASSERT_TRUE(lost.next == WifiState::Backoff);
    const WifiStep again = wifi_step(WifiState::Backoff, WifiState::Idle, WifiEvent::BackoffElapsed, ctx(true, true));
    TEST_ASSERT_TRUE(again.next == WifiState::Joining);
    TEST_ASSERT_TRUE(again.action & kActStartJoin);
}

static void test_setup_opens_hotspot() {
    const WifiStep s = wifi_step(WifiState::Connected, WifiState::Idle, WifiEvent::SetupConfirmed, ctx(true, true));
    TEST_ASSERT_TRUE(s.next == WifiState::SetupAP);
    TEST_ASSERT_TRUE(s.action & kActStartAp);
}

static void test_failed_try_discards_and_stays_in_setup() {
    const WifiStep t = wifi_step(WifiState::SetupAP, WifiState::Idle, WifiEvent::Submit, ctx(true, false));
    TEST_ASSERT_TRUE(t.next == WifiState::Trying);
    TEST_ASSERT_TRUE(t.action & kActTrySubmitted);
    const WifiStep f = wifi_step(WifiState::Trying, WifiState::Idle, WifiEvent::TryFail, ctx(true, false));
    TEST_ASSERT_TRUE(f.next == WifiState::SetupAP);
    TEST_ASSERT_TRUE(f.action & kActDiscardSubmitted);
    TEST_ASSERT_FALSE(f.action & kActStoreSubmitted);
    TEST_ASSERT_FALSE(f.action & kActStopAp);
}

static void test_successful_try_stores_and_closes_hotspot() {
    const WifiStep s = wifi_step(WifiState::Trying, WifiState::Idle, WifiEvent::TryOk, ctx(true, false));
    TEST_ASSERT_TRUE(s.next == WifiState::Connected);
    TEST_ASSERT_TRUE(s.action & kActStoreSubmitted);
    TEST_ASSERT_TRUE(s.action & kActStopApAfterGrace);
}

static void test_cancel_returns_to_previous_and_keeps_details() {
    const WifiStep from_idle = wifi_step(WifiState::SetupAP, WifiState::Idle, WifiEvent::Cancel, ctx(true, false));
    TEST_ASSERT_TRUE(from_idle.next == WifiState::Idle);
    TEST_ASSERT_TRUE(from_idle.action & kActStopAp);
    TEST_ASSERT_FALSE(from_idle.action & kActEraseStored);
    const WifiStep from_conn = wifi_step(WifiState::SetupAP, WifiState::Connected, WifiEvent::Cancel, ctx(true, true));
    TEST_ASSERT_TRUE(from_conn.next == WifiState::Joining);  // reconnect with the kept details
    TEST_ASSERT_TRUE(from_conn.action & kActStopAp);
    TEST_ASSERT_TRUE(from_conn.action & kActStartJoin);
    TEST_ASSERT_FALSE(from_conn.action & kActEraseStored);
}

static void test_forget_erases_and_goes_idle() {
    const WifiState from[] = {WifiState::Idle, WifiState::Joining, WifiState::Connected, WifiState::Backoff};
    for (WifiState st : from) {
        const WifiStep s = wifi_step(st, WifiState::Idle, WifiEvent::Forget, ctx(true, true));
        TEST_ASSERT_TRUE(s.next == WifiState::Idle);
        TEST_ASSERT_TRUE(s.action & kActEraseStored);
        TEST_ASSERT_TRUE(s.action & kActDisconnect);
    }
}

static void test_forget_ignored_while_hotspot_is_open() {
    const WifiStep s = wifi_step(WifiState::SetupAP, WifiState::Idle, WifiEvent::Forget, ctx(true, true));
    TEST_ASSERT_TRUE(s.next == WifiState::SetupAP);
    TEST_ASSERT_EQUAL_UINT32(kActNone, s.action);
}

static void test_silence_not_acknowledged_refuses_every_start() {
    const WifiContext c = ctx(false, true);
    const WifiStep boot = wifi_step(WifiState::Idle, WifiState::Idle, WifiEvent::Boot, c);
    TEST_ASSERT_TRUE(boot.next == WifiState::Idle);
    TEST_ASSERT_FALSE(boot.action & kActStartJoin);
    const WifiStep setup = wifi_step(WifiState::Idle, WifiState::Idle, WifiEvent::SetupConfirmed, c);
    TEST_ASSERT_TRUE(setup.next == WifiState::Idle);
    TEST_ASSERT_TRUE(setup.action & kActRefused);
    TEST_ASSERT_FALSE(setup.action & kActStartAp);
    const WifiStep backoff = wifi_step(WifiState::Backoff, WifiState::Idle, WifiEvent::BackoffElapsed, c);
    TEST_ASSERT_FALSE(backoff.action & kActStartJoin);
}

static void test_credential_validation() {
    char ssid33[34];
    std::memset(ssid33, 'a', 33);
    ssid33[33] = '\0';
    char pass64[65];
    std::memset(pass64, 'p', 64);
    pass64[64] = '\0';
    TEST_ASSERT_TRUE(valid_credentials("EvAgi", "secret12"));
    TEST_ASSERT_TRUE(valid_credentials("Open", ""));  // open network: empty password
    TEST_ASSERT_TRUE(valid_credentials(ssid33 + 1, pass64 + 1));  // 32 and 63 bytes
    TEST_ASSERT_FALSE(valid_credentials("", "secret12"));
    TEST_ASSERT_FALSE(valid_credentials(ssid33, "secret12"));
    TEST_ASSERT_FALSE(valid_credentials("EvAgi", pass64));
    TEST_ASSERT_FALSE(valid_credentials(nullptr, ""));
    TEST_ASSERT_FALSE(valid_credentials("EvAgi", nullptr));
}

void run_wifi_state_tests() {
    RUN_TEST(test_boot_with_details_joins);
    RUN_TEST(test_boot_without_details_stays_idle);
    RUN_TEST(test_join_results_and_backoff);
    RUN_TEST(test_setup_opens_hotspot);
    RUN_TEST(test_failed_try_discards_and_stays_in_setup);
    RUN_TEST(test_successful_try_stores_and_closes_hotspot);
    RUN_TEST(test_cancel_returns_to_previous_and_keeps_details);
    RUN_TEST(test_forget_erases_and_goes_idle);
    RUN_TEST(test_forget_ignored_while_hotspot_is_open);
    RUN_TEST(test_silence_not_acknowledged_refuses_every_start);
    RUN_TEST(test_credential_validation);
}
