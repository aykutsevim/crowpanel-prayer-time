// Scripted sources and a minute-by-minute simulator for the planner and the refresh runner
// (spec 007 T001, research R5, R6). Header-only; included as "../support/scripted_source.h".
//
// ScriptedSource answers each provider from a behaviour the test sets (ok with records up to a last
// day, failing, rate-limited, ...) and records every call. Sim ties a Planner, the runner, a
// FakeFs, a store and a scripted source together and advances time a minute at a time, the way the
// device's network worker does.
#pragma once
#include <cstdio>
#include <string>
#include <vector>

#include "fake_fs.h"
#include "nvp/calc/prayer_times.h"
#include "nvp/engine/calendar.h"
#include "nvp/providers/planner.h"
#include "nvp/providers/runner.h"
#include "nvp/providers/source.h"
#include "nvp/store/refresh_state.h"
#include "nvp/store/store.h"

namespace nvp_test {

using nvp::engine::Date;
using nvp::engine::DayRecord;
using nvp::providers::FetchOutcome;
using nvp::providers::FetchRequest;
using nvp::providers::FetchResult;
using nvp::providers::ProviderId;
using nvp::providers::Reason;

inline Date plus_days(Date d, int32_t n) { return nvp::engine::to_date(nvp::engine::to_day_number(d) + n); }
inline int32_t days_between(Date a, Date b) { return nvp::engine::to_day_number(b) - nvp::engine::to_day_number(a); }

enum class Mode { Ok, Failed, RateLimited, Timeout, Malformed, NotFound };

struct Behaviour {
    Mode mode = Mode::Ok;
    bool has_last = false;
    Date last;               // the last day the provider publishes (inclusive); none = up to req.to
    int32_t first_offset = 0;  // first day relative to req.from (emushaf starts before today: still clipped
                               // to yesterday, as the real source must, research R1)
    uint16_t shift = 0;      // minutes added to every time (to tell sources apart, or a mismatch)
    uint16_t hour_shift = 0;  // whole hours added (an implausible answer, FR-016)
};

struct Call {
    ProviderId provider;
    Date from, to;
    bool allow_fallback;
    int64_t at_utc;
    FetchOutcome outcome;
};

class ScriptedSource : public nvp::providers::Source {
public:
    Behaviour behaviour[nvp::providers::kProviderCount];
    std::vector<Call> calls;
    int64_t now_utc = 0;

    void set(ProviderId p, Mode m) { behaviour[static_cast<size_t>(p)].mode = m; }
    void set_last(ProviderId p, Date last) {
        behaviour[static_cast<size_t>(p)].has_last = true;
        behaviour[static_cast<size_t>(p)].last = last;
    }
    // Index of the first call to p, or -1.
    int first(ProviderId p) const {
        for (size_t i = 0; i < calls.size(); ++i)
            if (calls[i].provider == p) return static_cast<int>(i);
        return -1;
    }
    int count(ProviderId p) const {
        int n = 0;
        for (const Call& c : calls) n += c.provider == p ? 1 : 0;
        return n;
    }
    FetchResult fetch(const FetchRequest& req, DayRecord* out, size_t cap) override {
        const Behaviour& b = behaviour[static_cast<size_t>(req.provider)];
        FetchResult r;
        r.requests = 1;
        switch (b.mode) {
            case Mode::Ok: r.outcome = FetchOutcome::Ok; r.reason = Reason::Ok; r.http_status = 200; break;
            case Mode::Failed: r.outcome = FetchOutcome::Failed; r.reason = Reason::Network; break;
            case Mode::Timeout: r.outcome = FetchOutcome::Failed; r.reason = Reason::Network; break;
            case Mode::Malformed: r.outcome = FetchOutcome::Failed; r.reason = Reason::Malformed; r.http_status = 200; break;
            case Mode::RateLimited: r.outcome = FetchOutcome::RateLimited; r.reason = Reason::Http; r.http_status = 429; break;
            case Mode::NotFound: r.outcome = FetchOutcome::NotFound; r.reason = Reason::ErrorCode; r.http_status = 404; break;
        }
        // İmsakiyem tries v1 after a failed v2 unless the request is lite.
        if (req.provider == ProviderId::Imsakiyem && req.allow_fallback && r.outcome != FetchOutcome::Ok &&
            b.mode != Mode::NotFound)
            r.requests = 2;
        calls.push_back({req.provider, req.from, req.to, req.allow_fallback, now_utc, r.outcome});
        if (r.outcome != FetchOutcome::Ok) return r;
        Date first = plus_days(req.from, b.first_offset);
        const Date yesterday = plus_days(req.today, -1);
        if (days_between(first, yesterday) > 0) first = yesterday;
        Date last = req.to;
        if (b.has_last && days_between(b.last, last) > 0) last = b.last;
        const nvp::engine::SourceId src = nvp::providers::source_id(req.provider);
        for (Date d = first; days_between(d, last) >= 0 && r.count < cap; d = plus_days(d, 1)) {
            DayRecord& rec = out[r.count++];
            rec = DayRecord{};
            rec.date = d;
            // Plausible times for Ankara (the calculation), so that the plausibility check passes;
            // `hour_shift` moves them by whole hours to make them implausible.
            uint16_t t[6] = {368, 456, 760, 912, 1054, 1137};
            nvp::calc::prayer_times_local(nvp::calc::Coordinates{39.92063, 32.85403}, d, nvp::calc::turkey(), 10800, t);
            for (int i = 0; i < 6; ++i) rec.minutes[i] = static_cast<uint16_t>(t[i] + b.shift + 60 * b.hour_shift);
            rec.hijri = {15, 6, 1448, req.provider != ProviderId::Aladhan};
            rec.source = src;
        }
        if (r.count == 0) {
            r.outcome = FetchOutcome::Failed;
            r.reason = Reason::NoFutureDate;
        }
        return r;
    }
};

// A device in miniature: planner + runner + store, ticking once a minute.
class Sim {
public:
    static constexpr int32_t kOffset = 3 * 3600;  // <+03>-3

