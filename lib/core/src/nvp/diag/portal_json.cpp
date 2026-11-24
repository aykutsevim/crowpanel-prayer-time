#include "nvp/diag/portal_json.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "nvp/diag/json.h"

namespace nvp::diag {
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

    void add_escaped(const char* text) {
        char buf[200];  // a 32-byte name escapes to at most 192 bytes
        json_escape(text, buf, sizeof buf);
        add("%s", buf);
    }

    size_t length() const { return len_; }

private:
    char* out_;
    size_t cap_;
    size_t len_ = 0;
};

}  // namespace

size_t build_scan_json(ScanEntry* entries, size_t count, char* out, size_t cap) {
    // Insertion sort, strongest first (at most a few dozen entries).
    for (size_t i = 1; i < count; ++i) {
        const ScanEntry e = entries[i];
        size_t j = i;
        while (j > 0 && entries[j - 1].rssi < e.rssi) {
            entries[j] = entries[j - 1];
            --j;
        }
        entries[j] = e;
    }
    Out o(out, cap);
    o.add("{\"networks\":[");
    size_t listed = 0;
    for (size_t i = 0; i < count && listed < kMaxScanEntries; ++i) {
        if (entries[i].ssid[0] == '\0') continue;
        bool duplicate = false;
        for (size_t k = 0; k < i && !duplicate; ++k) {
            duplicate = std::strcmp(entries[k].ssid, entries[i].ssid) == 0;
        }
        if (duplicate) continue;
        o.add("%s{\"ssid\":\"", listed ? "," : "");
        o.add_escaped(entries[i].ssid);
        o.add("\",\"rssi\":%d,\"secured\":%s}", static_cast<int>(entries[i].rssi),
              entries[i].secured ? "true" : "false");
        ++listed;
    }
    o.add("]}");
    return o.length();
}

size_t build_status_json(PortalState state, const char* ssid, char* out, size_t cap) {
    static const char* const kNames[] = {"setup", "trying", "connected", "failed"};
    Out o(out, cap);
    o.add("{\"state\":\"%s\",\"ssid\":\"", kNames[static_cast<size_t>(state)]);
    o.add_escaped(ssid != nullptr ? ssid : "");
    o.add("\"}");
    return o.length();
}

}  // namespace nvp::diag
