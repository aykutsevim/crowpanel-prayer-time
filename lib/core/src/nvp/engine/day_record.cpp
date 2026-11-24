#include "nvp/engine/day_record.h"

namespace nvp::engine {

uint8_t rank(SourceId s) {
    switch (s) {
        case SourceId::Diyanet: return 50;
        case SourceId::Imsakiyem: return 40;
        case SourceId::Emushaf: return 40;
        case SourceId::Aladhan: return 20;
        case SourceId::Local: return 10;
    }
    return 0;
}

bool valid(const DayRecord& r) {
    if (!valid(r.date)) return false;
    for (uint8_t k = 0; k < kVakitCount; ++k) {
        if (r.minutes[k] > 1439) return false;
        if (k > 0 && r.minutes[k] <= r.minutes[k - 1]) return false;
    }
    return true;
}

}  // namespace nvp::engine
