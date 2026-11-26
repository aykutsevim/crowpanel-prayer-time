// One refresh: the ordered steps of 04 §5.2 over an abstract Source (spec 007 FR-001, FR-002,
// FR-007, FR-008, FR-010..FR-013, research R6). Pure C++, no heap; tested on the host with scripted
// sources (test/test_planner).
//
//   1. Diyanet (official API): an empty slot in this feature.
//   2. İmsakiyem, yesterday .. yesterday + 365 (v2, then v1 inside the source unless `lite`).
//   3. emushaf, when fewer than 30 Diyanet-derived days ahead are stored or İmsakiyem failed.
//   4. Aladhan, for the current month (and the next within 7 days of month end) when today or
//      tomorrow has no Diyanet-derived record, only for a month with a day that has no record at all,
//      and only with coordinates.
// The device's own calculation (Local) is not a step: it fills today .. today + 7 in RAM outside
// the refresh (research R8).
//
// Each batch is merged into a copy (rank before fetch time, so published days are never replaced
// by calculated ones); the copy is saved once at the end if anything changed, and the caller's
// store changes only after the save succeeds. The per-provider failure counters and cool-downs in
// the refresh state are updated here; the retry schedule is the planner's.
#pragma once
#include <cstddef>
#include <cstdint>

#include "nvp/engine/calendar.h"
#include "nvp/engine/day_record.h"
#include "nvp/providers/plausible.h"
#include "nvp/providers/source.h"
#include "nvp/providers/validate.h"
#include "nvp/store/file_system.h"
#include "nvp/store/merge.h"
#include "nvp/store/refresh_state.h"
#include "nvp/store/store.h"

namespace nvp::providers {

inline constexpr int32_t kGoalDaysAhead = 30;         // 04 §5.2
inline constexpr uint8_t kMaxRequestsPerRefresh = 12;  // 04 §5.2, FR-008
inline constexpr int64_t kRateLimitCoolDown = 30 * 60;  // FR-011
inline constexpr int64_t kSkipAfterFailures = 24 * 3600;  // FR-013
inline constexpr uint8_t kFailuresBeforeSkip = 5;
inline constexpr int32_t kImsakiyemDays = 365;
inline constexpr int32_t kEmushafDays = 30;

struct RunInput {
    uint32_t district = 0;
    engine::Date today;
    int64_t now_utc = 0;
    bool has_coords = false;
    int32_t lat_e5 = 0;
    int32_t lon_e5 = 0;
    bool today_calculated = false;  // Diyanet-derived providers ignore the 24 h skip (FR-013 exception)
    bool lite = false;              // the hourly retry: İmsakiyem without its v1 fallback (analysis I2)
    uint8_t blocked = 0;            // bit (1 << ProviderId): development block (R10)
    int32_t utc_offset_s = 3 * 3600;  // for the plausibility check (the calculation is in local time)
};

enum class Skip : uint8_t { None, NotNeeded, Disabled, Blocked, CoolingDown, Skipped24h, NoCoords, Cap, Covered };
const char* skip_text(Skip s);

struct ProviderReport {
    bool asked = false;
    Skip skip = Skip::NotNeeded;
    FetchResult result;   // of the last fetch (Aladhan may fetch two months)
    uint16_t records = 0;  // records merged
    Plausibility plausibility;  // Diyanet-derived batches with coordinates (FR-016)
};

struct RunResult {
    store::RefreshOutcome outcome = store::RefreshOutcome::Skipped;
    bool abandoned = false;        // should_continue() said stop; nothing stored
    bool diyanet_stored = false;   // a Diyanet-derived batch is now in the store
    bool changed = false;          // the merged copy differed and a save was attempted
    bool saved = false;
    store::SaveResult save = store::SaveResult::Ok;
    uint8_t requests = 0;
    int32_t days_ahead = -1;       // Diyanet-derived days ahead after the run (-1: none for today)
    uint16_t mismatches = 0;
    bool has_mismatch = false;
    engine::DayRecord mismatch_stored;
    engine::DayRecord mismatch_incoming;
    bool coords_suspect_set = false;  // FR-016 exception: two mirrors agreed, both implausible
    ProviderReport providers[kProviderCount];
};

struct RunScratch {
    engine::DayRecord incoming[kMaxBatchRecords];
    engine::DayRecord held[kMaxBatchRecords];  // İmsakiyem's implausible batch, kept for the FR-016 exception
    store::Store merged;
};

class RunControl {
public:
    virtual ~RunControl() = default;
    // Checked before each request: false abandons the run without storing (a location change).
    virtual bool should_continue() { return true; }
    // Right before the store file is written (the device waits for a Wi-Fi scan).
    virtual void before_save() {}
};

// Days after today covered by consecutive Diyanet-derived records, today included as day 0:
// -1 when today has none, 30 when today .. today + 30 are all Diyanet-derived.
int32_t diyanet_days_ahead(const store::Store& s, engine::Date today);

RunResult refresh_run(Source& source, store::FileSystem& fs, const RunInput& in, store::Store& s,
                      RunScratch& scratch, store::RefreshState& state, RunControl* control = nullptr);

}  // namespace nvp::providers
