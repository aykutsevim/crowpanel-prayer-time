#include "nvp/net/wifi_qr.h"

#include <cstring>

namespace nvp::net {
namespace {

bool put(char* out, size_t cap, size_t& n, char c) {
    if (n + 1 >= cap) return false;
    out[n++] = c;
    return true;
}

bool put_text(char* out, size_t cap, size_t& n, const char* s) {
    for (; *s; ++s) {
        if (!put(out, cap, n, *s)) return false;
    }
    return true;
}

bool put_escaped(char* out, size_t cap, size_t& n, const char* s) {
    for (; *s; ++s) {
        if (std::strchr("\\;,:\"", *s) != nullptr && !put(out, cap, n, '\\')) return false;
        if (!put(out, cap, n, *s)) return false;
    }
    return true;
}

}  // namespace

size_t wifi_qr_payload(const char* ssid, const char* password, char* out, size_t cap) {
    if (out == nullptr || cap == 0) return 0;
    size_t n = 0;
    const bool ok = put_text(out, cap, n, "WIFI:T:WPA;S:") && put_escaped(out, cap, n, ssid) &&
                    put_text(out, cap, n, ";P:") && put_escaped(out, cap, n, password) &&
                    put_text(out, cap, n, ";;");
    if (!ok) {
        out[0] = '\0';
        return 0;
    }
    out[n] = '\0';
    return n;
}

}  // namespace nvp::net
