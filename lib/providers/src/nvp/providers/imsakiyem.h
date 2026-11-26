// İmsakiyem parsers (spec 006 research R1, R2; docs/specification/04-data-sources.md §3.1).
//
// Two answer shapes, both fed in chunks through a JsonStream:
//   v2  GET /api/v2/times?district=&from=&to=
//       {success, data:{location:{district_id}, source:{type}, times:[{date, hijri:{day,month,year},
//        imsak, gunes, ogle, ikindi, aksam, yatsi}]}}   or   {success:false, error:{code}}
//   v1  GET /api/prayer-times/{district}/range?startDate=&endDate=
//       {success, data:[{district_id:{_id}, date:"YYYY-MM-DDT...", hijri_date:{day,month,year},
//        meta:{source}, times:{imsak, ...}}]}
// Each completed element becomes one engine::DayRecord (source İmsakiyem) passed to a RecordSink,
// so the record list is never held here. A field that is missing or cannot be read leaves the
// record invalid (date 0-0-0 or a time of kBadMinutes), and validate_batch rejects the batch.
#pragma once

#include <cstddef>
#include <cstdint>

#include "nvp/engine/day_record.h"
#include "nvp/providers/json_stream.h"

namespace nvp::providers {

enum class ImsakiyemApi : uint8_t { V2, V1 };

inline constexpr uint16_t kBadMinutes = 0xFFFF;

// What the answer said about itself, besides the records (data-model.md "Fetched batch").
struct BatchInfo {
    bool success_seen = false;
    bool success = false;
    char error_code[32] = {};     // v2 error.code, for example DISTRICT_NOT_FOUND
    bool district_seen = false;
    uint32_t district = 0;        // 0 when the id was not a number
    bool district_mixed = false;  // v1: the records named different districts
    bool source_diyanet = false;  // v2: data.source.type is "diyanet"; v1: every record's meta.source starts with "Diyanet"
    bool records_array_seen = false;
    uint32_t records_seen = 0;    // every element, including those the caller did not keep
    bool field_overflow = false;  // a needed string was longer than the tokenizer keeps
    JsonError json_error = JsonError::None;
    bool complete = false;        // finish() saw one whole, well-formed document
};

class RecordSink {
public:
    virtual ~RecordSink() = default;
    virtual void on_record(const engine::DayRecord& r) = 0;
};

class ImsakiyemParser : private JsonSink {
public:
    ImsakiyemParser(ImsakiyemApi api, RecordSink& sink);

    // False once the JSON is malformed (the caller may stop reading).
    bool feed(const char* data, size_t n);
    // End of the body; sets info().complete and info().json_error.
    void finish();

    const BatchInfo& info() const { return info_; }

private:
    void on_json(const JsonEvent& e) override;
    void begin_record();
    void end_record();
    void on_record_field(const char* field, const JsonEvent& e);
    void set_district(const JsonEvent& e, bool per_record);

    ImsakiyemApi api_;
    RecordSink& sink_;
    JsonStream json_;
    BatchInfo info_;
    bool in_record_ = false;
    engine::DayRecord rec_;
    int32_t hijri_[3] = {};
    uint8_t hijri_seen_ = 0;  // bit 0 day, 1 month, 2 year
    uint32_t diyanet_records_ = 0;
};

// "HH:MM" -> minutes after midnight, or kBadMinutes.
uint16_t parse_hhmm(const char* text, size_t length);
// "YYYY-MM-DD", optionally followed by "T..." when allow_time is set. False when not a valid date.
bool parse_iso_date(const char* text, size_t length, bool allow_time, engine::Date& out);

}  // namespace nvp::providers
