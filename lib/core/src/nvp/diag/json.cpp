#include "nvp/diag/json.h"

#include <cstdio>
#include <cstring>

namespace nvp::diag {

size_t json_escape(const char* in, char* out, size_t cap) {
    size_t need = 0;
    size_t used = 0;
    bool full = false;
    for (const char* p = in; p != nullptr && *p != '\0'; ++p) {
        const auto c = static_cast<unsigned char>(*p);
        char piece[8];
        size_t n = 0;
        switch (c) {
            case '"': std::memcpy(piece, "\\\"", n = 2); break;
            case '\\': std::memcpy(piece, "\\\\", n = 2); break;
            case '\n': std::memcpy(piece, "\\n", n = 2); break;
            case '\r': std::memcpy(piece, "\\r", n = 2); break;
            case '\t': std::memcpy(piece, "\\t", n = 2); break;
            default:
                if (c < 0x20 || c == 0x7F) {
                    std::snprintf(piece, sizeof piece, "\\u%04X", static_cast<unsigned>(c));
                    n = 6;
                } else {
                    piece[0] = static_cast<char>(c);
                    n = 1;
                }
        }
        need += n;
        if (!full && cap > 0 && used + n < cap) {
            std::memcpy(out + used, piece, n);
            used += n;
        } else {
            full = true;  // keep counting, but never write a partial escape
        }
    }
    if (cap > 0) out[used] = '\0';
    return need;
}

}  // namespace nvp::diag
