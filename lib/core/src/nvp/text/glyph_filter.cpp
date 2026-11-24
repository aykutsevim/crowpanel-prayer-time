#include "nvp/text/glyph_filter.h"

#include "nvp/text/utf8.h"

namespace nvp::text {

bool display_has_glyph(uint32_t c) {
    if (c >= 0x20 && c <= 0x7E) return true;
    switch (c) {
        case 0x00C7: case 0x00E7:  // Ç ç
        case 0x011E: case 0x011F:  // Ğ ğ
        case 0x0130: case 0x0131:  // İ ı
        case 0x00D6: case 0x00F6:  // Ö ö
        case 0x015E: case 0x015F:  // Ş ş
        case 0x00DC: case 0x00FC:  // Ü ü
        case 0x00C2: case 0x00E2:  // Â â
        case 0x00CE: case 0x00EE:  // Î î
        case 0x00DB: case 0x00FB:  // Û û
            return true;
        default:
            return false;
    }
}

size_t filter_display_glyphs(const char* in, char* out, size_t cap) {
    if (out == nullptr || cap == 0) return 0;
    size_t n = 0;
    const char* s = in != nullptr ? in : "";
    while (*s) {
        uint32_t c = utf8_next(s);
        if (c == kBadCodePoint || !display_has_glyph(c)) c = '?';
        char buf[4];
        const size_t k = utf8_put(c, buf);
        if (n + k + 1 > cap) break;
        for (size_t i = 0; i < k; ++i) out[n++] = buf[i];
    }
    out[n] = '\0';
    return n;
}

}  // namespace nvp::text
