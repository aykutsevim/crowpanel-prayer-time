// JSON string escaping for the setup page's API (contracts/setup-portal.md). Pure C++, no heap.
#pragma once

#include <cstddef>

namespace nvp::diag {

// Escapes `in` for use inside a JSON string (without the quotes): `"` and `\` are escaped,
// control characters become \n, \r, \t or \u00XX, and other bytes (UTF-8) pass through.
// Writes into `out` (always NUL-terminated when cap > 0, never splitting an escape) and returns
// the length the full escaped text needs.
size_t json_escape(const char* in, char* out, size_t cap);

}  // namespace nvp::diag
