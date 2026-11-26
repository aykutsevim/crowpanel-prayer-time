// The day-record store at run time (spec 006 T018..T031; spec 007 T036..T040, research R5..R10).
//
// The network worker (core 0) owns the store, the refresh planner, the persisted refresh state
// (refresh.bin) and the coordinates. Once a tick it feeds the planner (boot, link up/down, setup,
// location change, manual) and, when the planner says so, runs one refresh: İmsakiyem, emushaf and
// Aladhan through nvp::providers::refresh_run over HTTPS and LittleFS (the logic is host-tested).
// It publishes a RAM copy (PSRAM) with a version under a lock; that copy also carries the device's
// own calculated days for today .. today + 7 when nothing else covers today, and a computed Hijri
// date on calculated days (research R8). The UI (core 1) only reads the copy: no file I/O on the UI
// core (constitution IX). No SDK headers here (constitution VI).
#pragma once
#include <cstdint>

#include "nvp/engine/calendar.h"
#include "nvp/engine/day_record.h"

namespace app::data {

// --- UI core ---

// The local date of the real clock and the zone's UTC offset, noted by the UI every update (the
// worker has no time zone).
void note_today(nvp::engine::Date today, int32_t utc_offset_s);

// Yesterday, today and tomorrow from the published copy; false where it has no record.
struct Around {
    bool present[3] = {false, false, false};
    nvp::engine::DayRecord rec[3];
};
Around around(nvp::engine::Date today);

struct Summary {
    bool present = false;  // the published copy has records
    uint32_t district = 0;
    uint16_t count = 0;
    nvp::engine::Date first;
    nvp::engine::Date last;
    int64_t last_refresh_utc = 0;  // 0 = none
    uint32_t version = 0;          // changes with every publish (FR-015)
    uint8_t last_outcome = 0;      // store::RefreshOutcome of the last refresh (0 none, 1 complete, ...)
    int64_t next_utc = 0;          // the next planned refresh, 0 = none
};
Summary summary();

// Consecutive days after `today` in the published copy (the N of cache_ending, FR-019).
uint16_t stored_days_after(nvp::engine::Date today);

// Development commands (contracts/serial-console.md). The worker acts and logs the result.
void request_refresh();
void request_show();
void request_corrupt();
void request_erase();
void request_block(int provider, bool block);  // providers::ProviderId 1..3; 0 with unblock = all
void request_plan();
void request_calib();

// --- Network worker ---

void worker_start(bool has_location, uint32_t district, const char* district_name, void (*before_write)());

struct Tick {
    bool running_up = false;    // running mode with the link up
    bool link_came_up = false;  // this tick: a new connection
    bool time_valid = false;
    bool has_location = false;
    uint32_t district = 0;
    const char* district_name = "";   // upper case, as in the place list (for the coordinates)
    void (*before_save)() = nullptr;  // waits for a running Wi-Fi scan (spec 006 FR-011)
};
void worker_tick(const Tick& t);

}  // namespace app::data
