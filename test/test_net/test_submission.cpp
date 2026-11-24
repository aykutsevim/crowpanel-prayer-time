// The setup page's form body (spec 004 FR-012, contracts/portal-http.md). Written before the code.
#include <unity.h>

#include <cstdio>
#include <cstring>

#include "nvp/net/submission.h"

using namespace nvp::net;

static SubmitError parse(const char* body, Submission& s) { return parse_submission(body, std::strlen(body), s); }

static const char* kLoc = "&district_id=9549&province=%C4%B0STANBUL&district=SULTANBEYL%C4%B0";

static void test_decoding() {
    Submission s;
    char body[512];
    snprintf(body, sizeof body, "ssid=Ev+A%%C4%%9F%%C4%%B1+%%F0%%9F%%8F%%A0&password=gizli%%26sifre%s", kLoc);
    TEST_ASSERT_TRUE(parse(body, s) == SubmitError::Ok);
    TEST_ASSERT_EQUAL_STRING("Ev Ağı 🏠", s.ssid);
    TEST_ASSERT_EQUAL_STRING("gizli&sifre", s.pass);
    TEST_ASSERT_EQUAL_UINT32(9549, s.location.district_id);
    TEST_ASSERT_EQUAL_STRING("İSTANBUL", s.location.province);
    TEST_ASSERT_EQUAL_STRING("SULTANBEYLİ", s.location.district);
}

static void test_bounds() {
    Submission s;
    char body[600];
    // SSID: 1..32 bytes after decoding.
    snprintf(body, sizeof body, "ssid=&password=%s", kLoc);
    TEST_ASSERT_TRUE(parse(body, s) == SubmitError::Ssid);
    snprintf(body, sizeof body, "ssid=%s&password=%s", "123456789012345678901234567890123", kLoc);
    TEST_ASSERT_TRUE(parse(body, s) == SubmitError::Ssid);
    snprintf(body, sizeof body, "ssid=%s&password=%s", "12345678901234567890123456789012", kLoc);
    TEST_ASSERT_TRUE(parse(body, s) == SubmitError::Ok);
    // Password: empty (open network) or 8..63.
    snprintf(body, sizeof body, "ssid=Ev&password=%s", kLoc);
    TEST_ASSERT_TRUE(parse(body, s) == SubmitError::Ok);
    TEST_ASSERT_EQUAL_STRING("", s.pass);
    snprintf(body, sizeof body, "ssid=Ev&password=1234567%s", kLoc);
    TEST_ASSERT_TRUE(parse(body, s) == SubmitError::Password);
    char pw[70];
    std::memset(pw, 'x', 64);
    pw[64] = '\0';
    snprintf(body, sizeof body, "ssid=Ev&password=%s%s", pw, kLoc);
    TEST_ASSERT_TRUE(parse(body, s) == SubmitError::Password);
    pw[63] = '\0';
    snprintf(body, sizeof body, "ssid=Ev&password=%s%s", pw, kLoc);
    TEST_ASSERT_TRUE(parse(body, s) == SubmitError::Ok);
    // Body over 512 bytes.
    std::memset(body, 'a', 520);
    body[520] = '\0';
    TEST_ASSERT_TRUE(parse(body, s) == SubmitError::Ssid);
}

static void test_location_fields() {
    Submission s;
    TEST_ASSERT_TRUE(parse("ssid=Ev&password=12345678", s) == SubmitError::Location);
    TEST_ASSERT_TRUE(parse("ssid=Ev&password=12345678&district_id=x&province=A&district=B", s) ==
                     SubmitError::Location);
    TEST_ASSERT_TRUE(parse("ssid=Ev&password=12345678&district_id=0&province=A&district=B", s) ==
                     SubmitError::Location);
    TEST_ASSERT_TRUE(parse("ssid=Ev&password=12345678&district_id=5&province=&district=B", s) ==
                     SubmitError::Location);
}

static void test_bad_escape_and_wipe() {
    Submission s;
    TEST_ASSERT_TRUE(parse("ssid=%ZZ&password=12345678&district_id=5&province=A&district=B", s) == SubmitError::Ssid);
    TEST_ASSERT_TRUE(parse("ssid=a%00b&password=12345678&district_id=5&province=A&district=B", s) ==
                     SubmitError::Ssid);  // NUL inside a name
    parse("ssid=Ev&password=12345678&district_id=5&province=A&district=B", s);
    wipe(s);
    for (char c : s.pass) TEST_ASSERT_EQUAL_CHAR(0, c);
    TEST_ASSERT_EQUAL_STRING("", s.ssid);
}

static void test_error_names() {
    TEST_ASSERT_EQUAL_STRING("ssid", submit_error_name(SubmitError::Ssid));
    TEST_ASSERT_EQUAL_STRING("password", submit_error_name(SubmitError::Password));
    TEST_ASSERT_EQUAL_STRING("location", submit_error_name(SubmitError::Location));
}

void run_submission_tests() {
    RUN_TEST(test_decoding);
    RUN_TEST(test_bounds);
    RUN_TEST(test_location_fields);
    RUN_TEST(test_bad_escape_and_wipe);
    RUN_TEST(test_error_names);
}
