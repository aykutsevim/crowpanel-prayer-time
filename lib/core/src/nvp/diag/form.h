// Form-encoded body decoding (application/x-www-form-urlencoded) for POST /api/setup
// (contracts/setup-portal.md; FR-021). Pure, host-tested (test/test_diag/test_form.cpp).
#pragma once

#include <cstddef>

namespace nvp::diag {

// Finds `key` in `body` ("a=1&b=2") and writes its decoded value ('+' is a space, %XX a byte)
// to `out`. Returns false, with `out` empty, when the key is missing, the value does not fit in
// `cap` bytes with its terminator, or an escape is malformed or decodes to NUL.
bool form_value(const char* body, const char* key, char* out, size_t cap);

}  // namespace nvp::diag
