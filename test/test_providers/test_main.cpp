// Test runner for the providers library (spec 006): JSON tokenizer, İmsakiyem parsers, batch
// validation and the refresh flow. PlatformIO builds every file of a test folder into one program,
// so each file exposes a run_*() function.
#include <unity.h>

void run_json_stream_tests();
void run_imsakiyem_tests();
void run_validate_tests();
void run_refresh_flow_tests();

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    run_json_stream_tests();
    run_imsakiyem_tests();
    run_validate_tests();
    run_refresh_flow_tests();
    return UNITY_END();
}
