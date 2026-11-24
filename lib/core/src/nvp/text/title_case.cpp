#include "nvp/text/title_case.h"

#include "nvp/text/utf8.h"

namespace nvp::text {
namespace {

uint32_t lower_tr(uint32_t c) {
    switch (c) {
        case 'I': return 0x0131;     // ı
        case 0x0130: return 'i';     // İ
        case 0x00C7: return 0x00E7;  // Ç
        case 0x011E: return 0x011F;  // Ğ
        case 0x00D6: return 0x00F6;  // Ö
        case 0x015E: return 0x015F;  // Ş
        case 0x00DC: return 0x00FC;  // Ü
        case 0x00C2: return 0x00E2;  // Â
        case 0x00CE: return 0x00EE;  // Î
        case 0x00DB: return 0x00FB;  // Û
        default: return (c >= 'A' && c <= 'Z') ? c + 32 : c;
    }
}

uint32_t upper_tr(uint32_t c) {
    switch (c) {
        case 'i': return 0x0130;     // İ
        case 0x0131: return 'I';     // ı
        case 0x00E7: return 0x00C7;
        case 0x011F: return 0x011E;
        case 0x00F6: return 0x00D6;
        case 0x015F: return 0x015E;
        case 0x00FC: return 0x00DC;
        case 0x00E2: return 0x00C2;
        case 0x00EE: return 0x00CE;
        case 0x00FB: return 0x00DB;
        default: return (c >= 'a' && c <= 'z') ? c - 32 : c;
    }
}

bool word_break(uint32_t c) { return c == ' ' || c == '-' || c == '(' || c == '.'; }

}  // namespace

size_t title_case_tr(const char* in, char* out, size_t cap) {
    if (out == nullptr || cap == 0) return 0;
    out[0] = '\0';
    if (in == nullptr) return 0;
    size_t n = 0;
    bool start = true;
    const char* s = in;
    while (*s) {
        uint32_t c = utf8_next(s);
        if (c == kBadCodePoint) c = '?';
        c = lower_tr(c);
        if (start) c = upper_tr(c);
        start = word_break(c);
        char buf[4];
        const size_t k = utf8_put(c, buf);
        if (n + k + 1 > cap) {
            out[0] = '\0';
            return 0;
        }
        for (size_t i = 0; i < k; ++i) out[n++] = buf[i];
    }
    out[n] = '\0';
    return n;
}

}  // namespace nvp::text
