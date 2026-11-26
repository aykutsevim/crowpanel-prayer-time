#include "nvp/store/atomic_file.h"

namespace nvp::store {
namespace {

SaveResult abandon(FileSystem& fs, int h, const char* tmp, SaveResult why) {
    if (h >= 0) fs.close(h);
    fs.remove(tmp);  // best effort; the loaders remove a leftover temporary file too
    return why;
}

}  // namespace

const char* save_result_text(SaveResult r) {
    switch (r) {
        case SaveResult::Ok: return "ok";
        case SaveResult::Empty: return "empty";
        case SaveResult::OpenFailed: return "open failed";
        case SaveResult::WriteFailed: return "write failed";
        case SaveResult::CloseFailed: return "close failed";
        case SaveResult::VerifyFailed: return "verify failed";
        case SaveResult::RenameFailed: return "rename failed";
    }
    return "?";
}

uint32_t crc32(uint32_t crc, const void* data, size_t n) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    crc = ~crc;
    for (size_t i = 0; i < n; ++i) {
        crc ^= p[i];
        for (int k = 0; k < 8; ++k) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

bool read_all(FileSystem& fs, int h, void* buf, size_t n) {
    uint8_t* p = static_cast<uint8_t*>(buf);
    size_t got = 0;
    while (got < n) {
        long k = fs.read(h, p + got, n - got);
        if (k <= 0) return false;
        got += static_cast<size_t>(k);
    }
    return true;
}

bool AtomicWriter::write(const void* data, size_t n) {
    if (!fs_.write(handle_, data, n)) return false;
    crc_ = crc32(crc_, data, n);
    length_ += static_cast<uint32_t>(n);
    return true;
}

SaveResult atomic_write(FileSystem& fs, const char* final_name, const char* tmp, Produce produce, void* ctx) {
    // 1. Write the new file completely and close it.
    if (fs.exists(tmp) && !fs.remove(tmp)) return SaveResult::OpenFailed;
    int h = fs.open(tmp, OpenMode::Write);
    if (h < 0) return abandon(fs, -1, tmp, SaveResult::OpenFailed);
    AtomicWriter w(fs, h);
    if (!produce(ctx, w)) return abandon(fs, h, tmp, SaveResult::WriteFailed);
    if (!fs.close(h)) return abandon(fs, -1, tmp, SaveResult::CloseFailed);

    // 2. Read it back: same length, same CRC.
    h = fs.open(tmp, OpenMode::Read);
    if (h < 0) return abandon(fs, -1, tmp, SaveResult::VerifyFailed);
    uint8_t chunk[320];
    uint32_t check = 0, left = w.length();
    while (left > 0) {
        const size_t k = left < sizeof chunk ? left : sizeof chunk;
        if (!read_all(fs, h, chunk, k)) return abandon(fs, h, tmp, SaveResult::VerifyFailed);
        check = crc32(check, chunk, k);
        left -= static_cast<uint32_t>(k);
    }
    uint8_t extra;
    if (fs.read(h, &extra, 1) != 0 || check != w.crc()) return abandon(fs, h, tmp, SaveResult::VerifyFailed);
    if (!fs.close(h)) return abandon(fs, -1, tmp, SaveResult::VerifyFailed);

    // 3. Atomic replace.
    if (!fs.rename(tmp, final_name)) return abandon(fs, -1, tmp, SaveResult::RenameFailed);
    return SaveResult::Ok;
}

}  // namespace nvp::store
