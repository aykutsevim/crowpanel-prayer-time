#include "storage.h"

#include <esp_littlefs.h>

#include <cstdio>
#include <cstring>

#include "sysinfo.h"

namespace platform::storage {
namespace {

constexpr const char* kBasePath = "/littlefs";
constexpr const char* kPartitionLabel = "littlefs";  // partitions/crowpanel7_advance.csv
constexpr const char* kSelfTestFile = "/littlefs/selftest.bin";
constexpr const char* kScratchFile = "/littlefs/scratch.bin";

bool g_mounted = false;

}  // namespace

Result mount() {
    sysinfo::note_peripheral_init();
    // Formats this partition only, and only when it cannot be mounted (joltwallet/littlefs 1.22.3).
    esp_vfs_littlefs_conf_t conf = {};
    conf.base_path = kBasePath;
    conf.partition_label = kPartitionLabel;
    conf.format_if_mount_failed = true;
    g_mounted = esp_vfs_littlefs_register(&conf) == ESP_OK;
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
    FILE* out = std::fopen(kSelfTestFile, "wb");
    if (out == nullptr) {
        return {false, "open for write failed"};
    }
    const size_t written = std::fwrite(pattern, 1, sizeof pattern, out);
    std::fclose(out);
    if (written != sizeof pattern) {
        return {false, "write failed"};
    }
    FILE* in = std::fopen(kSelfTestFile, "rb");
    if (in == nullptr) {
        return {false, "open for read failed"};
    }
    uint8_t back[sizeof pattern] = {};
    const size_t read = std::fread(back, 1, sizeof back, in);
    std::fclose(in);
    if (read != sizeof back) {
        return {false, "read failed"};
    }
    if (std::memcmp(pattern, back, sizeof pattern) != 0) {
        return {false, "read-back mismatch"};
    }
    return {true, ""};
}

Result scratch_write(uint32_t counter) {
    if (!g_mounted) return {false, "not mounted"};
    static uint8_t block[4096];
    for (size_t i = 0; i < sizeof block; i += 4) {
        const uint32_t v = counter + static_cast<uint32_t>(i);
        std::memcpy(block + i, &v, 4);
    }
    FILE* f = std::fopen(kScratchFile, "wb");  // truncates: the file stays 4 kB
    if (f == nullptr) return {false, "scratch open failed"};
    const size_t written = std::fwrite(block, 1, sizeof block, f);
    std::fclose(f);
    if (written != sizeof block) return {false, "scratch write failed"};
    return {true, ""};
}

}  // namespace platform::storage
