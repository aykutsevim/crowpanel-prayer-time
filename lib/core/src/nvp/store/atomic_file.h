// Atomic file replace on the FileSystem interface (spec 006 research R5, spec 007 research R7,
// DATA-07). Shared by the day-record store and the refresh state. Pure C++, no heap.
//
//   1. remove a leftover temporary file, then write the temporary file completely and close it
//      (littlefs commits on close);
//   2. read it back: the same length and the same CRC-32 as what was written;
//   3. rename it over the final file (atomic in littlefs).
// A power cut at any call leaves exactly the old or exactly the new final file.
#pragma once
#include <cstddef>
#include <cstdint>

#include "nvp/store/file_system.h"

namespace nvp::store {

enum class SaveResult : uint8_t { Ok, Empty, OpenFailed, WriteFailed, CloseFailed, VerifyFailed, RenameFailed };
const char* save_result_text(SaveResult r);

// Given to the producer; collects the CRC and the length of what it writes.
class AtomicWriter {
public:
    AtomicWriter(FileSystem& fs, int handle) : fs_(fs), handle_(handle) {}
    bool write(const void* data, size_t n);
    uint32_t crc() const { return crc_; }
    uint32_t length() const { return length_; }

private:
    FileSystem& fs_;
    int handle_;
    uint32_t crc_ = 0;
    uint32_t length_ = 0;
};

// Writes the whole file content; false when a write failed.
using Produce = bool (*)(void* ctx, AtomicWriter& w);

SaveResult atomic_write(FileSystem& fs, const char* final_name, const char* temp_name, Produce produce, void* ctx);

// Reads exactly n bytes; false on an error or a short read.
bool read_all(FileSystem& fs, int handle, void* buf, size_t n);

// IEEE 802.3 CRC-32 (reflected, 0xEDB88320), continued from `crc` (start with 0).
uint32_t crc32(uint32_t crc, const void* data, size_t n);

}  // namespace nvp::store
