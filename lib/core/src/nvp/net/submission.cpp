#include "nvp/net/submission.h"

#include <cstring>

#include "nvp/diag/form.h"

namespace nvp::net {
namespace {

void secure_zero(void* p, size_t n) {
    volatile unsigned char* v = static_cast<volatile unsigned char*>(p);
    while (n--) *v++ = 0;
}

bool parse_id(const char* s, uint32_t& out) {
    if (s[0] == '\0') return false;
    uint64_t v = 0;
    for (const char* p = s; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
        v = v * 10 + static_cast<uint32_t>(*p - '0');
        if (v > 0xFFFFFFFFu) return false;
    }
    out = static_cast<uint32_t>(v);
    return v > 0;
}

}  // namespace

SubmitError parse_submission(const char* body, size_t len, Submission& out) {
    wipe(out);
    if (body == nullptr || len > kMaxSubmissionBody) return SubmitError::Ssid;
    char text[kMaxSubmissionBody + 1];
    std::memcpy(text, body, len);
    text[len] = '\0';

    SubmitError result = SubmitError::Ok;
    if (!nvp::diag::form_value(text, "ssid", out.ssid, sizeof out.ssid) || out.ssid[0] == '\0') {
        result = SubmitError::Ssid;
    } else if (!nvp::diag::form_value(text, "password", out.pass, sizeof out.pass)) {
        result = SubmitError::Password;
    } else {
        const size_t p = std::strlen(out.pass);
        if (p != 0 && p < 8) result = SubmitError::Password;
    }
    if (result == SubmitError::Ok) {
        char id[12];
        Location& l = out.location;
        if (!nvp::diag::form_value(text, "district_id", id, sizeof id) || !parse_id(id, l.district_id) ||
            !nvp::diag::form_value(text, "province", l.province, sizeof l.province) || l.province[0] == '\0' ||
            !nvp::diag::form_value(text, "district", l.district, sizeof l.district) || l.district[0] == '\0') {
            result = SubmitError::Location;
        }
    }
    secure_zero(text, sizeof text);
    if (result != SubmitError::Ok) wipe(out);
    return result;
}

const char* submit_error_name(SubmitError e) {
    switch (e) {
        case SubmitError::Ok: return "";
        case SubmitError::Ssid: return "ssid";
        case SubmitError::Password: return "password";
        case SubmitError::Location: return "location";
    }
    return "ssid";
}

void wipe(Submission& s) { secure_zero(&s, sizeof s); }

}  // namespace nvp::net
