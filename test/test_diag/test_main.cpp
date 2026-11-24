// Test runner for the diagnostic-mode core (lib/core/src/nvp/diag, spec 001).
// PlatformIO builds every file of a test folder into one program, so each file exposes a
// run_*() function and this file owns main(), setUp() and tearDown().
#include <unity.h>

void run_selftest_report_tests();
void run_command_parser_tests();
void run_touch_judge_tests();
void run_net_helper_tests();
void run_stress_tests();
void run_form_tests();
void run_command_args_tests();

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    run_selftest_report_tests();
    run_command_parser_tests();
    run_touch_judge_tests();
    run_net_helper_tests();
    run_stress_tests();
    run_form_tests();
    run_command_args_tests();
    return UNITY_END();
}
