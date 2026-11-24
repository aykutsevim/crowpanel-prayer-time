#include "nvp/net/portal_json.h"

#include <cstdarg>
#include <cstdio>

#include "nvp/diag/json.h"

namespace nvp::net {
namespace {

class Out {
public:
    Out(char* out, size_t cap) : out_(out), cap_(cap) {
        if (cap_ > 0) out_[0] = '\0';
    }

    void add(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
        va_list args;
        va_start(args, fmt);
        const size_t room = len_ < cap_ ? cap_ - len_ : 0;
        const int n = std::vsnprintf(room ? out_ + len_ : nullptr, room, fmt, args);
        va_end(args);
        if (n > 0) len_ += static_cast<size_t>(n);
    }

    void add_string(const char* text) {
        char buf[256];  // a 40-byte name escapes to at most 240 bytes
        nvp::diag::json_escape(text != nullptr ? text : "", buf, sizeof buf);
        add("\"%s\"", buf);
    }

    size_t length() const { return len_; }

private:
    char* out_;
    size_t cap_;
    size_t len_ = 0;
};

}  // namespace

size_t scan_json(const ScanEntry* entries, size_t count, bool scanning, char* out, size_t cap) {
    Out o(out, cap);
    o.add("{\"scanning\":%s,\"networks\":[", scanning ? "true" : "false");
    for (size_t i = 0; i < count; ++i) {
        o.add("%s{\"ssid\":", i == 0 ? "" : ",");
        o.add_string(entries[i].ssid);
        o.add(",\"rssi\":%d,\"signal\":\"%s\",\"auth\":\"%s\"}", entries[i].rssi, signal_class(entries[i].rssi),
              auth_name(entries[i].auth));
    }
    o.add("]}");
    return o.length();
}

size_t status_json(const char* mode, const char* result, const char* ssid, const Location* loc, char* out, size_t cap) {
    Out o(out, cap);
    o.add("{\"mode\":\"%s\",\"result\":\"%s\",\"ssid\":", mode, result);
    o.add_string(ssid);
    if (loc == nullptr || loc->district_id == 0) {
        o.add(",\"location\":null}");
    } else {
        o.add(",\"location\":{\"id\":%lu,\"province\":", static_cast<unsigned long>(loc->district_id));
        o.add_string(loc->province);
        o.add(",\"district\":");
        o.add_string(loc->district);
        o.add("}}");
    }
    return o.length();
}

}  // namespace nvp::net
