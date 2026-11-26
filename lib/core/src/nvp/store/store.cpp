#include "nvp/store/store.h"

#include <cstdio>
#include <cstring>

namespace nvp::store {
namespace {

constexpr char kMagic[4] = {'N', 'V', 'P', 'S'};
constexpr uint16_t kChunkRecords = 16;  // records per read or write call (320 B on the stack)

uint16_t chunk_of(uint16_t count, uint16_t done) {
    const uint16_t left = static_cast<uint16_t>(count - done);
    return left < kChunkRecords ? left : kChunkRecords;
}
constexpr uint8_t kSourceCount = 5;   // engine::SourceId Diyanet..Local

engine::DayNumber epoch2020() { return engine::to_day_number(engine::Date{2020, 1, 1}); }

void put16(uint8_t* p, uint16_t v) {
    p[0] = static_cast<uint8_t>(v);
    p[1] = static_cast<uint8_t>(v >> 8);
}
void put32(uint8_t* p, uint32_t v) {
    for (int i = 0; i < 4; ++i) p[i] = static_cast<uint8_t>(v >> (8 * i));
}
void put64(uint8_t* p, uint64_t v) {
    for (int i = 0; i < 8; ++i) p[i] = static_cast<uint8_t>(v >> (8 * i));
}
uint16_t get16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
uint32_t get32(const uint8_t* p) {
    uint32_t v = 0;
    for (int i = 3; i >= 0; --i) v = (v << 8) | p[i];
    return v;
}
uint64_t get64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 7; i >= 0; --i) v = (v << 8) | p[i];
    return v;
}

void encode_record(const engine::DayRecord& r, uint8_t* p) {
    put16(p, static_cast<uint16_t>(engine::to_day_number(r.date) - epoch2020()));
    for (int i = 0; i < engine::kVakitCount; ++i) put16(p + 2 + 2 * i, r.minutes[i]);
    p[14] = r.hijri.present ? r.hijri.day : 0;
    p[15] = r.hijri.present ? r.hijri.month : 0;
    put16(p + 16, r.hijri.present ? r.hijri.year : 0);
    p[18] = source_code(r.source);
    p[19] = r.hijri.present ? 1 : 0;
}

bool decode_record(const uint8_t* p, engine::DayRecord& r) {
    r = engine::DayRecord{};
    r.date = engine::to_date(epoch2020() + get16(p));
    for (int i = 0; i < engine::kVakitCount; ++i) r.minutes[i] = get16(p + 2 + 2 * i);
    if ((p[19] & 1) != 0) {
        r.hijri.day = p[14];
        r.hijri.month = p[15];
        r.hijri.year = get16(p + 16);
        r.hijri.present = true;
    }
    return source_from_code(p[18], r.source);
}

void encode_header(const Store& s, uint32_t crc, uint8_t* h) {
    std::memset(h, 0, kHeaderBytes);
    std::memcpy(h, kMagic, 4);
    put16(h + 4, kFormatVersion);
    put16(h + 6, s.count);
    put32(h + 8, s.district_id);
    put32(h + 12, crc);
    put64(h + 16, static_cast<uint64_t>(s.last_refresh_utc));
    put32(h + 24, static_cast<uint32_t>(engine::to_day_number(s.records[0].date)));
    put32(h + 28, static_cast<uint32_t>(engine::to_day_number(s.records[s.count - 1].date)));
}

uint32_t records_crc(const Store& s) {
    uint32_t crc = 0;
    uint8_t rec[kRecordBytes];
    for (uint16_t i = 0; i < s.count; ++i) {
        encode_record(s.records[i], rec);
        crc = crc32(crc, rec, sizeof rec);
    }
    return crc;
}

// Reads exactly n bytes; false on an error or a short read.
bool read_all(FileSystem& fs, int h, void* buf, size_t n) {
    uint8_t* p = static_cast<uint8_t*>(buf);
    size_t got = 0;
    while (got < n) {
        long k = fs.read(h, p + got, n - got);
        if (k <= 0) return false;
        got += static_cast<size_t>(k);
    }
    return true;
}

SaveResult abandon(FileSystem& fs, int h, const char* tmp, SaveResult why) {
    if (h >= 0) fs.close(h);
    fs.remove(tmp);  // best effort; load() removes a leftover .new too
    return why;
}

}  // namespace

