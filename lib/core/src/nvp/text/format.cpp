#include "nvp/text/format.h"

#include "nvp/text/tr.h"

namespace nvp::text {
namespace {

// A bounded writer: once anything does not fit, the result is empty.
class Writer {
public:
    Writer(char* out, size_t cap) : out_(out), cap_(cap) {}

    void str(const char* s) {
        for (; *s != '\0'; ++s) put(*s);
    }

    void uint(uint32_t v) {
        char digits[10];
        int n = 0;
        do {
            digits[n++] = static_cast<char>('0' + v % 10);
            v /= 10;
        } while (v != 0);
        while (n > 0) put(digits[--n]);
    }

    void two(uint32_t v) {  // zero-padded, 00..99
        put(static_cast<char>('0' + (v / 10) % 10));
        put(static_cast<char>('0' + v % 10));
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
    void put(char c) {
        if (out_ == nullptr || len_ + 1 >= cap_) {
            overflow_ = true;
            return;
        }
        out_[len_++] = c;
    }

    char* out_;
    size_t cap_;
    size_t len_ = 0;
    bool overflow_ = false;
};

}  // namespace

size_t format_hhmm(uint16_t minutes, char* out, size_t cap) {
    Writer w(out, cap);
    w.two(minutes / 60u);
    w.str(":");
    w.two(minutes % 60u);
    return w.finish();
}

size_t format_hhmmss(int32_t seconds, char* out, size_t cap) {
    const uint32_t s = seconds < 0 ? 0u : static_cast<uint32_t>(seconds);
    Writer w(out, cap);
    const uint32_t hours = s / 3600u;
    if (hours < 100) {
        w.two(hours);
    } else {
        w.uint(hours);
    }
    w.str(":");
    w.two((s / 60u) % 60u);
    w.str(":");
    w.two(s % 60u);
    return w.finish();
}

size_t format_duration_note(int32_t seconds, char* out, size_t cap) {
    const uint32_t m = seconds < 60 ? 0u : static_cast<uint32_t>(seconds) / 60u;  // floor (02 §9.1)
    Writer w(out, cap);
    if (m < 1) {
        w.str(tr::kLessThanAMinute);
    } else if (m < 60) {
        w.uint(m);
        w.str(tr::kMinutes);
    } else {
        w.uint(m / 60u);
        w.str(tr::kHours);
        if (m % 60u != 0) {
            w.str(" ");
            w.uint(m % 60u);
            w.str(tr::kMinutes);
        }
    }
    w.str(tr::kSonra);
    return w.finish();
}

size_t format_date(const engine::Date& d, char* out, size_t cap) {
    if (!engine::valid(d)) {
        Writer empty(out, cap);
        return empty.finish();
    }
    Writer w(out, cap);
    w.uint(d.day);
    w.str(" ");
    w.str(tr::kMonths[d.month]);
    w.str(" ");
    w.uint(static_cast<uint32_t>(d.year));
    w.str(" ");
    w.str(tr::kWeekdays[engine::weekday(engine::to_day_number(d))]);
    return w.finish();
}

size_t format_hijri(const engine::HijriDate& h, char* out, size_t cap) {
    Writer w(out, cap);
    if (!h.present || h.month < 1 || h.month > 12) {
        return w.finish();  // empty
    }
    w.uint(h.day);
    w.str(" ");
    w.str(tr::kHijriMonths[h.month]);
    w.str(" ");
    w.uint(h.year);
    const size_t n = w.finish();
    return n;
}

}  // namespace nvp::text
