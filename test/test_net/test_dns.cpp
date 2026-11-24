// DNS answers of the captive portal (spec 004 FR-011, contracts/portal-http.md, research R1, R14).
// Written before the code.
#include <unity.h>

#include <cstdint>
#include <cstring>

#include "nvp/net/dns_reply.h"

using namespace nvp::net;

namespace {

const uint8_t kIp[4] = {192, 168, 4, 1};

// Builds a standard query: header (id, flags RD, 1 question) + name + type + class IN.
size_t query(uint8_t* out, uint16_t id, const char* name, uint16_t type, uint16_t qdcount = 1, uint16_t flags = 0x0100) {
    size_t n = 0;
    out[n++] = id >> 8;
    out[n++] = id & 0xFF;
    out[n++] = flags >> 8;
    out[n++] = flags & 0xFF;
    out[n++] = qdcount >> 8;
    out[n++] = qdcount & 0xFF;
    for (int i = 0; i < 6; ++i) out[n++] = 0;
    const char* p = name;
    while (*p) {
        const char* dot = std::strchr(p, '.');
        const size_t len = dot ? static_cast<size_t>(dot - p) : std::strlen(p);
        out[n++] = static_cast<uint8_t>(len);
        std::memcpy(out + n, p, len);
        n += len;
        p += len;
        if (*p == '.') ++p;
    }
    out[n++] = 0;
    out[n++] = type >> 8;
    out[n++] = type & 0xFF;
    out[n++] = 0;
    out[n++] = 1;
    return n;
}

uint16_t u16(const uint8_t* p) { return static_cast<uint16_t>(p[0] << 8 | p[1]); }

}  // namespace

static void test_a_query_answered_with_ap_address() {
    uint8_t q[128], r[160];
    const size_t n = query(q, 0xBEEF, "connectivitycheck.gstatic.com", 1);
    const size_t m = dns_reply(q, n, kIp, r, sizeof r);
    TEST_ASSERT_EQUAL_UINT32(n + 16, m);
    TEST_ASSERT_EQUAL_HEX16(0xBEEF, u16(r));
    TEST_ASSERT_TRUE(r[2] & 0x80);                 // QR: response
    TEST_ASSERT_EQUAL_UINT8(0, r[3] & 0x0F);        // RCODE NOERROR
    TEST_ASSERT_EQUAL_UINT16(1, u16(r + 4));        // QDCOUNT
    TEST_ASSERT_EQUAL_UINT16(1, u16(r + 6));        // ANCOUNT
    TEST_ASSERT_EQUAL_MEMORY(q + 12, r + 12, n - 12);  // question echoed
    const uint8_t* a = r + n;
    TEST_ASSERT_EQUAL_HEX16(0xC00C, u16(a));        // pointer to the question name
    TEST_ASSERT_EQUAL_UINT16(1, u16(a + 2));        // type A
    TEST_ASSERT_EQUAL_UINT16(1, u16(a + 4));        // class IN
    TEST_ASSERT_EQUAL_UINT32(0, (a[6] << 24) | (a[7] << 16) | (a[8] << 8) | a[9]);  // TTL 0
    TEST_ASSERT_EQUAL_UINT16(4, u16(a + 10));
    TEST_ASSERT_EQUAL_MEMORY(kIp, a + 12, 4);
}

static void test_aaaa_gets_no_answer() {
    uint8_t q[128], r[160];
    const size_t n = query(q, 7, "captive.apple.com", 28);
    const size_t m = dns_reply(q, n, kIp, r, sizeof r);
    TEST_ASSERT_EQUAL_UINT32(n, m);
    TEST_ASSERT_TRUE(r[2] & 0x80);
    TEST_ASSERT_EQUAL_UINT8(0, r[3] & 0x0F);
    TEST_ASSERT_EQUAL_UINT16(0, u16(r + 6));
}

static void test_ignored_packets() {
    uint8_t q[128], r[160];
    size_t n = query(q, 1, "a.b", 1, 1, 0x8100);  // a response
    TEST_ASSERT_EQUAL_UINT32(0, dns_reply(q, n, kIp, r, sizeof r));
    n = query(q, 1, "a.b", 1, 2);  // two questions
    TEST_ASSERT_EQUAL_UINT32(0, dns_reply(q, n, kIp, r, sizeof r));
    n = query(q, 1, "a.b", 1);
    TEST_ASSERT_EQUAL_UINT32(0, dns_reply(q, 11, kIp, r, sizeof r));  // truncated header
    TEST_ASSERT_EQUAL_UINT32(0, dns_reply(q, n - 3, kIp, r, sizeof r));  // truncated question
    TEST_ASSERT_EQUAL_UINT32(0, dns_reply(q, n, kIp, r, n + 15));  // output too small
    // A compression pointer in a query's question is not allowed (and could loop).
    uint8_t loop[] = {0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0xC0, 12, 0, 1, 0, 1};
    TEST_ASSERT_EQUAL_UINT32(0, dns_reply(loop, sizeof loop, kIp, r, sizeof r));
    // A label running past the end.
    uint8_t over[] = {0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 40, 'a', 'b'};
    TEST_ASSERT_EQUAL_UINT32(0, dns_reply(over, sizeof over, kIp, r, sizeof r));
}

static void test_never_amplifies() {
    uint8_t q[300], r[400];
    const size_t n = query(q, 9, "a.very.long.name.of.some.host.example.com", 1);
    TEST_ASSERT_TRUE(dns_reply(q, n, kIp, r, sizeof r) <= n + 16);
}

void run_dns_tests() {
    RUN_TEST(test_a_query_answered_with_ap_address);
    RUN_TEST(test_aaaa_gets_no_answer);
    RUN_TEST(test_ignored_packets);
    RUN_TEST(test_never_amplifies);
}
