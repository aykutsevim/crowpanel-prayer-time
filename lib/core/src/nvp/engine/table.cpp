#include "nvp/engine/table.h"

#include "nvp/text/tr.h"

namespace nvp::engine {
namespace {

class Line {
public:
    Line(char* out, size_t cap) : out_(out), cap_(cap) {}

    Line& operator<<(const char* s) {
        for (; *s != '\0'; ++s) {
            if (out_ == nullptr || len_ + 1 >= cap_) {
                overflow_ = true;
                return *this;
            }
            out_[len_++] = *s;
        }
        return *this;
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

const char* state_word(ColumnState s) {
    switch (s) {
        case ColumnState::Past: return "past";
        case ColumnState::Current: return "current";
        case ColumnState::Next: return "next";
        case ColumnState::Later: return "later";
    }
    return "?";
}

}  // namespace

size_t table_header(char* out, size_t cap) {
    Line l(out, cap);
    l << "| Time | Period | Bounds | Countdown label | Remaining | Soon | Entered";
    for (const char* name : text::tr::kVakitNames) l << " | " << name;
    l << " |\n|";
    for (int i = 0; i < 13; ++i) l << "---|";
    l << "\n";
    return l.finish();
}

size_t table_row(const MainView& v, char* out, size_t cap) {
    Line l(out, cap);
    l << "| " << v.clock_hhmm << v.clock_ss << " | " << v.period_name << " | " << v.start_text
      << "\xE2\x80\x93"  // U+2013 en dash
      << v.end_text << " | " << v.countdown_label << " | " << v.countdown_text << " | " << (v.soon ? "yes" : "no")
      << " | " << (v.entered ? "yes" : "no");
    for (const Column& c : v.columns) {
        l << " | " << state_word(c.state);
        if (c.note[0] != '\0') l << " " << c.note;
    }
    l << " |\n";
    return l.finish();
}

}  // namespace nvp::engine