const char* load_result_text(LoadResult r) {
    switch (r) {
        case LoadResult::Ok: return "ok";
        case LoadResult::Missing: return "missing";
        case LoadResult::BadMagic: return "bad magic";
        case LoadResult::BadVersion: return "unknown version";
        case LoadResult::BadCrc: return "bad checksum";
        case LoadResult::Truncated: return "truncated";
        case LoadResult::ReadError: return "read error";
        case LoadResult::WrongDistrict: return "wrong district";
    }
    return "?";
}

const char* save_result_text(SaveResult r) {
    switch (r) {
        case SaveResult::Ok: return "ok";
        case SaveResult::Empty: return "empty";
        case SaveResult::OpenFailed: return "open failed";
        case SaveResult::WriteFailed: return "write failed";
        case SaveResult::CloseFailed: return "close failed";
        case SaveResult::VerifyFailed: return "verify failed";
        case SaveResult::RenameFailed: return "rename failed";
    }
    return "?";
}

bool store_file_name(uint32_t district, bool temporary, char* out, size_t cap) {
    int n = std::snprintf(out, cap, "store_%lu.%s", static_cast<unsigned long>(district), temporary ? "new" : "bin");
    return n > 0 && static_cast<size_t>(n) < cap;
}

uint32_t crc32(uint32_t crc, const void* data, size_t n) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    crc = ~crc;
    for (size_t i = 0; i < n; ++i) {
        crc ^= p[i];
        for (int k = 0; k < 8; ++k) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

uint8_t source_code(engine::SourceId s) { return static_cast<uint8_t>(s); }

bool source_from_code(uint8_t code, engine::SourceId& out) {
    if (code >= kSourceCount) return false;
    out = static_cast<engine::SourceId>(code);
    return true;
}

const char* source_key(engine::SourceId s) {
    switch (s) {
        case engine::SourceId::Diyanet: return "diyanet";
        case engine::SourceId::Imsakiyem: return "imsakiyem";
        case engine::SourceId::Emushaf: return "emushaf";
        case engine::SourceId::Aladhan: return "aladhan";
        case engine::SourceId::Local: return "local";
    }
    return "?";
}

LoadResult load(FileSystem& fs, uint32_t district, Store& out) {
    out.district_id = district;
    out.last_refresh_utc = 0;
    out.count = 0;

    char bin[32], tmp[32];
    store_file_name(district, false, bin, sizeof bin);
    store_file_name(district, true, tmp, sizeof tmp);
    if (fs.exists(tmp)) fs.remove(tmp);  // an interrupted save (research R5)
    if (!fs.exists(bin)) return LoadResult::Missing;

    int h = fs.open(bin, OpenMode::Read);
    if (h < 0) return LoadResult::ReadError;
    uint8_t head[kHeaderBytes];
    LoadResult result = LoadResult::Ok;
    uint32_t crc = 0;
    uint16_t count = 0;
    if (!read_all(fs, h, head, sizeof head)) {
        result = LoadResult::Truncated;
    } else if (std::memcmp(head, kMagic, 4) != 0) {
        result = LoadResult::BadMagic;
    } else if (get16(head + 4) != kFormatVersion) {
        result = LoadResult::BadVersion;
    } else if (get32(head + 8) != district) {
        result = LoadResult::WrongDistrict;
    } else {
        count = get16(head + 6);
        if (count == 0 || count > kMaxRecords) result = LoadResult::BadCrc;
    }
    // Records, in chunks.
    uint8_t chunk[kChunkRecords * kRecordBytes];
    for (uint16_t done = 0; result == LoadResult::Ok && done < count;) {
        uint16_t k = chunk_of(count, done);
        if (!read_all(fs, h, chunk, k * kRecordBytes)) {
            result = LoadResult::Truncated;
            break;
        }
        crc = crc32(crc, chunk, k * kRecordBytes);
        for (uint16_t i = 0; i < k; ++i) {
            if (!decode_record(chunk + i * kRecordBytes, out.records[done + i])) result = LoadResult::BadCrc;
        }
        done = static_cast<uint16_t>(done + k);
    }
    fs.close(h);
    if (result == LoadResult::Ok && crc != get32(head + 12)) result = LoadResult::BadCrc;
    if (result == LoadResult::Ok) {
        // The header's first and last day must match the records (they are outside the CRC), and
        // the records are sorted, one per date: anything else is not a file this code wrote.
        if (static_cast<int32_t>(get32(head + 24)) != engine::to_day_number(out.records[0].date) ||
            static_cast<int32_t>(get32(head + 28)) != engine::to_day_number(out.records[count - 1].date)) {
            result = LoadResult::BadCrc;
        }
        for (uint16_t i = 1; i < count; ++i) {
            if (engine::to_day_number(out.records[i - 1].date) >= engine::to_day_number(out.records[i].date))
                result = LoadResult::BadCrc;
        }
    }
    if (result != LoadResult::Ok) return result;
    out.count = count;
    out.last_refresh_utc = static_cast<int64_t>(get64(head + 16));
    return LoadResult::Ok;
}

SaveResult save(FileSystem& fs, const Store& s) {
    if (s.count == 0) return SaveResult::Empty;
    const uint16_t count = s.count > kMaxRecords ? static_cast<uint16_t>(kMaxRecords) : s.count;
    char bin[32], tmp[32];
    store_file_name(s.district_id, false, bin, sizeof bin);
    store_file_name(s.district_id, true, tmp, sizeof tmp);

    const uint32_t crc = records_crc(s);
    uint8_t head[kHeaderBytes];
    encode_header(s, crc, head);

    // 1. Write the new file completely and close it (littlefs commits on close).
    if (fs.exists(tmp) && !fs.remove(tmp)) return SaveResult::OpenFailed;
    int h = fs.open(tmp, OpenMode::Write);
    if (h < 0) return abandon(fs, -1, tmp, SaveResult::OpenFailed);
    if (!fs.write(h, head, sizeof head)) return abandon(fs, h, tmp, SaveResult::WriteFailed);
    uint8_t chunk[kChunkRecords * kRecordBytes];
    for (uint16_t done = 0; done < count;) {
        uint16_t k = chunk_of(count, done);
        for (uint16_t i = 0; i < k; ++i) encode_record(s.records[done + i], chunk + i * kRecordBytes);
        if (!fs.write(h, chunk, k * kRecordBytes)) return abandon(fs, h, tmp, SaveResult::WriteFailed);
        done = static_cast<uint16_t>(done + k);
    }
    if (!fs.close(h)) return abandon(fs, -1, tmp, SaveResult::CloseFailed);

    // 2. Read it back and check the header and the CRC.
    h = fs.open(tmp, OpenMode::Read);
    if (h < 0) return abandon(fs, -1, tmp, SaveResult::VerifyFailed);
    uint8_t back[kHeaderBytes];
    if (!read_all(fs, h, back, sizeof back) || std::memcmp(back, head, sizeof head) != 0)
        return abandon(fs, h, tmp, SaveResult::VerifyFailed);
    uint32_t check = 0;
    for (uint16_t done = 0; done < count;) {
        uint16_t k = chunk_of(count, done);
        if (!read_all(fs, h, chunk, k * kRecordBytes)) return abandon(fs, h, tmp, SaveResult::VerifyFailed);
        check = crc32(check, chunk, k * kRecordBytes);
        done = static_cast<uint16_t>(done + k);
    }
    uint8_t extra;
    if (fs.read(h, &extra, 1) != 0 || check != crc) return abandon(fs, h, tmp, SaveResult::VerifyFailed);
    if (!fs.close(h)) return abandon(fs, -1, tmp, SaveResult::VerifyFailed);

    // 3. Atomic replace.
    if (!fs.rename(tmp, bin)) return abandon(fs, -1, tmp, SaveResult::RenameFailed);
    return SaveResult::Ok;
}

const engine::DayRecord* find(const Store& s, engine::Date d) {
    if (!engine::valid(d) || s.count == 0) return nullptr;
    const engine::DayNumber want = engine::to_day_number(d);
    size_t lo = 0, hi = s.count;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        engine::DayNumber n = engine::to_day_number(s.records[mid].date);
        if (n == want) return &s.records[mid];
        if (n < want) lo = mid + 1; else hi = mid;
    }
    return nullptr;
}

uint16_t days_after(const Store& s, engine::Date d) {
    if (!engine::valid(d)) return 0;
    const engine::DayNumber n = engine::to_day_number(d);
    uint16_t k = 0;
    for (uint16_t i = s.count; i > 0; --i) {
        if (engine::to_day_number(s.records[i - 1].date) <= n) break;
        ++k;
    }
    return k;
}

}  // namespace nvp::store
