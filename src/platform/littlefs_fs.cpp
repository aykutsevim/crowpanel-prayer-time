#include "littlefs_fs.h"

#include <sys/stat.h>

#include <cstdio>

namespace platform::littlefs_fs {
namespace {

constexpr int kMaxOpen = 2;
std::FILE* g_files[kMaxOpen] = {};

void full_path(const char* name, char* out, size_t cap) { std::snprintf(out, cap, "/littlefs/%s", name); }

std::FILE* get(int h) { return (h >= 0 && h < kMaxOpen) ? g_files[h] : nullptr; }

}  // namespace

int Fs::open(const char* path, nvp::store::OpenMode mode) {
    int slot = -1;
    for (int i = 0; i < kMaxOpen; ++i) {
        if (g_files[i] == nullptr) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;
    char p[64];
    full_path(path, p, sizeof p);
    std::FILE* f = std::fopen(p, mode == nvp::store::OpenMode::Write ? "wb" : "rb");
    if (f == nullptr) return -1;
    g_files[slot] = f;
    return slot;
}

long Fs::read(int h, void* buf, size_t n) {
    std::FILE* f = get(h);
    if (f == nullptr) return -1;
    const size_t k = std::fread(buf, 1, n, f);
    if (k == 0 && std::ferror(f)) return -1;
    return static_cast<long>(k);
}

bool Fs::write(int h, const void* buf, size_t n) {
    std::FILE* f = get(h);
    return f != nullptr && std::fwrite(buf, 1, n, f) == n;
}

bool Fs::close(int h) {
    std::FILE* f = get(h);
    if (f == nullptr) return false;
    g_files[h] = nullptr;
    return std::fclose(f) == 0;  // littlefs commits here
}

bool Fs::rename(const char* from, const char* to) {
    char a[64], b[64];
    full_path(from, a, sizeof a);
    full_path(to, b, sizeof b);
    return std::rename(a, b) == 0;
}

bool Fs::remove(const char* path) {
    char p[64];
    full_path(path, p, sizeof p);
    return std::remove(p) == 0;
}

bool Fs::exists(const char* path) {
    char p[64];
    full_path(path, p, sizeof p);
    struct stat st;
    return ::stat(p, &st) == 0;
}

bool flip_middle_byte(const char* path) {
    char p[64];
    full_path(path, p, sizeof p);
    std::FILE* f = std::fopen(p, "r+b");
    if (f == nullptr) return false;
    bool ok = std::fseek(f, 0, SEEK_END) == 0;
    const long size = ok ? std::ftell(f) : -1;
    ok = ok && size > 0 && std::fseek(f, size / 2, SEEK_SET) == 0;
    int c = ok ? std::fgetc(f) : EOF;
    ok = ok && c != EOF && std::fseek(f, size / 2, SEEK_SET) == 0 && std::fputc(c ^ 0xFF, f) != EOF;
    return std::fclose(f) == 0 && ok;
}

}  // namespace platform::littlefs_fs
