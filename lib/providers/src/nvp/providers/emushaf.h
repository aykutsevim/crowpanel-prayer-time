// emushaf `/vakitler/{district}` parser (spec 007 T016, research R1, 04-data-sources §3.2).
// Pure C++, streaming over JsonStream; no heap.
//
// The answer is a top-level JSON array of about 30 day objects. Taken per element:
// `MiladiTarihKisa` (DD.MM.YYYY), `Imsak`, `Gunes`, `Ogle`, `Ikindi`, `Aksam`, `Yatsi` (HH:MM) and
// `HicriTarihKisa` (D.M.YYYY). The ISO fields are ignored (they always carry Türkiye's offset).
// The answer does not name the place; the request does. It starts 5-7 days before today and is
// served from a cache for up to five days, so records before `keep_from` (yesterday) are dropped
// here, before validation, and counted in info().records_dropped.
#pragma once
#include <cstddef>
#include <cstdint>

#include "nvp/engine/calendar.h"
#include "nvp/engine/day_record.h"
#include "nvp/providers/imsakiyem.h"
#include "nvp/providers/json_stream.h"

namespace nvp::providers {

class EmushafParser : private JsonSink {
public:
    EmushafParser(RecordSink& sink, engine::Date keep_from);

    bool feed(const char* data, size_t n);
    void finish();
    const BatchInfo& info() const { return info_; }

private:
    void on_json(const JsonEvent& e) override;
    void end_record();

    RecordSink& sink_;
    engine::Date keep_from_;
    JsonStream json_;
    BatchInfo info_;
    bool in_record_ = false;
    engine::DayRecord rec_;
};

// "DD.MM.YYYY" (separator '.' or '-') -> date; false when not a valid date.
bool parse_dmy(const char* text, size_t length, char sep, engine::Date& out);

}  // namespace nvp::providers
