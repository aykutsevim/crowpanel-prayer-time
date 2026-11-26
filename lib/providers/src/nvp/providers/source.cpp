#include "nvp/providers/source.h"

#include <cstring>

namespace nvp::providers {

engine::SourceId source_id(ProviderId p) {
    switch (p) {
        case ProviderId::Diyanet: return engine::SourceId::Diyanet;
        case ProviderId::Imsakiyem: return engine::SourceId::Imsakiyem;
        case ProviderId::Emushaf: return engine::SourceId::Emushaf;
        case ProviderId::Aladhan: return engine::SourceId::Aladhan;
        case ProviderId::Local: return engine::SourceId::Local;
    }
    return engine::SourceId::Local;
}

const char* provider_key(ProviderId p) {
    switch (p) {
        case ProviderId::Diyanet: return "diyanet";
        case ProviderId::Imsakiyem: return "imsakiyem";
        case ProviderId::Emushaf: return "emushaf";
        case ProviderId::Aladhan: return "aladhan";
        case ProviderId::Local: return "local";
    }
    return "?";
}

bool provider_from_key(const char* key, ProviderId& out) {
    for (size_t i = 0; i < kProviderCount; ++i) {
        const ProviderId p = static_cast<ProviderId>(i);
        if (key != nullptr && std::strcmp(key, provider_key(p)) == 0) {
            out = p;
            return true;
        }
    }
    return false;
}

const char* fetch_outcome_text(FetchOutcome o) {
    switch (o) {
        case FetchOutcome::Ok: return "ok";
        case FetchOutcome::Failed: return "failed";
        case FetchOutcome::RateLimited: return "rate limited";
        case FetchOutcome::NotFound: return "not found";
    }
    return "?";
}

}  // namespace nvp::providers
