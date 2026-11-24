#include "nvp/net/places.h"

#include <cstring>

namespace nvp::net {
namespace {

// A forward-only cursor over the list text. Every read checks the end, so truncated input fails.
struct Cursor {
    const char* p;
    const char* end;

    void ws() {
        while (p < end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) ++p;
    }
    bool eat(char c) {
        ws();
        if (p < end && *p == c) {
            ++p;
            return true;
        }
        return false;
    }
    bool peek(char c) {
        ws();
        return p < end && *p == c;
    }
    bool number(uint32_t& out) {
        ws();
        if (p >= end || *p < '0' || *p > '9') return false;
        uint64_t v = 0;
        while (p < end && *p >= '0' && *p <= '9') {
            v = v * 10 + static_cast<uint32_t>(*p - '0');
            if (v > 0xFFFFFFFFu) return false;
            ++p;
        }
        out = static_cast<uint32_t>(v);
        return true;
    }
    // Reads a JSON string and reports whether it equals `want` (simple escapes only: \" and \\).
    bool string_equals(const char* want, bool& equal) {
        ws();
        if (p >= end || *p != '"') return false;
        ++p;
        const char* w = want;
        equal = true;
        while (p < end && *p != '"') {
            char c = *p++;
            if (c == '\\') {
                if (p >= end) return false;
                c = *p++;
                if (c != '"' && c != '\\' && c != '/') return false;
            }
            if (equal && *w == c) {
                ++w;
            } else {
                equal = false;
            }
        }
        if (p >= end) return false;
        ++p;  // closing quote
        if (*w != '\0') equal = false;
        return true;
    }
};

}  // namespace

bool place_lookup(const char* json, size_t len, uint32_t district_id, const char* province, const char* district) {
    if (json == nullptr || province == nullptr || district == nullptr) return false;
    if (district_id == 0 || province[0] == '\0' || district[0] == '\0') return false;
    const char* key = nullptr;
    for (const char* s = json; s + 11 <= json + len; ++s) {
        if (std::memcmp(s, "\"provinces\"", 11) == 0) {
            key = s + 11;
            break;
        }
    }
    if (key == nullptr) return false;
    Cursor c{key, json + len};
    if (!c.eat(':') || !c.eat('[')) return false;
    if (c.eat(']')) return false;
    do {
        uint32_t pid = 0;
        bool province_match = false;
        if (!c.eat('[') || !c.number(pid) || !c.eat(',') || !c.string_equals(province, province_match) ||
            !c.eat(',') || !c.eat('[')) {
            return false;
        }
        if (!c.peek(']')) {
            do {
                uint32_t id = 0;
                bool name_match = false;
                if (!c.eat('[') || !c.number(id) || !c.eat(',') || !c.string_equals(district, name_match) ||
                    !c.eat(']')) {
                    return false;
                }
                if (province_match && name_match && id == district_id) return true;
            } while (c.eat(','));
        }
        if (!c.eat(']') || !c.eat(']')) return false;
    } while (c.eat(','));
    return false;
}

}  // namespace nvp::net
