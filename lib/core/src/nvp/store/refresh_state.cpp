#include "nvp/store/refresh_state.h"

#include <cstring>

namespace nvp::store {
namespace {

constexpr char kMagic[4] = {'N', 'V', 'P', 'R'};
constexpr size_t kHead = 12;
// last_success 8, last_attempt 8, outcome 1, retry_step 1, next_retry 8,
// providers 4 x (failures 1, cool_down 8, skip 8), calibration (valid 1, offsets 6, days 1,
// district 4, hijri 1), coords_suspect 1
constexpr size_t kPayload = 8 + 8 + 1 + 1 + 8 + kProviderSlots * 17 + (1 + 6 + 1 + 4 + 1) + 1;

class Out {
public:
    explicit Out(uint8_t* p) : p_(p) {}
    void u8(uint8_t v) { *p_++ = v; }
    void u32(uint32_t v) {
        for (int i = 0; i < 4; ++i) *p_++ = static_cast<uint8_t>(v >> (8 * i));
    }
    void i64(int64_t v) {
        const uint64_t u = static_cast<uint64_t>(v);
        for (int i = 0; i < 8; ++i) *p_++ = static_cast<uint8_t>(u >> (8 * i));
    }

private:
    uint8_t* p_;
};

class In {
public:
    explicit In(const uint8_t* p) : p_(p) {}
    uint8_t u8() { return *p_++; }
    uint32_t u32() {
        uint32_t v = 0;
        for (int i = 0; i < 4; ++i) v |= static_cast<uint32_t>(*p_++) << (8 * i);
        return v;
    }
    int64_t i64() {
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i) v |= static_cast<uint64_t>(*p_++) << (8 * i);
        return static_cast<int64_t>(v);
    }

private:
    const uint8_t* p_;
};

void encode(const RefreshState& s, uint8_t* p) {
    Out o(p);
    o.i64(s.last_success_utc);
    o.i64(s.last_attempt_utc);
    o.u8(static_cast<uint8_t>(s.last_outcome));
    o.u8(s.retry_step);
    o.i64(s.next_retry_utc);
    for (const ProviderState& ps : s.providers) {
        o.u8(ps.failures);
        o.i64(ps.cool_down_until);
        o.i64(ps.skip_until);
    }
    o.u8(s.calibration.valid ? 1 : 0);
    for (int8_t v : s.calibration.offsets) o.u8(static_cast<uint8_t>(v));
    o.u8(s.calibration.days);
    o.u32(s.calibration.district);
    o.u8(static_cast<uint8_t>(s.calibration.hijri_offset));
    o.u8(s.coords_suspect ? 1 : 0);
}

bool decode(const uint8_t* p, RefreshState& s) {
    In in(p);
    s.last_success_utc = in.i64();
    s.last_attempt_utc = in.i64();
    const uint8_t outcome = in.u8();
    if (outcome > static_cast<uint8_t>(RefreshOutcome::Skipped)) return false;
    s.last_outcome = static_cast<RefreshOutcome>(outcome);
    s.retry_step = in.u8();
    s.next_retry_utc = in.i64();
    for (ProviderState& ps : s.providers) {
        ps.failures = in.u8();
        ps.cool_down_until = in.i64();
        ps.skip_until = in.i64();
    }
    s.calibration.valid = in.u8() != 0;
    for (int8_t& v : s.calibration.offsets) v = static_cast<int8_t>(in.u8());
    s.calibration.days = in.u8();
    s.calibration.district = in.u32();
    s.calibration.hijri_offset = static_cast<int8_t>(in.u8());
    s.coords_suspect = in.u8() != 0;
    return true;
}

bool produce(void* ctx, AtomicWriter& w) {
    const RefreshState& s = *static_cast<const RefreshState*>(ctx);
    uint8_t payload[kPayload];
    encode(s, payload);
    uint8_t head[kHead];
    std::memcpy(head, kMagic, 4);
    head[4] = static_cast<uint8_t>(kRefreshStateVersion);
    head[5] = static_cast<uint8_t>(kRefreshStateVersion >> 8);
    head[6] = static_cast<uint8_t>(kPayload);
    head[7] = static_cast<uint8_t>(kPayload >> 8);
    const uint32_t crc = crc32(0, payload, sizeof payload);
    for (int i = 0; i < 4; ++i) head[8 + i] = static_cast<uint8_t>(crc >> (8 * i));
    return w.write(head, sizeof head) && w.write(payload, sizeof payload);
}

}  // namespace

const char* refresh_outcome_text(RefreshOutcome o) {
    switch (o) {
        case RefreshOutcome::None: return "none";
        case RefreshOutcome::Complete: return "complete";
        case RefreshOutcome::Partial: return "partial";
        case RefreshOutcome::Failed: return "failed";
        case RefreshOutcome::Skipped: return "skipped";
    }
    return "?";
}

bool operator==(const RefreshState& a, const RefreshState& b) {
    uint8_t x[kPayload], y[kPayload];
    encode(a, x);
    encode(b, y);
    return std::memcmp(x, y, kPayload) == 0;
}

LoadResult load_refresh_state(FileSystem& fs, RefreshState& out) {
    out = RefreshState{};
    if (fs.exists(kRefreshStateTemp)) fs.remove(kRefreshStateTemp);  // an interrupted save
    if (!fs.exists(kRefreshStateFile)) return LoadResult::Missing;
    const int h = fs.open(kRefreshStateFile, OpenMode::Read);
    if (h < 0) return LoadResult::ReadError;
    uint8_t head[kHead], payload[kPayload];
    LoadResult r = LoadResult::Ok;
    if (!read_all(fs, h, head, sizeof head)) {
        r = LoadResult::Truncated;
    } else if (std::memcmp(head, kMagic, 4) != 0) {
        r = LoadResult::BadMagic;
    } else if ((head[4] | (head[5] << 8)) != kRefreshStateVersion || (head[6] | (head[7] << 8)) != kPayload) {
        r = LoadResult::BadVersion;
    } else if (!read_all(fs, h, payload, sizeof payload)) {
        r = LoadResult::Truncated;
    } else {
        uint32_t crc = 0;
        for (int i = 3; i >= 0; --i) crc = (crc << 8) | head[8 + i];
        RefreshState s;
        if (crc != crc32(0, payload, sizeof payload) || !decode(payload, s)) {
            r = LoadResult::BadCrc;
        } else {
            out = s;
        }
    }
    fs.close(h);
    return r;
}

SaveResult save_refresh_state(FileSystem& fs, const RefreshState& s) {
    return atomic_write(fs, kRefreshStateFile, kRefreshStateTemp, produce, const_cast<RefreshState*>(&s));
}

}  // namespace nvp::store
