// Test runner for the refresh planner and the refresh runner (spec 007 Stage 1): triggers,
// provider order, mirrors, calculated fallback, politeness and edge cases, all against scripted
// sources. PlatformIO builds every file of a test folder into one program, so each file exposes a
// run_*().
#include <unity.h>

void run_trigger_tests();
void run_runner_order_tests();
void run_runner_mirror_tests();
void run_calculated_tests();
void run_politeness_tests();
void run_edge_case_tests();

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    run_trigger_tests();
    run_runner_order_tests();
    run_runner_mirror_tests();
    run_calculated_tests();
    run_politeness_tests();
    run_edge_case_tests();
    return UNITY_END();
}
