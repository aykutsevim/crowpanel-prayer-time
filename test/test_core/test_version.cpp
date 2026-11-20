// Slice 0 placeholder test for lib/core: proves the host test environment builds lib/core
// with no hardware headers. Real engine tests (ENG-07) arrive in Slice 2.
#include <unity.h>

#include <cstdio>

#include "nvp/version.h"

void setUp() {}
void tearDown() {}

static void test_version_is_semver_shaped() {
    const char* v = nvp::kFirmwareVersion;
    TEST_ASSERT_NOT_NULL(v);
    int dots = 0;
    for (const char* p = v; *p; ++p) {
        if (*p == '.') {
            ++dots;
        } else {
            TEST_ASSERT_TRUE_MESSAGE(*p >= '0' && *p <= '9', "version must be digits and dots");
        }
    }
    TEST_ASSERT_EQUAL_INT(2, dots);
}

static void test_user_agent_names_project_and_version() {
    // SEC-03 / 04-data-sources.md §10: "NamazVaktiPanel/<version> (ESP32-S3; personal device)"
    char expected[96];
    std::snprintf(expected, sizeof expected, "NamazVaktiPanel/%s (ESP32-S3; personal device)",
                  nvp::kFirmwareVersion);
    TEST_ASSERT_EQUAL_STRING(expected, nvp::kUserAgent);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_version_is_semver_shaped);
    RUN_TEST(test_user_agent_names_project_and_version);
    return UNITY_END();
}
