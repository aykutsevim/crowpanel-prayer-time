// The printed table equals specs/002-prayer-engine/contracts/engine-table.md byte for byte
// (FR-022, SC-005). The expected rows in that file were computed independently of the engine and
// checked against reference renders 02 and 09. Written before the code.
#include <unity.h>

#include <cstdio>
#include <cstring>

#include "helpers.h"
#include "nvp/engine/table.h"

using namespace engine_test;

namespace {

constexpr const char* kContract = "specs/002-prayer-engine/contracts/engine-table.md";
constexpr int32_t kMoments[] = {
    3 * 3600 + 30 * 60,  6 * 3600 + 15 * 60 + 59, 6 * 3600 + 16 * 60, 6 * 3600 + 50 * 60, 10 * 3600,
    12 * 3600 + 54 * 60, 12 * 3600 + 55 * 60,     fixture::kCheckSecond, 16 * 3600 + 20 * 60,
    17 * 3600 + 42 * 60, 17 * 3600 + 51 * 60,     18 * 3600 + 20 * 60,   19 * 3600 + 14 * 60,
    21 * 3600 + 30 * 60, 23 * 3600 + 59 * 60 + 59};

// The table lines of the contract: every line that starts with '|', each ending in '\n'.
size_t read_contract_table(char* out, size_t cap) {
    std::FILE* f = std::fopen(kContract, "rb");
    if (f == nullptr) return static_cast<size_t>(-1);
    size_t len = 0;
    char line[1024];
    while (std::fgets(line, sizeof line, f) != nullptr) {
        if (line[0] != '|') continue;
        size_t n = std::strlen(line);
        // A Windows checkout may have CRLF line ends; compare text, not line-end style.
        if (n >= 2 && line[n - 2] == '\r') {
            line[n - 2] = '\n';
            line[n - 1] = '\0';
            --n;
        }
        if (len + n + 1 > cap) break;
        std::memcpy(out + len, line, n);
        len += n;
    }
    std::fclose(f);
    out[len] = '\0';
    return len;
}

}  // namespace

static void test_table_matches_contract() {
    static char expected[16384];
    const size_t n = read_contract_table(expected, sizeof expected);
    if (n == static_cast<size_t>(-1)) {
        TEST_FAIL_MESSAGE("cannot open specs/002-prayer-engine/contracts/engine-table.md (run from the project folder)");
    }
    static char actual[16384];
    size_t len = table_header(actual, sizeof actual);
    TEST_ASSERT_TRUE(len > 0);
    for (int32_t s : kMoments) {
        const MainView v = eval(fixture_at(s));
        const size_t w = table_row(v, actual + len, sizeof actual - len);
        TEST_ASSERT_TRUE(w > 0);
        len += w;
    }
    TEST_ASSERT_EQUAL_STRING(expected, actual);
}

static void test_row_needs_room() {
    char small[16] = "x";
    TEST_ASSERT_EQUAL_UINT32(0, table_row(eval(fixture_at(fixture::kCheckSecond)), small, sizeof small));
    TEST_ASSERT_EQUAL_STRING("", small);
}

void run_table_tests() {
    RUN_TEST(test_table_matches_contract);
    RUN_TEST(test_row_needs_room);
}
