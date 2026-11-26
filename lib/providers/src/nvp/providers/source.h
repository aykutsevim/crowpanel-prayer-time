// The provider model and the abstract source the refresh runner asks (spec 007 T008, FR-001,
// research R6, data-model "Provider"). Pure C++.
//
// A Source fetches one provider's records for a date range, parses them while they stream in and
// validates the batch (spec 006 rules); it returns only a valid batch. The device implements it over
// HTTPS (http_source.h); the host tests script it (test/support/scripted_source.h), so the runner and
// the planner are tested without any parser.
#pragma once
#include <cstddef>
#include <cstdint>

#include "nvp/engine/calendar.h"
#include "nvp/engine/day_record.h"
#include "nvp/providers/validate.h"

namespace nvp::providers {

// In the order of 04 §5.2. Diyanet is the official API: an empty slot in this feature (FR-001).
enum class ProviderId : uint8_t { Diyanet, Imsakiyem, Emushaf, Aladhan, Local };
inline constexpr size_t kProviderCount = 5;
inline constexpr size_t kFetchedProviders = 4;  // Diyanet..Aladhan; Local is computed, never fetched

engine::SourceId source_id(ProviderId p);
const char* provider_key(ProviderId p);           // "imsakiyem"
bool provider_from_key(const char* key, ProviderId& out);
inline bool diyanet_derived(ProviderId p) { return engine::rank(source_id(p)) >= 40; }

enum class FetchOutcome : uint8_t {
    Ok,           // a valid batch
    Failed,       // no answer, a bad status, malformed or rejected data (reason says which)
    RateLimited,  // HTTP 429 or 503: do not call again for 30 minutes (FR-011)
    NotFound,     // the provider does not know the district
};
const char* fetch_outcome_text(FetchOutcome o);

struct FetchRequest {
    ProviderId provider = ProviderId::Imsakiyem;
    uint32_t district = 0;
    engine::Date today;
    engine::Date from;
    engine::Date to;
    bool allow_fallback = true;  // İmsakiyem: v1 after a v2 failure (off for the hourly retry, analysis I2)
    bool has_coords = false;
    int32_t lat_e5 = 0;
    int32_t lon_e5 = 0;
};

struct FetchResult {
    FetchOutcome outcome = FetchOutcome::Failed;
    Reason reason = Reason::Network;
    int http_status = 0;
    uint8_t requests = 0;  // HTTP requests made (İmsakiyem v2 + v1 = 2)
    size_t count = 0;      // records written to `out` (only when Ok)
};

class Source {
public:
    virtual ~Source() = default;
    // The most requests one fetch of this provider may make (for the cap of 12, FR-008).
    virtual uint8_t max_requests(ProviderId p) const { return p == ProviderId::Imsakiyem ? 2 : 1; }
    virtual FetchResult fetch(const FetchRequest& req, engine::DayRecord* out, size_t cap) = 0;
};

}  // namespace nvp::providers
