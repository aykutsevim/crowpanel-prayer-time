#include "nvp/diag/command.h"

#include <cstddef>
#include <cstring>

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

}  // namespace nvp::diag
