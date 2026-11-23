// Device storage (LittleFS on the partition labelled "littlefs", partitions/crowpanel7_advance.csv).
// Slice 1 uses it only for the storage self-test and, in stress mode, a bounded scratch file
// (FR-013). Nothing else is written.
#pragma once

#include <cstddef>

namespace platform::storage {

struct Result {
    bool ok = false;
    const char* reason = "";  // short, static text
};

// Mounts the file system. On failure it formats only the "littlefs" partition, which can take
// several seconds, so it runs after the diagnostic screen is up (plan startup step 7).
Result mount();

// Writes 16 bytes to /selftest.bin, reads them back and compares them.
Result self_test();

}  // namespace platform::storage
