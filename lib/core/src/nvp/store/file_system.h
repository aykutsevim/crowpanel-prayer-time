// The file-system interface the day-record store is written against (spec 006 research R5).
// The device implements it over the mounted LittleFS (src/platform/littlefs_fs); the host tests
// use an in-memory fake that can cut power at any call (test/support/fake_fs.h). Paths are plain
// file names such as "store_9206.bin"; the implementation adds its mount point. Pure C++.
//
// Semantics the store relies on (littlefs README, research R5):
//   - data written to a file is committed when the file is closed;
//   - rename() and remove() are atomic, and rename() replaces an existing destination.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvp::store {

enum class OpenMode : uint8_t {
    Read,
    Write,  // create or truncate
};

class FileSystem {
public:
    virtual ~FileSystem() = default;
    virtual int open(const char* path, OpenMode mode) = 0;        // a handle >= 0, or -1
    virtual long read(int handle, void* buf, size_t n) = 0;       // bytes read (0 at the end), -1 on error
    virtual bool write(int handle, const void* buf, size_t n) = 0;  // all n bytes, or false
    virtual bool close(int handle) = 0;
    virtual bool rename(const char* from, const char* to) = 0;    // replaces `to`
    virtual bool remove(const char* path) = 0;
    virtual bool exists(const char* path) = 0;
};

}  // namespace nvp::store
