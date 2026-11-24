// Test runner for the main-screen logic (spec 003): system states, no-time evaluation, fixtures,
// RTC codec, simulated clock, gesture, status strings. PlatformIO builds every file of a test
// folder into one program, so each file exposes a run_*() function and this file owns main().
#include <unity.h>

void run_state_text_tests();
void run_system_state_tests();
void run_no_time_tests();
void run_fixture_tests();
void run_rtc_codec_tests();
void run_sim_clock_tests();
void run_gesture_tests();

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    run_state_text_tests();
    run_system_state_tests();
    run_no_time_tests();
    run_fixture_tests();
    run_rtc_codec_tests();
    run_sim_clock_tests();
    run_gesture_tests();
    return UNITY_END();
}
