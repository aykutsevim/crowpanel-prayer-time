// Turkish title case for Diyanet's upper-case place names (04 §9; spec 004 research R8):
// "SULTANBEYLİ" -> "Sultanbeyli", "IĞDIR" -> "Iğdır". The page applies the same rules in JS.
#pragma once

#include <cstddef>

namespace nvp::text {

// Lowers every letter with Turkish rules (I -> ı, İ -> i, Â/Î/Û and Ç Ğ Ö Ş Ü to their lower forms),
// then raises the first letter of each word (after start, space, '-', '(' or '.'), with i -> İ and
// ı -> I. Returns the length, or 0 with `out` empty when it does not fit.
size_t title_case_tr(const char* in, char* out, size_t cap);

}  // namespace nvp::text
