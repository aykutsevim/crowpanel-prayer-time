// The refresh flow (spec 006 T014a, FR-005..FR-009, research R1, R4..R6, analysis D1). Pure:
// it talks to an abstract HttpGet and an abstract FileSystem, so the whole flow is tested on the
// host. The device (src/app/data_service) only wires it to HTTPS and LittleFS.
//
//   1. İmsakiyem v2, today .. today + 365 days.
//   2. On a v2 failure other than DISTRICT_NOT_FOUND (no answer, another status, or an answer that
//      fails validation): İmsakiyem v1 range, today .. today + 29 days.
//   3. The first valid batch is merged into a copy of the store and saved atomically. Only when
//      the save succeeds does the caller's store change.
#pragma once

#include <cstddef>
#include <cstdint>

#include "nvp/engine/calendar.h"
#include "nvp/engine/day_record.h"
#include "nvp/providers/http_get.h"
#include "nvp/providers/validate.h"
#include "nvp/store/file_system.h"
#include "nvp/store/merge.h"
#include "nvp/store/store.h"

namespace nvp::providers {

enum class Provider : uint8_t { None, ImsakiyemV2, ImsakiyemV1 };
const char* provider_text(Provider p);  // "imsakiyem v2"

inline constexpr int32_t kV2Days = 365;  // the v2 limit is 366 days (R1)
inline constexpr int32_t kV1Days = 30;   // one v1 page (R1)

struct RefreshRequest {
    uint32_t district = 0;
    engine::Date today;
    int64_t now_utc = 0;
};

struct Attempt {
    bool tried = false;
    HttpResult http;
    Reason reason = Reason::Ok;
    uint32_t records = 0;  // records parsed
};

struct RefreshResult {
    bool stored = false;              // the store and its file now hold the merged batch
    Provider provider = Provider::None;  // the provider of the last attempt
    Reason reason = Reason::Ok;       // of the last attempt; Ok when stored or when the save failed
    store::SaveResult save = store::SaveResult::Ok;
    store::MergeStats merge;
    Attempt v2;
    Attempt v1;
};

// Big buffers, owned by the caller (on the device, in PSRAM).
struct RefreshScratch {
    engine::DayRecord incoming[kMaxBatchRecords];
    store::Store merged;
};

// Called right before the store file is written, so the device can wait for a running Wi-Fi scan
// (FR-011). Optional.
class SaveGate {
public:
    virtual ~SaveGate() = default;
    virtual void before_save() = 0;
};

// Writes the request URL. Returns false when cap is too small.
bool v2_url(uint32_t district, engine::Date from, char* out, size_t cap);
bool v1_url(uint32_t district, engine::Date from, char* out, size_t cap);

// `s` must be the store for req.district (an empty one with that district id when there is none).
RefreshResult refresh_with(HttpGet& http, store::FileSystem& fs, const RefreshRequest& req, store::Store& s,
                           RefreshScratch& scratch, SaveGate* gate = nullptr);

}  // namespace nvp::providers
