#include "nvp/diag/command.h"

#include <cstddef>
#include <cstring>

#include "nvp/engine/calendar.h"

namespace nvp::diag {
namespace {

bool is_space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

char lower(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; }

struct Entry {
    const char* text;
    Command command;
};

constexpr Entry kCommands[] = {
    {"help", Command::Help},
    {"report", Command::Report},
    {"stress on", Command::StressOn},
    {"stress off", Command::StressOff},
    {"wifi setup", Command::WifiSetup},
    {"wifi cancel", Command::WifiCancel},
    {"wifi forget", Command::WifiForget},
    {"diag", Command::Diag},
    {"test write", Command::TestWrite},
    {"test scan", Command::TestScan},
    {"test https", Command::TestHttps},
    {"test mix", Command::TestMix},
    {"main", Command::Main},
    {"time get", Command::TimeGet},
    {"clock rtc", Command::ClockRtc},
    {"clock pause", Command::ClockPause},
    {"clock run", Command::ClockRun},
    {"state auto", Command::StateAuto},
    {"fixture off", Command::FixtureOff},
    {"tz", Command::TzGet},
    {"time sync", Command::TimeSync},
    {"ntp on", Command::NtpOn},
    {"ntp off", Command::NtpOff},
    {"clock invalid", Command::ClockInvalid},
    {"data refresh", Command::DataRefresh},
    {"data show", Command::DataShow},
    {"data corrupt", Command::DataCorrupt},
    {"data erase", Command::DataErase},
    {"data plan", Command::DataPlan},
    {"data calib", Command::DataCalib},
};

}  // namespace

Command parse_command(const char* line) {
    if (line == nullptr) return Command::Empty;
    // Normalise into lower case, words separated by single spaces.
    char norm[32];
    size_t n = 0;
    bool pending_space = false;
    for (const char* p = line; *p != '\0'; ++p) {
        if (is_space(*p)) {
            pending_space = n > 0;
            continue;
        }
        if (n + 2 >= sizeof norm) return Command::Unknown;  // longer than any command
        if (pending_space) {
            norm[n++] = ' ';
            pending_space = false;
        }
        norm[n++] = lower(*p);
    }
    norm[n] = '\0';
    if (n == 0) return Command::Empty;
    for (const Entry& e : kCommands) {
        if (std::strcmp(norm, e.text) == 0) return e.command;
    }
    return Command::Unknown;
}

namespace {

// Lower-case copy with single spaces; false if longer than `cap`.
bool normalise(const char* line, char* norm, size_t cap) {
    size_t n = 0;
    bool pending_space = false;
    for (const char* p = line; *p != '\0'; ++p) {
        if (is_space(*p)) {
            pending_space = n > 0;
            continue;
        }
        if (n + 2 >= cap) return false;
        if (pending_space) {
            norm[n++] = ' ';
            pending_space = false;
        }
        norm[n++] = lower(*p);
    }
    norm[n] = '\0';
    return true;
}

bool starts_with(const char* s, const char* prefix, const char** rest) {
    const size_t n = std::strlen(prefix);
    if (std::strncmp(s, prefix, n) != 0) return false;
    *rest = s + n;
    return true;
}

// Unsigned decimal of exactly `digits` digits (0 = any count >= 1) up to the end or a separator.
bool number(const char*& p, int digits, int* out) {
    int v = 0, n = 0;
    while (*p >= '0' && *p <= '9') {
        v = v * 10 + (*p - '0');
        ++p;
        if (++n > 9) return false;
    }
    if (n == 0 || (digits != 0 && n != digits)) return false;
    *out = v;
    return true;
}

// "YYYY-MM-DD HH:MM:SS", a real calendar date in 2000..2099.
bool date_time(const char* p, ParsedCommand& c) {
    if (!number(p, 4, &c.year) || *p++ != '-' || !number(p, 2, &c.month) || *p++ != '-' ||
        !number(p, 2, &c.day) || *p++ != ' ' || !number(p, 2, &c.hour) || *p++ != ':' ||
        !number(p, 2, &c.minute) || *p++ != ':' || !number(p, 2, &c.second) || *p != '\0') {
        return false;
    }
    if (c.year < 2000 || c.year > 2099 || c.hour > 23 || c.minute > 59 || c.second > 59) return false;
    const engine::Date d{static_cast<int16_t>(c.year), static_cast<uint8_t>(c.month), static_cast<uint8_t>(c.day)};
    return c.month >= 1 && c.month <= 12 && engine::valid(d);
}

// "tz <string>": the zone is case-sensitive, so it is taken from the raw line, trimmed.
bool tz_set(const char* line, ParsedCommand& c) {
    const char* p = line;
    while (is_space(*p)) ++p;
    if (lower(p[0]) != 't' || lower(p[1]) != 'z' || !is_space(p[2])) return false;
    p += 2;
    while (is_space(*p)) ++p;
    const char* end = p + std::strlen(p);
    while (end > p && is_space(end[-1])) --end;
    const size_t n = static_cast<size_t>(end - p);
    c.command = Command::InvalidArgument;
    if (n == 0 || n >= sizeof c.text) return true;
    std::memcpy(c.text, p, n);
    c.text[n] = '\0';
    c.command = Command::TzSet;
    return true;
}

}  // namespace

ParsedCommand parse_command_line(const char* line) {
    ParsedCommand c;
    c.command = parse_command(line);
    if (c.command != Command::Unknown) return c;
    if (line != nullptr && tz_set(line, c)) return c;
    char norm[64];
    if (line == nullptr || !normalise(line, norm, sizeof norm)) return c;  // Unknown
    const char* rest = nullptr;
    int n = 0;
    if (starts_with(norm, "time set", &rest)) {
        c.command = (*rest == ' ' && date_time(rest + 1, c)) ? Command::TimeSet : Command::InvalidArgument;
    } else if (starts_with(norm, "clock sim", &rest)) {
        c.command = (*rest == ' ' && date_time(rest + 1, c)) ? Command::ClockSim : Command::InvalidArgument;
    } else if (starts_with(norm, "clock speed", &rest)) {
        const char* p = rest + 1;
        const bool ok = *rest == ' ' && number(p, 0, &n) && *p == '\0' && n >= 1 && n <= 3600;
        c.command = ok ? Command::ClockSpeed : Command::InvalidArgument;
        c.number = n;
    } else if (starts_with(norm, "time jitter", &rest)) {
        const char* p = rest + 1;
        const bool ok = *rest == ' ' && number(p, 0, &n) && *p == '\0' && n >= 10 && n <= 3600;
        c.command = ok ? Command::TimeJitter : Command::InvalidArgument;
        c.number = n;
    } else if (starts_with(norm, "data block", &rest) || starts_with(norm, "data unblock", &rest)) {
        // "data block <imsakiyem|emushaf|aladhan>", "data unblock <name|all>" (spec 007 R10)
        const bool block = norm[5] == 'b';
        c.command = Command::InvalidArgument;
        if (*rest != ' ') return c;
        const char* name = rest + 1;
        const char* names[4] = {"all", "imsakiyem", "emushaf", "aladhan"};
        for (int i = block ? 1 : 0; i < 4; ++i) {
            if (std::strcmp(name, names[i]) == 0) {
                c.number = i;
                c.command = block ? Command::DataBlock : Command::DataUnblock;
            }
        }
    } else if (starts_with(norm, "fixture", &rest)) {
        const char* p = rest + 1;
        const bool ok = *rest == ' ' && number(p, 0, &n) && *p == '\0' && n >= 1 && n <= 17;  // 15-17: spec 004 screens
        c.command = ok ? Command::Fixture : Command::InvalidArgument;
        c.number = n;
    } else if (starts_with(norm, "state", &rest)) {
        c.command = Command::InvalidArgument;
        if (*rest != ' ') return c;
        char name[24];
        const char* p = rest + 1;
        size_t len = 0;
        while (*p != '\0' && *p != ' ' && len + 1 < sizeof name) name[len++] = *p++;
        name[len] = '\0';
        screen::SystemState st;
        if (!screen::parse_state_name(name, &st)) return c;
        if (st == screen::SystemState::CacheEnding) {
            if (*p != ' ') return c;
            ++p;
            if (!number(p, 0, &n) || *p != '\0' || n > 7) return c;
            c.number = n;
        } else if (*p != '\0') {
            return c;
        }
        c.state = st;
        c.command = Command::StateForce;
    }
    return c;
}

}  // namespace nvp::diag
