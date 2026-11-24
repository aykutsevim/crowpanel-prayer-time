#include "nvp/diag/form.h"

#include <cstring>

namespace nvp::diag {
namespace {

int hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Decodes [begin, end) into out. False on overflow or a bad escape.
bool decode(const char* begin, const char* end, char* out, size_t cap) {
    size_t n = 0;
    for (const char* p = begin; p < end; ++p) {
        char c = *p;
        if (c == '+') {
            c = ' ';
        } else if (c == '%') {
            if (end - p < 3) return false;
            const int hi = hex(p[1]);
            const int lo = hex(p[2]);
            if (hi < 0 || lo < 0 || (hi == 0 && lo == 0)) return false;
            c = static_cast<char>(hi * 16 + lo);
            p += 2;
        }
        if (n + 1 >= cap) return false;
        out[n++] = c;
    }
    out[n] = '\0';
    return true;
}

}  // namespace

bool form_value(const char* body, const char* key, char* out, size_t cap) {
    if (out == nullptr || cap == 0) return false;
    out[0] = '\0';
    if (body == nullptr || key == nullptr) return false;
    const size_t key_len = std::strlen(key);
    const char* p = body;
    while (*p != '\0') {
        const char* amp = std::strchr(p, '&');
        const char* end = amp != nullptr ? amp : p + std::strlen(p);
        const char* eq = static_cast<const char*>(std::memchr(p, '=', static_cast<size_t>(end - p)));
        if (eq != nullptr && static_cast<size_t>(eq - p) == key_len && std::strncmp(p, key, key_len) == 0) {
            if (decode(eq + 1, end, out, cap)) return true;
            out[0] = '\0';
            return false;
        }
        if (amp == nullptr) break;
        p = amp + 1;
    }
    return false;
}

}  // namespace nvp::diag
