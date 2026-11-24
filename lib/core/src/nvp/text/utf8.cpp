#include "nvp/text/utf8.h"

namespace nvp::text {

uint32_t utf8_next(const char*& s) {
    const auto* p = reinterpret_cast<const unsigned char*>(s);
    const unsigned char c = p[0];
    if (c < 0x80) {
        s += 1;
        return c;
    }
    int len = 0;
    uint32_t cp = 0;
    if ((c & 0xE0) == 0xC0) {
        len = 2;
        cp = c & 0x1F;
    } else if ((c & 0xF0) == 0xE0) {
        len = 3;
        cp = c & 0x0F;
    } else if ((c & 0xF8) == 0xF0) {
        len = 4;
        cp = c & 0x07;
    } else {
        s += 1;
        return kBadCodePoint;
    }
    for (int i = 1; i < len; ++i) {
        if ((p[i] & 0xC0) != 0x80) {  // also stops at the NUL terminator
            s += 1;
            return kBadCodePoint;
        }
        cp = (cp << 6) | (p[i] & 0x3F);
    }
    const uint32_t min[5] = {0, 0, 0x80, 0x800, 0x10000};
    if (cp < min[len] || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        s += 1;
        return kBadCodePoint;
    }
    s += len;
    return cp;
}

size_t utf8_put(uint32_t cp, char* out) {
    auto* o = reinterpret_cast<unsigned char*>(out);
    if (cp < 0x80) {
        o[0] = static_cast<unsigned char>(cp);
        return 1;
    }
    if (cp < 0x800) {
        o[0] = static_cast<unsigned char>(0xC0 | (cp >> 6));
        o[1] = static_cast<unsigned char>(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        o[0] = static_cast<unsigned char>(0xE0 | (cp >> 12));
        o[1] = static_cast<unsigned char>(0x80 | ((cp >> 6) & 0x3F));
        o[2] = static_cast<unsigned char>(0x80 | (cp & 0x3F));
        return 3;
    }
    o[0] = static_cast<unsigned char>(0xF0 | (cp >> 18));
    o[1] = static_cast<unsigned char>(0x80 | ((cp >> 12) & 0x3F));
    o[2] = static_cast<unsigned char>(0x80 | ((cp >> 6) & 0x3F));
    o[3] = static_cast<unsigned char>(0x80 | (cp & 0x3F));
    return 4;
}

}  // namespace nvp::text
