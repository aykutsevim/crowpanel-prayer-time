// Minimal UTF-8 decoding and encoding for the text helpers. Pure C++, no heap.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvp::text {

inline constexpr uint32_t kBadCodePoint = 0xFFFFFFFFu;

// Decodes one code point at `s` (NUL-terminated) and advances `s`. Invalid or truncated sequences
// give kBadCodePoint and advance by one byte.
uint32_t utf8_next(const char*& s);

// Encodes `cp` into `out` (at least 4 bytes). Returns the byte count (1..4).
size_t utf8_put(uint32_t cp, char* out);

}  // namespace nvp::text
