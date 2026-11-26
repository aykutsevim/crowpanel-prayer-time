// Test runner for the prayer-time calculation and calibration (spec 007 Stage 2, research R3, R8).
// PlatformIO builds every file of a test folder into one program, so each file exposes a run_*().
#include <unity.h>

void run_prayer_times_tests();
void run_calibrate_tests();

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    run_prayer_times_tests();
    run_calibrate_tests();
    return UNITY_END();
}
