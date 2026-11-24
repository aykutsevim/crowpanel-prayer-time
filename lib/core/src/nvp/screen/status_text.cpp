#include "nvp/screen/status_text.h"

#include "nvp/text/tr.h"

namespace nvp::screen {
namespace {

namespace tr = nvp::text::tr;

class Writer {
public:
    Writer(char* out, size_t cap) : out_(out), cap_(cap) {}
    void str(const char* s) {
        for (; s != nullptr && *s != '\0'; ++s) {
            if (out_ == nullptr || len_ + 1 >= cap_) {
                overflow_ = true;
                return;
            }
            out_[len_++] = *s;
        }
    }
    void uint(uint32_t v) {
        char d[10];
        int n = 0;
        do {
            d[n++] = static_cast<char>('0' + v % 10);
            v /= 10;
        } while (v != 0);
        char one[2] = {0, 0};
        while (n > 0) {
            one[0] = d[--n];
            str(one);
        }
    }
    size_t finish() {
        if (out_ == nullptr || cap_ == 0) return 0;
        if (overflow_) {
            out_[0] = '\0';
            return 0;
        }
        out_[len_] = '\0';
        return len_;
    }

private:
    char* out_;
    size_t cap_;
    size_t len_ = 0;
    bool overflow_ = false;
};

}  // namespace

size_t status_text(SystemState s, const char* last_update, int16_t days_left, char* out, size_t cap) {
    Writer w(out, cap);
    switch (s) {
        case SystemState::NoTime: w.str(tr::kStatusNoTime); break;
        case SystemState::NoData: w.str(tr::kStatusNoData); break;
        case SystemState::Calculated: w.str(tr::kStatusCalculated); break;
        case SystemState::CacheEnding:
            if (days_left <= 0) {
                w.str(tr::kStatusCacheEndingToday);
            } else {
                w.str(tr::kStatusCacheEndingPrefix);
                w.uint(static_cast<uint32_t>(days_left));
                w.str(tr::kStatusCacheEndingDays);
            }
            break;
        case SystemState::Offline: w.str(tr::kStatusOffline); break;
        case SystemState::Ok:
            w.str(tr::kStatusLastUpdate);
            w.str(last_update);
            break;
    }
    return w.finish();
}

bool status_is_alert(SystemState s) {
    return s == SystemState::NoTime || s == SystemState::NoData || s == SystemState::Calculated ||
           s == SystemState::CacheEnding;
}

StatusIcon status_icon(SystemState s) {
    switch (s) {
        case SystemState::Ok: return StatusIcon::Wifi;
        case SystemState::Calculated: return StatusIcon::Approx;
        default: return StatusIcon::WifiOff;
    }
}

}  // namespace nvp::screen
