// Network names on the device screen: characters the fonts lack become '?' (02 §7.3; spec 004
// research R13). The set here must match the ranges of nvp_cond_600_38 and nvp_txt_400_18 in
// assets/fonts/gen_fonts.py: printable ASCII, the Turkish letters and Â â Î î Û û.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvp::text {

bool display_has_glyph(uint32_t code_point);

// Copies `in` to `out`, replacing each code point outside the set (and each invalid byte) with '?'.
// Truncates at a code-point boundary. Returns the length written.
size_t filter_display_glyphs(const char* in, char* out, size_t cap);

}  // namespace nvp::text
