// The persisted refresh state (spec 007 FR-014, research R7, data-model "Planner state",
// 04-data-sources §8). Pure C++, no heap.
//
// File "refresh.bin", little-endian, replaced atomically by atomic_write:
//   0 "NVPR" | 4 u16 version = 1 | 6 u16 payload length | 8 u32 CRC-32 of the payload | 12 payload
// A missing or bad file loads as the defaults ("never refreshed"), so the boot rules refresh at once.
#pragma once
#include <cstddef>
#include <cstdint>

#include "nvp/store/atomic_file.h"
#include "nvp/store/file_system.h"
#include "nvp/store/store.h"

namespace nvp::store {

inline constexpr uint16_t kRefreshStateVersion = 1;
inline constexpr size_t kProviderSlots = 4;  // Diyanet, Imsakiyem, Emushaf, Aladhan (providers::ProviderId order)
inline constexpr const char* kRefreshStateFile = "refresh.bin";
inline constexpr const char* kRefreshStateTemp = "refresh.new";

enum class RefreshOutcome : uint8_t { None, Complete, Partial, Failed, Skipped };
const char* refresh_outcome_text(RefreshOutcome o);  // "complete", ...

struct ProviderState {
    uint8_t failures = 0;          // refreshes in a row in which this provider failed (FR-013)
    int64_t cool_down_until = 0;   // UTC s; 30 min after 429/503 (FR-011)
    int64_t skip_until = 0;        // UTC s; 24 h after the fifth failed refresh (FR-013)
};

struct Calibration {
    bool valid = false;
    int8_t offsets[6] = {};   // published minus calculated, minutes (04 §3.5)
    uint8_t days = 0;         // Diyanet-derived days used
    uint32_t district = 0;    // the district the offsets belong to
    int8_t hijri_offset = 0;  // days between the tabular Hijri date and Diyanet's (research R8)
};

struct RefreshState {
    int64_t last_success_utc = 0;  // the end of the last refresh that stored Diyanet-derived records
    int64_t last_attempt_utc = 0;
    RefreshOutcome last_outcome = RefreshOutcome::None;
    uint8_t retry_step = 0;        // 0 none; 1..4 = 5 min, 15 min, 1 h, 3 h; 5 = every 6 h (FR-012)
    int64_t next_retry_utc = 0;    // 0 when no retry is due
    ProviderState providers[kProviderSlots];
    Calibration calibration;
    bool coords_suspect = false;   // FR-016 exception
};

bool operator==(const RefreshState& a, const RefreshState& b);

// Ok, Missing, BadMagic, BadVersion, BadCrc, Truncated or ReadError; on anything but Ok `out` is
// the defaults. Removes a leftover temporary file.
LoadResult load_refresh_state(FileSystem& fs, RefreshState& out);
SaveResult save_refresh_state(FileSystem& fs, const RefreshState& s);

}  // namespace nvp::store
