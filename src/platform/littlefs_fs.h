// nvp::store::FileSystem over the mounted LittleFS (spec 006 T017, research R5). Names are relative
// to "/littlefs" (platform/storage mounts it). stdio on the esp_littlefs VFS: data is committed on
// fclose, rename() is atomic and replaces the destination (littlefs 2.11 README, lfs.h). esp_littlefs
// refuses a rename while either file is open, so the store never keeps a file open between calls.
// Network worker only.
#pragma once

#include "nvp/store/file_system.h"

namespace platform::littlefs_fs {

class Fs : public nvp::store::FileSystem {
public:
    int open(const char* path, nvp::store::OpenMode mode) override;
    long read(int handle, void* buf, size_t n) override;
    bool write(int handle, const void* buf, size_t n) override;
    bool close(int handle) override;
    bool rename(const char* from, const char* to) override;
    bool remove(const char* path) override;
    bool exists(const char* path) override;
};

// Flips one byte in the middle of the file (`data corrupt`, development builds). False on failure.
bool flip_middle_byte(const char* path);

}  // namespace platform::littlefs_fs
