// Test runner for the diagnostic-mode core (lib/core/src/nvp/diag, spec 001).
// PlatformIO builds every file of a test folder into one program, so each file exposes a
// run_*() function and this file owns main(), setUp() and tearDown().
#include <unity.h>

void run_selftest_report_tests();
void run_command_parser_tests();
void run_touch_judge_tests();

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    run_selftest_report_tests();
    run_command_parser_tests();
    run_touch_judge_tests();
    return UNITY_END();
}
