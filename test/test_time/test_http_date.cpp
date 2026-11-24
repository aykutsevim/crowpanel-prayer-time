// The HTTP Date header of the time fallback (spec 005 FR-002, TIME-02, research R2; RFC 9110 §5.6.7).
// Written before the code.
#include <unity.h>

#include <cstdint>

#include "nvp/time/http_date.h"

using nvp::time::parse_http_date;

static void test_imf_fixdate() {
    int64_t t = 0;
    TEST_ASSERT_TRUE(parse_http_date("Tue, 24 Nov 2026 11:41:17 GMT", &t));
    TEST_ASSERT_TRUE(t == 1795520477);
    TEST_ASSERT_TRUE(parse_http_date("Thu, 01 Jan 1970 00:00:00 GMT", &t));
    TEST_ASSERT_TRUE(t == 0);
    TEST_ASSERT_TRUE(parse_http_date("Sun, 29 Feb 2032 23:59:59 GMT", &t));  // leap day
}

static void test_refused() {
    int64_t t = 7;
    const char* bad[] = {
        "Wed, 24 Nov 2026 11:41:17 GMT",       // wrong weekday
        "Tue, 24 nov 2026 11:41:17 GMT",       // lower-case month
        "Tue, 24 Nov 2026 11:41:17",           // no GMT
        "Tue, 24 Nov 2026 11:41:17 UTC",       // not GMT
        "Tuesday, 24-Nov-26 11:41:17 GMT",     // RFC 850 form
        "Tue Nov 24 11:41:17 2026",            // asctime form
        "Tue, 31 Nov 2026 11:41:17 GMT",       // no 31 November
        "Tue, 24 Nov 2026 24:00:00 GMT",       // hour out of range
        "Tue, 24 Nov 2026 11:60:17 GMT",       // minute out of range
        "Tue, 24 Nov 2026 11:41:17 GMT extra", // trailing text
        "",
    };
    for (const char* s : bad) TEST_ASSERT_FALSE_MESSAGE(parse_http_date(s, &t), s);
    TEST_ASSERT_TRUE(t == 7);  // untouched
    TEST_ASSERT_FALSE(parse_http_date(nullptr, &t));
}

void run_http_date_tests() {
    RUN_TEST(test_imf_fixdate);
    RUN_TEST(test_refused);
}
