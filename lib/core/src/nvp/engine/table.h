// The engine's output as Markdown table lines, for the owner and the reference comparison
// (spec 002 FR-022, SC-005; format: specs/002-prayer-engine/contracts/engine-table.md).
// Development aid; unused in the firmware, so the linker drops it.
#pragma once

#include <cstddef>

#include "nvp/engine/engine.h"

namespace nvp::engine {

// Header and separator lines, each ending in '\n'. Returns the bytes written, 0 if out of room.
size_t table_header(char* out, size_t cap);

// One row for `v`, ending in '\n'. Returns the bytes written; writes "" and returns 0 if out of room.
size_t table_row(const MainView& v, char* out, size_t cap);

}  // namespace nvp::engine
