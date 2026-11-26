#include "nvp/providers/validate.h"

#include "nvp/engine/calendar.h"

namespace nvp::providers {

const char* reason_text(Reason r) {
    switch (r) {
        case Reason::Ok: return "ok";
        case Reason::Network: return "no answer";
        case Reason::Http: return "http status";
        case Reason::NotSuccess: return "not success";
        case Reason::ErrorCode: return "error code";
        case Reason::Malformed: return "malformed";
        case Reason::Truncated: return "truncated";
        case Reason::WrongDistrict: return "wrong district";
        case Reason::WrongSource: return "not diyanet";
        case Reason::Empty: return "empty";
        case Reason::NoFutureDate: return "no date from today";
        case Reason::BadRecord: return "bad record";
        case Reason::BadDate: return "bad date";
        case Reason::DuplicateDate: return "duplicate date";
        case Reason::DateTooFar: return "date too far";
        case Reason::DateTooOld: return "date too old";
        case Reason::TooMany: return "too many records";
    }
    return "?";
}

Reason validate_batch(const engine::DayRecord* records, size_t n, const BatchInfo& info,
                      uint32_t requested_district, engine::Date today) {
    // The answer as a whole.
    if (info.json_error == JsonError::Truncated) return Reason::Truncated;
    if (info.json_error != JsonError::None || !info.complete) return Reason::Malformed;
    if (!info.success_seen || !info.success) return info.error_code[0] != '\0' ? Reason::ErrorCode : Reason::NotSuccess;
    if (info.field_overflow || !info.records_array_seen) return Reason::Malformed;
    if (!info.district_seen || info.district_mixed || info.district != requested_district) return Reason::WrongDistrict;
    if (!info.source_diyanet) return Reason::WrongSource;
    if (info.records_seen > kMaxBatchRecords || n > kMaxBatchRecords) return Reason::TooMany;
    if (info.records_seen != n) return Reason::Malformed;  // an element that was not an object
    if (n == 0) return Reason::Empty;

    // Each record (ENG-01, dates).
    const engine::DayNumber t = engine::to_day_number(today);
    bool future = false;
    for (size_t i = 0; i < n; ++i) {
        const engine::DayRecord& r = records[i];
        if (!engine::valid(r.date)) return Reason::BadDate;
        if (!engine::valid(r)) return Reason::BadRecord;
        const engine::DayNumber d = engine::to_day_number(r.date);
        if (d - t > kMaxDaysAhead) return Reason::DateTooFar;
        if (t - d > kMaxDaysBehind) return Reason::DateTooOld;
        if (d >= t) future = true;
    }
    // One record per date (order is not required). n <= 400, so the square is small.
    for (size_t i = 1; i < n; ++i) {
        for (size_t j = 0; j < i; ++j) {
            if (records[i].date == records[j].date) return Reason::DuplicateDate;
        }
    }
    if (!future) return Reason::NoFutureDate;
    return Reason::Ok;
}

}  // namespace nvp::providers
