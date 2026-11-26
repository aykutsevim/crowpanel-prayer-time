#include "nvp/providers/refresh.h"

#include <cstdio>

#include "nvp/providers/http_get.h"

namespace nvp::providers {
namespace {

// 04-data-sources §3.1 and §11.
constexpr const char* kHost = "https://ezanvakti.imsakiyem.com";

bool format_url(char* out, size_t cap, const char* fmt, uint32_t district, engine::Date a, engine::Date b) {
    int n = std::snprintf(out, cap, fmt, kHost, static_cast<unsigned long>(district), a.year, a.month, a.day, b.year,
                          b.month, b.day);
    return n > 0 && static_cast<size_t>(n) < cap;
}

engine::Date plus_days(engine::Date d, int32_t days) { return engine::to_date(engine::to_day_number(d) + days); }

}  // namespace

const char* http_outcome_text(HttpOutcome o) {
    switch (o) {
        case HttpOutcome::Done: return "done";
        case HttpOutcome::Refused: return "refused";
        case HttpOutcome::Connect: return "connect";
        case HttpOutcome::Tls: return "tls";
        case HttpOutcome::Timeout: return "timeout";
        case HttpOutcome::Aborted: return "aborted";
        case HttpOutcome::Error: return "error";
    }
    return "?";
}

bool v2_url(uint32_t district, engine::Date from, char* out, size_t cap) {
    return format_url(out, cap, "%s/api/v2/times?district=%lu&from=%04d-%02d-%02d&to=%04d-%02d-%02d", district, from,
                      plus_days(from, kV2Days));
}

bool v1_url(uint32_t district, engine::Date from, char* out, size_t cap) {
    return format_url(out, cap, "%s/api/prayer-times/%lu/range?startDate=%04d-%02d-%02d&endDate=%04d-%02d-%02d",
                      district, from, plus_days(from, kV1Days - 1));
}

}  // namespace nvp::providers