    nvp::store::RefreshState state;
    nvp::providers::Planner planner{state};
    nvp::store::Store store;
    nvp::providers::RunScratch scratch;
    FakeFs fs;
    ScriptedSource source;
    int64_t now = 0;  // UTC s
    bool clock_valid = true;
    bool link_up = true;
    bool has_location = true;
    bool has_coords = true;
    uint8_t blocked = 0;
    uint32_t district = 9206;
    std::vector<std::string> log;  // "2026-11-26 03:41 boot: complete"
    nvp::providers::RunResult last;
    int refreshes = 0;
    int total_requests = 0;
    nvp::providers::Trigger last_trigger = nvp::providers::Trigger::None;

    Sim() {
        store.district_id = district;
        planner.set_night_minute(221);  // 03:41
    }

    // Starts the simulated device at a local date and time.
    void start(Date local_day, int hh, int mm) {
        now = static_cast<int64_t>(nvp::engine::to_day_number(local_day)) * 86400 + hh * 3600 + mm * 60 - kOffset;
        source.now_utc = now;
        planner.on_boot();
        if (link_up) planner.on_link_up();
    }

    Date today() const {
        const int64_t local = now + kOffset;
        return nvp::engine::to_date(static_cast<nvp::engine::DayNumber>(local / 86400));
    }

    // What the screen would show as calculated: a record below rank 40, or no record but coordinates
    // (the device then fills today from its own calculation, research R8).
    bool today_calculated() const {
        const DayRecord* r = nvp::store::find(store, today());
        if (r == nullptr) return has_coords;
        return nvp::engine::rank(r->source) < 40;
    }

    nvp::providers::PlannerView view() const {
        nvp::providers::PlannerView v;
        v.now_utc = now;
        v.utc_offset_s = kOffset;
        v.clock_valid = clock_valid;
        v.link_up = link_up;
        v.has_location = has_location;
        v.days_ahead = nvp::providers::diyanet_days_ahead(store, today());
        v.today_diyanet = v.days_ahead >= 0;
        v.today_calculated = today_calculated();
        return v;
    }

    // One decision; runs the refresh when the planner asks. Returns true when a refresh ran.
    bool step() {
        source.now_utc = now;
        const nvp::providers::Decision d = planner.decide(view());
        if (!d.start) return false;
        nvp::providers::RunInput in;
        in.district = district;
        in.today = today();
        in.now_utc = now;
        in.has_coords = has_coords;
        in.lat_e5 = 3992063;
        in.lon_e5 = 3285403;
        in.today_calculated = today_calculated();
        in.lite = d.lite;
        in.blocked = blocked;
        last = nvp::providers::refresh_run(source, fs, in, store, scratch, state);
        planner.on_finished(last.outcome, last.diyanet_stored, now, today_calculated());
        ++refreshes;
        total_requests += last.requests;
        last_trigger = d.trigger;
        char line[160];
        const int64_t local = now + kOffset;
        const Date t = today();
        std::snprintf(line, sizeof line, "%04d-%02d-%02d %02d:%02d %s: %s, %u requests, %d days ahead", t.year, t.month,
                      t.day, static_cast<int>((local % 86400) / 3600), static_cast<int>((local % 3600) / 60),
                      nvp::providers::trigger_text(d.trigger), nvp::store::refresh_outcome_text(last.outcome),
                      static_cast<unsigned>(last.requests), static_cast<int>(last.days_ahead));
        log.push_back(line);
        return true;
    }

    void advance_minutes(int64_t minutes) {
        for (int64_t i = 0; i < minutes; ++i) {
            step();
            now += 60;
        }
    }

    void link(bool up) {
        link_up = up;
        if (up) planner.on_link_up(); else planner.on_link_down();
    }

    void print_log(const char* title) const {
        std::printf("%s\n", title);
        for (const std::string& s : log) std::printf("  %s\n", s.c_str());
    }
};

}  // namespace nvp_test
