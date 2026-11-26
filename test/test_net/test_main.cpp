// Test runner for the Wi-Fi setup logic (spec 004): mode machine, failure classes, DNS reply,
// scan entries, QR payload, place lookup, submission, screen views. PlatformIO builds every file
// of a test folder into one program, so each file exposes a run_*() function and this file owns main().
#include <unity.h>

void run_mode_tests();
void run_failure_tests();
void run_dns_tests();
void run_scan_tests();
void run_wifi_qr_tests();
void run_places_tests();
void run_submission_tests();
void run_views_tests();
void run_secrets_tests();
void run_coords_tests();

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    run_mode_tests();
    run_failure_tests();
    run_dns_tests();
    run_scan_tests();
    run_wifi_qr_tests();
    run_places_tests();
    run_submission_tests();
    run_views_tests();
    run_secrets_tests();
    run_coords_tests();
    return UNITY_END();
}
