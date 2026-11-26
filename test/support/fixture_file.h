// Host test helper (spec 006 T002): reads a recorded provider answer from test/fixtures/providers/ into memory.
// Paths are relative to the project root, the working directory of `pio test` (as
// test/test_screen/test_fixtures.cpp already relies on). Header-only, because PlatformIO builds one
// test folder per program; include it as "../support/fixture_file.h".
#pragma once

#include <cstdio>
#include <string>

namespace nvp_test {

// The whole file, or an empty string when it cannot be read (the caller fails the test).
inline std::string read_fixture(const char* relative_path) {
    std::string path = std::string("test/fixtures/providers/") + relative_path;
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) return std::string();
    std::string out;
    char buf[4096];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) out.append(buf, n);
    std::fclose(f);
    return out;
}

// Replaces the first occurrence of `from` with `to`; returns false when `from` is absent.
inline bool replace_first(std::string& s, const std::string& from, const std::string& to) {
    size_t at = s.find(from);
    if (at == std::string::npos) return false;
    s.replace(at, from.size(), to);
    return true;
}

}  // namespace nvp_test
