// Batch validation (spec 006 FR-006, research R4, DATA-06, 04-data-sources §7). Pure.
// A batch is accepted or rejected as a whole; the first failing rule gives the reason.
#pragma once

#include <cstddef>
#include <cstdint>

#include "nvp/engine/day_record.h"
#include "nvp/providers/imsakiyem.h"

namespace nvp::providers {

inline constexpr size_t kMaxBatchRecords = 400;
inline constexpr int32_t kMaxDaysAhead = 400;
inline constexpr int32_t kMaxDaysBehind = 2;

enum class Reason : uint8_t {
    Ok,
    Network,        // no answer (connection, TLS, timeout); set by the refresh flow
    Http,           // an HTTP status other than 200 without an error code; set by the refresh flow
    NotSuccess,     // success missing or false
    ErrorCode,      // success false with error.code (for example DISTRICT_NOT_FOUND)
    Malformed,      // malformed JSON, no records array, or a needed field too long
    Truncated,      // the JSON ended early
    WrongDistrict,  // the answer is for another place
    WrongSource,    // not Diyanet-published (constitution II)
    Empty,          // no records
    NoFutureDate,   // no record from today on
    BadRecord,      // a record breaks ENG-01
    BadDate,        // a date missing or malformed
    DuplicateDate,
    DateTooFar,     // more than 400 days after today
    DateTooOld,     // more than 2 days before today
    TooMany,        // more than 400 records
    Implausible,    // spec 007 FR-016: over 20 minutes from the calculation for the place
};

// Short English text for logs, for example "wrong district".
const char* reason_text(Reason r);

Reason validate_batch(const engine::DayRecord* records, size_t n, const BatchInfo& info,
                      uint32_t requested_district, engine::Date today);

}  // namespace nvp::providers
