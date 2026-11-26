// The day-record store (spec 006 FR-008..FR-010, research R5, data-model.md "Store";
// docs/specification/04-data-sources.md §8, DATA-07, DATA-08). Pure C++, no heap.
//
// One file per district, "store_<id>.bin", little-endian:
//   header 32 B  0 "NVPS" | 4 u16 version = 1 | 6 u16 count | 8 u32 district
//                | 12 u32 CRC-32 (IEEE) of the records | 16 i64 last refresh (UTC s)
//                | 24 i32 first day | 28 i32 last day (days since 1970-01-01)
//   records count x 20 B  u16 days since 2020-01-01 | 6 x u16 minutes | u8 hijri day | u8 hijri month
//                | u16 hijri year | u8 source id | u8 flags (bit 0: hijri present)
// A save writes "store_<id>.new", closes it, reads it back, checks it, then renames it over the
// old file, so a power cut leaves exactly the old or exactly the new store.
#pragma once

#include <cstddef>
#include <cstdint>

#include "nvp/engine/calendar.h"
#include "nvp/engine/day_record.h"
#include "nvp/store/file_system.h"

namespace nvp::store {

inline constexpr size_t kMaxRecords = 400;
inline constexpr size_t kHeaderBytes = 32;
inline constexpr size_t kRecordBytes = 20;
inline constexpr uint16_t kFormatVersion = 1;

struct Store {
    uint32_t district_id = 0;
    int64_t last_refresh_utc = 0;  // the last successful refresh, once per store (analysis I1)
    uint16_t count = 0;
    engine::DayRecord records[kMaxRecords];  // sorted by date, one per date
};

enum class LoadResult : uint8_t { Ok, Missing, BadMagic, BadVersion, BadCrc, Truncated, ReadError, WrongDistrict };
enum class SaveResult : uint8_t { Ok, Empty, OpenFailed, WriteFailed, CloseFailed, VerifyFailed, RenameFailed };

const char* load_result_text(LoadResult r);  // "bad checksum", ...
const char* save_result_text(SaveResult r);

// "store_9206.bin" / "store_9206.new". Returns false when cap is too small.
bool store_file_name(uint32_t district, bool temporary, char* out, size_t cap);

// Removes a leftover "store_<id>.new" first. A bad file is never returned as data: on any result
// other than Ok, `out` is left empty (count 0) for `district`.
LoadResult load(FileSystem& fs, uint32_t district, Store& out);

// Atomic replace (research R5). On any failure the old file is untouched.
SaveResult save(FileSystem& fs, const Store& s);

const engine::DayRecord* find(const Store& s, engine::Date d);
inline bool covers(const Store& s, engine::Date d) { return find(s, d) != nullptr; }
// Days stored after `d` (records dated later than d), for the cache_ending state of Slice 7.
uint16_t days_after(const Store& s, engine::Date d);

// IEEE 802.3 CRC-32 (reflected, 0xEDB88320), continued from `crc` (start with 0).
uint32_t crc32(uint32_t crc, const void* data, size_t n);

// The stable source id stored on flash (04 §8) and its log name ("imsakiyem").
uint8_t source_code(engine::SourceId s);
bool source_from_code(uint8_t code, engine::SourceId& out);
const char* source_key(engine::SourceId s);

}  // namespace nvp::store
