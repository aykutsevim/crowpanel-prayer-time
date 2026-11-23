#include "nvp/diag/report.h"

#include <cstdarg>
#include <cstdio>

namespace nvp::diag::report {
namespace {

// Appends formatted text, tracking the length the full output needs.
class Writer {
public:
    Writer(char* out, size_t cap) : out_(out), cap_(cap) {
        if (cap_ > 0) out_[0] = '\0';
    }

    void add(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
        va_list args;
        va_start(args, fmt);
        char* dst = len_ < cap_ ? out_ + len_ : nullptr;
        const size_t room = len_ < cap_ ? cap_ - len_ : 0;
        const int n = std::vsnprintf(dst, room, fmt, args);
        va_end(args);
        if (n > 0) len_ += static_cast<size_t>(n);
    }

    // Appends a name with control bytes replaced, so it cannot break a report line.
    void add_name(const char* name) {
        char clean[34];
        size_t i = 0;
        for (; name[i] != '\0' && i + 1 < sizeof clean; ++i) {
            const auto c = static_cast<unsigned char>(name[i]);
            clean[i] = (c < 0x20 || c == 0x7F) ? '?' : static_cast<char>(c);
        }
        clean[i] = '\0';
        add("%s", clean);
    }

    size_t length() const { return len_; }

private:
    char* out_;
    size_t cap_;
    size_t len_ = 0;
};

void add_size(Writer& w, const char* what, uint32_t bytes, const char* none) {
    constexpr uint32_t kMiB = 1024u * 1024u;
    if (bytes == 0) {
        w.add("%s %s", what, none);
    } else if (bytes % kMiB == 0) {
        w.add("%s %lu MB", what, static_cast<unsigned long>(bytes / kMiB));
    } else {
        w.add("%s %lu bytes", what, static_cast<unsigned long>(bytes));
    }
}

void add_silence(Writer& w, const SilenceLine& s) {
    // One line for the whole sequence; it names no individual part (FR-026, SIL-05).
    if (!s.ran) {
        w.add("audio-off: not run\n");
    } else if (!s.acknowledged) {
        w.add("audio-off: failed (%s, attempts %u)\n", s.reason[0] ? s.reason : "no reason given",
              static_cast<unsigned>(s.attempts));
    } else if (s.ran_first) {
        w.add("audio-off: acknowledged first (attempts %u)\n", static_cast<unsigned>(s.attempts));
    } else {
        w.add("audio-off: acknowledged, not first (attempts %u)\n", static_cast<unsigned>(s.attempts));
    }
}

void add_wifi(Writer& w, const WifiLine& wifi) {
    w.add("wifi: ");
    switch (wifi.kind) {
        case WifiKind::Off: w.add("off"); break;
        case WifiKind::NotSetUp: w.add("not set up"); break;
        case WifiKind::Joining:
            w.add("joining ");
            w.add_name(wifi.ssid);
            break;
        case WifiKind::Connected:
            w.add("connected ");
            w.add_name(wifi.ssid);
            w.add(" %d dBm", static_cast<int>(wifi.rssi_dbm));
            break;
        case WifiKind::Hotspot:
            w.add("hotspot ");
            w.add_name(wifi.ap_ssid);
            w.add(" at ");
            w.add_name(wifi.ap_address);
            break;
    }
    w.add("\n");
}

// Signed error without a sign on zero, as in contracts/serial-console.md: (+2,-1), (0,0).
void add_signed(Writer& w, int v) {
    if (v > 0) {
        w.add("+%d", v);
    } else {
        w.add("%d", v);
    }
}

void add_corners(Writer& w, const CornerResult* corners) {
    static const char* const kNames[kCornerCount] = {"TL", "TR", "BL", "BR"};
    w.add("touch corners:");
    for (size_t i = 0; i < kCornerCount; ++i) {
        const CornerResult& c = corners[i];
        w.add(" %s ", kNames[i]);
        if (c.state == CornerState::NotTouched) {
            w.add("not touched");
            continue;
        }
        w.add("%s (", c.state == CornerState::Passed ? "passed" : "failed");
        add_signed(w, c.dx);
        w.add(",");
        add_signed(w, c.dy);
        w.add(")");
    }
    w.add("\n");
}

}  // namespace

size_t format_silence_line(const SilenceLine& silence, char* out, size_t cap) {
    Writer w(out, cap);
    add_silence(w, silence);
    return w.length();
}

size_t format(const ReportInput& in, char* out, size_t cap) {
    Writer w(out, cap);
    w.add("=== NVP REPORT ===\n");
    w.add("firmware: %s\n", in.firmware);
    w.add("board: %s %s (configured; %s)\n", in.model, in.revision,
          in.revision_detected ? "detected" : "not detected");
    w.add("memory: ");
    add_size(w, "flash", in.flash_bytes, "unknown");
    w.add(", ");
    add_size(w, "psram", in.psram_bytes, "none");
    w.add(" (detected)\n");
    add_silence(w, in.silence);
    for (size_t i = 0; i < kTestCount; ++i) {
        const SelfTestResult& r = in.tests[i];
        w.add("selftest %s: %s", test_name(static_cast<TestName>(i)), outcome_name(r.outcome));
        if ((r.outcome == Outcome::Failed || r.outcome == Outcome::Skipped) && r.reason[0] != '\0') {
            w.add(" (%s)", r.reason);
        }
        w.add("\n");
    }
    add_wifi(w, in.wifi);
    w.add("backlight: full (dimming disabled)\n");
    add_corners(w, in.corners);
    w.add("heap internal: %lu free, %lu largest\n", static_cast<unsigned long>(in.heap_internal_free),
          static_cast<unsigned long>(in.heap_internal_largest));
    w.add("heap psram: %lu free\n", static_cast<unsigned long>(in.heap_psram_free));
    w.add("stress: %s\n", in.stress_running ? "on" : "off");
    if (in.boot_to_diag_ms == 0) {
        w.add("boot-to-diag: n/a (no display)\n");
    } else {
        w.add("boot-to-diag: %lu ms\n", static_cast<unsigned long>(in.boot_to_diag_ms));
    }
    w.add("uptime: %lu s\n", static_cast<unsigned long>(in.uptime_s));
    w.add("=== END ===\n");
    return w.length();
}

}  // namespace nvp::diag::report
