#include "nvp/net/dns_reply.h"

#include <cstring>

namespace nvp::net {

size_t dns_reply(const uint8_t* q, size_t n, const uint8_t ip[4], uint8_t* out, size_t cap) {
    constexpr size_t kHeader = 12;
    if (q == nullptr || out == nullptr || n < kHeader + 5) return 0;
    if (q[2] & 0x80) return 0;                         // QR set: a response
    if ((q[2] & 0x78) != 0) return 0;                  // opcode other than a standard query
    if (q[4] != 0 || q[5] != 1) return 0;              // exactly one question
    // Walk the question name: plain labels only, at most 255 bytes.
    size_t p = kHeader;
    size_t name_len = 0;
    while (true) {
        if (p >= n) return 0;
        const uint8_t len = q[p];
        if (len == 0) break;
        if (len & 0xC0) return 0;                      // compression pointer or reserved bits
        name_len += len + 1u;
        if (name_len > 255) return 0;
        p += 1u + len;
    }
    p += 1;                                            // the zero length byte
    if (p + 4 > n) return 0;                           // type and class
    const uint16_t type = static_cast<uint16_t>(q[p] << 8 | q[p + 1]);
    const size_t question_end = p + 4;
    const bool answer_a = type == 1;
    const size_t total = question_end + (answer_a ? 16u : 0u);
    if (total > cap) return 0;

    std::memcpy(out, q, question_end);                 // header and question, echoed
    out[2] = static_cast<uint8_t>(0x80 | (q[2] & 0x01) | 0x04);  // QR, AA, keep RD
    out[3] = 0x80;                                     // RA, RCODE NOERROR
    out[6] = 0;
    out[7] = answer_a ? 1 : 0;                         // ANCOUNT
    out[8] = out[9] = out[10] = out[11] = 0;           // NSCOUNT, ARCOUNT
    if (answer_a) {
        uint8_t* a = out + question_end;
        a[0] = 0xC0;                                   // pointer to the name at offset 12
        a[1] = 0x0C;
        a[2] = 0; a[3] = 1;                            // type A
        a[4] = 0; a[5] = 1;                            // class IN
        a[6] = a[7] = a[8] = a[9] = 0;                 // TTL 0
        a[10] = 0; a[11] = 4;
        std::memcpy(a + 12, ip, 4);
    }
    return total;
}

}  // namespace nvp::net
