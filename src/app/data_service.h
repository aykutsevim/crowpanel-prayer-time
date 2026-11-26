// The day-record store at run time (spec 006 T018, T024, T025, T027, T031; research R5, R7, R8;
// FR-005..FR-017).
//
// The network worker (core 0) owns the store: it loads it at start, refreshes it through
// nvp::providers::refresh_with (wired here to HTTPS and LittleFS; the flow itself is tested on the
// host), and publishes a RAM copy (PSRAM) with a version under a lock. The UI (core 1) only reads
// that copy: no file I/O on the UI core (constitution IX). No SDK headers here (constitution VI).
#pragma once

#include <cstdint>

#include "nvp/engine/calendar.h"
#include "nvp/engine/day_record.h"

namespace app::data {

// --- UI core ---

// The local date of the real clock, noted by the UI every update (the worker has no time zone).
void note_today(nvp::engine::Date today);

// Yesterday, today and tomorrow from the published copy; false where the store has no record.
struct Around {
    bool present[3] = {false, false, false};
    nvp::engine::DayRecord rec[3];
};
Around around(nvp::engine::Date today);

struct Summary {
    bool present = false;  // the store for the stored district has records
    uint32_t district = 0;
    uint16_t count = 0;
    nvp::engine::Date first;
    nvp::engine::Date last;
    int64_t last_refresh_utc = 0;  // 0 = none
    uint32_t version = 0;          // changes with every publish (FR-015)
};
Summary summary();

// Development commands (contracts/serial-console.md). The worker acts and logs the result.
void request_refresh();
void request_show();
void request_corrupt();
void request_erase();

// --- Network worker ---

void worker_start(bool has_location, uint32_t district, void (*before_write)());  // before_write: FR-011

struct Tick {
    bool running_up = false;   // running mode with the link up
    bool link_came_up = false;  // this tick: a new connection (allows a new attempt)
    bool time_valid = false;
    bool has_location = false;
    uint32_t district = 0;
    void (*before_save)() = nullptr;  // waits for a running Wi-Fi scan (FR-011)
};
void worker_tick(const Tick& t);

}  // namespace app::data
