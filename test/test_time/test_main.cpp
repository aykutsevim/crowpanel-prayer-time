// Test runner for real time (spec 005): wall clock, POSIX time zones, HTTP dates. PlatformIO builds
// every file of a test folder into one program, so each file exposes a run_*() function.
#include <unity.h>

void run_wall_clock_tests();
void run_zone_tests();
void run_http_date_tests();

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    run_wall_clock_tests();
    run_zone_tests();
    run_http_date_tests();
    return UNITY_END();
}
