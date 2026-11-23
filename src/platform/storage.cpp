#include "storage.h"

#include <FS.h>
#include <LittleFS.h>

#include <cstring>

#include "sysinfo.h"

namespace platform::storage {
namespace {

constexpr const char* kBasePath = "/littlefs";
constexpr const char* kPartitionLabel = "littlefs";  // partitions/crowpanel7_advance.csv
constexpr uint8_t kMaxOpenFiles = 4;
constexpr const char* kSelfTestFile = "/selftest.bin";

bool g_mounted = false;

}  // namespace

Result mount() {
    sysinfo::note_peripheral_init();
    // formatOnFail = true formats this partition only (Arduino-ESP32 3.3.12 LittleFS::begin).
    g_mounted = LittleFS.begin(true, kBasePath, kMaxOpenFiles, kPartitionLabel);
    if (!g_mounted) {
        return {false, "mount failed"};
    }
    return {true, ""};
}

Result self_test() {
    if (!g_mounted) {
        return {false, "not mounted"};
    }
    uint8_t pattern[16];
    for (size_t i = 0; i < sizeof pattern; ++i) {
        pattern[i] = static_cast<uint8_t>(0xA5 ^ (i * 17));
    }
    File out = LittleFS.open(kSelfTestFile, FILE_WRITE);
    if (!out) {
        return {false, "open for write failed"};
    }
    const size_t written = out.write(pattern, sizeof pattern);
    out.close();
    if (written != sizeof pattern) {
        return {false, "write failed"};
    }
    File in = LittleFS.open(kSelfTestFile, FILE_READ);
    if (!in) {
        return {false, "open for read failed"};
    }
    uint8_t back[sizeof pattern] = {};
    const size_t read = in.read(back, sizeof back);
    in.close();
    if (read != sizeof back) {
        return {false, "read failed"};
    }
    if (std::memcmp(pattern, back, sizeof pattern) != 0) {
        return {false, "read-back mismatch"};
    }
    return {true, ""};
}

}  // namespace platform::storage
