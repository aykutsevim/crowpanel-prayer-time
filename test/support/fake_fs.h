// Host test helper (spec 006 T002, T008, research R5): an in-memory nvp::store::FileSystem with the
// littlefs semantics the store relies on:
//   - data written to a file becomes durable only when the file is closed (commit on close);
//   - rename and remove are atomic, and rename replaces an existing destination;
//   - a "power cut" at call N makes call N and every later call fail; remount() then drops every
//     open handle and its uncommitted data, as a restart would.
// fail_once_at(N) instead makes only call N fail and lets the next calls succeed (an I/O error).
// Header-only; include it as "../support/fake_fs.h".
#pragma once

#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "nvp/store/file_system.h"

namespace nvp_test {

class FakeFs : public nvp::store::FileSystem {
public:
    // Durable contents, by path.
    std::map<std::string, std::string> files;

    int calls = 0;       // calls made since the last reset_counter()
    int cut_at = 0;      // power cut at this call (1-based); 0 = never
    int fail_at = 0;     // single failure at this call; 0 = never
    bool dead = false;   // after a power cut

    void reset_counter() { calls = 0; }
    void power_cut_at(int n) { cut_at = n; dead = false; calls = 0; }
    void fail_once_at(int n) { fail_at = n; calls = 0; }
    void remount() {
        handles_.clear();
        dead = false;
        cut_at = 0;
        fail_at = 0;
        calls = 0;
    }
    size_t open_handles() const {
        size_t n = 0;
        for (const auto& h : handles_) n += h.used ? 1 : 0;
        return n;
    }

    int open(const char* path, nvp::store::OpenMode mode) override {
        if (!step()) return -1;
        Handle h;
        h.used = true;
        h.path = path;
        h.write = mode == nvp::store::OpenMode::Write;
        if (!h.write) {
            auto it = files.find(path);
            if (it == files.end()) return -1;
            h.data = it->second;
        }
        for (size_t i = 0; i < handles_.size(); ++i) {
            if (!handles_[i].used) {
                handles_[i] = h;
                return static_cast<int>(i);
            }
        }
        handles_.push_back(h);
        return static_cast<int>(handles_.size() - 1);
    }

    long read(int fd, void* buf, size_t n) override {
        if (!step()) return -1;
        Handle* h = get(fd);
        if (h == nullptr || h->write) return -1;
        size_t left = h->data.size() - h->pos;
        size_t k = n < left ? n : left;
        std::memcpy(buf, h->data.data() + h->pos, k);
        h->pos += k;
        return static_cast<long>(k);
    }

    bool write(int fd, const void* buf, size_t n) override {
        if (!step()) return false;
        Handle* h = get(fd);
        if (h == nullptr || !h->write) return false;
        h->data.append(static_cast<const char*>(buf), n);
        return true;
    }

    bool close(int fd) override {
        Handle* h = get(fd);
        if (!step()) {
            // A failed close still frees the file, without committing it (lfs_file_close always
            // deallocates the file; its data is lost).
            if (h != nullptr && !dead) h->used = false;
            return false;
        }
        if (h == nullptr) return false;
        if (h->write) files[h->path] = h->data;  // commit on close
        h->used = false;
        return true;
    }

    bool rename(const char* from, const char* to) override {
        if (!step()) return false;
        auto it = files.find(from);
        if (it == files.end()) return false;
        files[to] = it->second;
        files.erase(from);
        return true;
    }

    bool remove(const char* path) override {
        if (!step()) return false;
        return files.erase(path) > 0;
    }

    bool exists(const char* path) override {
        if (!step()) return false;
        return files.count(path) > 0;
    }

private:
    struct Handle {
        bool used = false;
        bool write = false;
        std::string path;
        std::string data;
        size_t pos = 0;
    };
    std::vector<Handle> handles_;

    bool step() {
        if (dead) return false;
        ++calls;
        if (cut_at > 0 && calls >= cut_at) {
            dead = true;
            return false;
        }
        if (fail_at > 0 && calls == fail_at) return false;
        return true;
    }
    Handle* get(int fd) {
        if (fd < 0 || static_cast<size_t>(fd) >= handles_.size() || !handles_[fd].used) return nullptr;
        return &handles_[fd];
    }
};

}  // namespace nvp_test
