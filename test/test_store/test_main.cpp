// Test runner for the day-record store (spec 006): file format, merge rules and power loss.
// PlatformIO builds every file of a test folder into one program, so each file exposes a run_*().
#include <unity.h>

void run_store_format_tests();
void run_merge_tests();
void run_power_loss_tests();

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    run_store_format_tests();
    run_merge_tests();
    run_power_loss_tests();
    return UNITY_END();
}
