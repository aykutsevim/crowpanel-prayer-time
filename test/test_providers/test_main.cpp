// Test runner for the providers library (spec 006): JSON tokenizer, İmsakiyem parsers, batch
// validation and the refresh flow. PlatformIO builds every file of a test folder into one program,
// so each file exposes a run_*() function.
#include <unity.h>

void run_json_stream_tests();
void run_imsakiyem_tests();
void run_validate_tests();
void run_refresh_flow_tests();
void run_emushaf_tests();
void run_aladhan_tests();
void run_plausible_tests();
void run_http_source_tests();
void run_district_detail_tests();

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    run_json_stream_tests();
    run_imsakiyem_tests();
    run_validate_tests();
    run_refresh_flow_tests();
    run_emushaf_tests();
    run_aladhan_tests();
    run_plausible_tests();
    run_http_source_tests();
    run_district_detail_tests();
    return UNITY_END();
}
