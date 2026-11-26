// Aladhan calendar parser (spec 007 T020, research R2, 04-data-sources §3.4). Pure C++, streaming
// over JsonStream; no heap.
//
// `GET /v1/calendar/{year}/{month}?latitude&longitude&method=13&school=0&timezonestring=Europe/Istanbul`
// answers `{code:200, status:"OK", data:[{timings:{Fajr:"06:08 (+03)", ...}, date:{gregorian:{date:
// "DD-MM-YYYY"}}, ...}]}`. Mapping: Fajr -> İmsak (not Aladhan's Imsak), Sunrise, Dhuhr, Asr,
// Maghrib, Isha; the first five characters of each time. Aladhan's Hijri date is NOT taken (it
// differed from Diyanet on 16 of 38 days); calculated records get theirs on the device (R8).
// Records outside [keep_from, keep_to] (before yesterday, or outside the requested month) are dropped
// before validation and counted in info().records_dropped. The source is calculated (rank 20).
#pragma once
#include <cstddef>
#include <cstdint>

#include "nvp/engine/calendar.h"
#include "nvp/engine/day_record.h"
#include "nvp/providers/imsakiyem.h"
#include "nvp/providers/json_stream.h"

namespace nvp::providers {

class AladhanParser : private JsonSink {
public:
    AladhanParser(RecordSink& sink, engine::Date keep_from, engine::Date keep_to);

    bool feed(const char* data, size_t n);
    void finish();
    const BatchInfo& info() const { return info_; }

private:
    void on_json(const JsonEvent& e) override;
    void end_record();

    RecordSink& sink_;
    engine::Date keep_from_;
    engine::Date keep_to_;
    JsonStream json_;
    BatchInfo info_;
    bool code_200_ = false;
    bool status_ok_ = false;
    bool in_record_ = false;
    engine::DayRecord rec_;
};

// The calendar URL for a month; false when cap is too small. Coordinates in degrees x 100 000.
bool aladhan_url(int year, int month, int32_t lat_e5, int32_t lon_e5, char* out, size_t cap);

}  // namespace nvp::providers
