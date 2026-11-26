#include "nvp/providers/runner.h"

#include <cstring>

namespace nvp::providers {
namespace {

engine::Date plus_days(engine::Date d, int32_t days) { return engine::to_date(engine::to_day_number(d) + days); }

bool is_diyanet(const engine::DayRecord& r) { return engine::rank(r.source) >= 40; }

// True when some day in [from, to] has no record at all.
bool has_gap(const store::Store& s, engine::Date from, engine::Date to) {
    for (engine::DayNumber n = engine::to_day_number(from); n <= engine::to_day_number(to); ++n) {
        if (store::find(s, engine::to_date(n)) == nullptr) return true;
    }
    return false;
}

bool diyanet_on(const store::Store& s, engine::Date d) {
    const engine::DayRecord* r = store::find(s, d);
    return r != nullptr && is_diyanet(*r);
}

class Run {
public:
    Run(Source& src, const RunInput& in, RunScratch& scratch, store::RefreshState& state, RunControl* ctl,
        RunResult& out)
        : src_(src), in_(in), scratch_(scratch), state_(state), ctl_(ctl), out_(out) {}

    // Whether `p` may be asked now; records the reason in the report when not.
    bool available(ProviderId p) {
        ProviderReport& rep = out_.providers[static_cast<size_t>(p)];
        const store::ProviderState& ps = state_.providers[static_cast<size_t>(p)];
        if ((in_.blocked & (1u << static_cast<unsigned>(p))) != 0) {
            rep.skip = Skip::Blocked;
        } else if (in_.now_utc < ps.cool_down_until) {
            rep.skip = Skip::CoolingDown;
        } else if (in_.now_utc < ps.skip_until && !(in_.today_calculated && diyanet_derived(p))) {
            rep.skip = Skip::Skipped24h;
        } else if (out_.requests + src_.max_requests(p) > kMaxRequestsPerRefresh) {
            rep.skip = Skip::Cap;
        } else {
            return true;
        }
        return false;
    }

    // Fetches and merges; true when a valid batch was merged.
    bool fetch(ProviderId p, engine::Date from, engine::Date to, bool allow_fallback) {
        ProviderReport& rep = out_.providers[static_cast<size_t>(p)];
        if (ctl_ != nullptr && !ctl_->should_continue()) {
            out_.abandoned = true;
            return false;
        }
        FetchRequest req;
        req.provider = p;
        req.district = in_.district;
        req.today = in_.today;
        req.from = from;
        req.to = to;
        req.allow_fallback = allow_fallback;
        req.has_coords = in_.has_coords;
        req.lat_e5 = in_.lat_e5;
        req.lon_e5 = in_.lon_e5;
        rep.asked = true;
        rep.skip = Skip::None;
        rep.result = src_.fetch(req, scratch_.incoming, kMaxBatchRecords);
        out_.requests = static_cast<uint8_t>(out_.requests + rep.result.requests);
        if (rep.result.outcome != FetchOutcome::Ok) {
            failed_[static_cast<size_t>(p)] = true;
            if (rep.result.outcome == FetchOutcome::RateLimited) rate_limited_[static_cast<size_t>(p)] = true;
            return false;
        }
        const size_t n = rep.result.count;

        // Plausibility of Diyanet-derived batches (FR-016), unless the coordinates are suspect.
        if (diyanet_derived(p) && in_.has_coords && !state_.coords_suspect) {
            const calc::Coordinates where{in_.lat_e5 / 100000.0, in_.lon_e5 / 100000.0};
            rep.plausibility = check_plausible(scratch_.incoming, n, where, in_.utc_offset_s);
            if (!rep.plausibility.ok) {
                if (p == ProviderId::Imsakiyem) {
                    for (size_t i = 0; i < n; ++i) scratch_.held[i] = scratch_.incoming[i];
                    held_ = n;
                } else if (held_ > 0 && batches_agree(scratch_.held, held_, scratch_.incoming, n)) {
                    // Two Diyanet-derived mirrors agree: trust them over the coordinates (owner
                    // direction). Both batches are kept and the coordinates are marked suspect.
                    state_.coords_suspect = true;
                    out_.coords_suspect_set = true;
                    merge_batch(ProviderId::Imsakiyem, scratch_.held, held_);
                    failed_[static_cast<size_t>(ProviderId::Imsakiyem)] = false;
                    out_.providers[static_cast<size_t>(ProviderId::Imsakiyem)].result.outcome = FetchOutcome::Ok;
                    merge_batch(p, scratch_.incoming, n);
                    return true;
                }
                rep.result.outcome = FetchOutcome::Failed;
                rep.result.reason = Reason::Implausible;
                failed_[static_cast<size_t>(p)] = true;
                return false;
            }
        }
        merge_batch(p, scratch_.incoming, n);
        return true;
    }

    void merge_batch(ProviderId p, const engine::DayRecord* recs, size_t n) {
        ProviderReport& rep = out_.providers[static_cast<size_t>(p)];
        ok_[static_cast<size_t>(p)] = true;
        const store::MergeStats m = store::merge(scratch_.merged, recs, n, in_.now_utc, in_.today);
        rep.records = static_cast<uint16_t>(rep.records + n);
        if (m.added + m.replaced + m.pruned + m.dropped > 0) changed_ = true;
        if (diyanet_derived(p) && m.added + m.replaced + m.kept > 0) out_.diyanet_stored = true;
        if (m.mismatches > 0) {
            if (!out_.has_mismatch && m.has_mismatch) {
                out_.has_mismatch = true;
                out_.mismatch_stored = m.mismatch_stored;
                out_.mismatch_incoming = m.mismatch_incoming;
            }
            out_.mismatches = static_cast<uint16_t>(out_.mismatches + m.mismatches);
        }
    }

    // Counts a failed refresh for a provider that was needed but failed or could not be asked.
    void update_state() {
        for (size_t i = 0; i < kFetchedProviders; ++i) {
            store::ProviderState& ps = state_.providers[i];
            const Skip skip = out_.providers[i].skip;
            if (ok_[i]) {
                ps.failures = 0;
                ps.skip_until = 0;
                continue;
            }
            const bool counted = failed_[i] || skip == Skip::Blocked;
            if (!counted) continue;
            if (ps.failures < 255) ++ps.failures;
            if (rate_limited_[i]) ps.cool_down_until = in_.now_utc + kRateLimitCoolDown;
            if (ps.failures >= kFailuresBeforeSkip && ps.skip_until <= in_.now_utc)
                ps.skip_until = in_.now_utc + kSkipAfterFailures;
        }
    }

    bool needed_failed(ProviderId p) const {
        const ProviderReport& rep = out_.providers[static_cast<size_t>(p)];
        if (rep.asked) return !ok_[static_cast<size_t>(p)];
        return rep.skip == Skip::Blocked || rep.skip == Skip::CoolingDown || rep.skip == Skip::Skipped24h ||
               rep.skip == Skip::Cap;
    }

    bool changed() const { return changed_; }

private:
    Source& src_;
    const RunInput& in_;
    RunScratch& scratch_;
    store::RefreshState& state_;
    RunControl* ctl_;
    RunResult& out_;
    bool ok_[kProviderCount] = {};
    bool failed_[kProviderCount] = {};
    bool rate_limited_[kProviderCount] = {};
    bool changed_ = false;
    size_t held_ = 0;
};

}  // namespace

const char* skip_text(Skip s) {
    switch (s) {
        case Skip::None: return "asked";
        case Skip::NotNeeded: return "not needed";
        case Skip::Disabled: return "disabled";
        case Skip::Blocked: return "blocked";
        case Skip::CoolingDown: return "cooling down";
        case Skip::Skipped24h: return "skipped 24 h";
        case Skip::NoCoords: return "no coordinates";
        case Skip::Cap: return "request cap";
        case Skip::Covered: return "covered";
    }
    return "?";
}

int32_t diyanet_days_ahead(const store::Store& s, engine::Date today) {
    if (!diyanet_on(s, today)) return -1;
    int32_t k = 0;
    while (k < 400 && diyanet_on(s, plus_days(today, k + 1))) ++k;
    return k;
}

RunResult refresh_run(Source& source, store::FileSystem& fs, const RunInput& in, store::Store& s,
                      RunScratch& scratch, store::RefreshState& state, RunControl* control) {
    RunResult out;
    scratch.merged = s;
    scratch.merged.district_id = in.district;
    const int64_t kept_refresh = s.last_refresh_utc;
    Run run(source, in, scratch, state, control, out);

    // 1. The official API: an empty slot (FR-001).
    out.providers[static_cast<size_t>(ProviderId::Diyanet)].skip = Skip::Disabled;

    // 2. İmsakiyem.
    if (run.available(ProviderId::Imsakiyem)) {
        // From yesterday (T045, constitution II): after a calculated period yesterday's record must be
        // the published one too, because the engine starts the night at yesterday's Yatsı.
        const engine::Date from = plus_days(in.today, -1);
        run.fetch(ProviderId::Imsakiyem, from, plus_days(from, kImsakiyemDays), !in.lite);
    }
    const bool imsakiyem_failed = run.needed_failed(ProviderId::Imsakiyem);

    // 3. emushaf.
    if (!out.abandoned &&
        (imsakiyem_failed || diyanet_days_ahead(scratch.merged, in.today) < kGoalDaysAhead)) {
        if (run.available(ProviderId::Emushaf)) {
            run.fetch(ProviderId::Emushaf, in.today, plus_days(in.today, kEmushafDays), true);
        }
    }

    // 4. Aladhan, only for gaps no Diyanet-derived provider could fill (FR-002).
    ProviderReport& al = out.providers[static_cast<size_t>(ProviderId::Aladhan)];
    if (!out.abandoned &&
        (!diyanet_on(scratch.merged, in.today) || !diyanet_on(scratch.merged, plus_days(in.today, 1)))) {
        if (!in.has_coords) {
            al.skip = Skip::NoCoords;
        } else {
            engine::Date months[2];
            size_t n = 0;
            const uint8_t dim = engine::days_in_month(in.today.year, in.today.month);
            months[n++] = engine::Date{in.today.year, in.today.month, 1};
            if (dim - in.today.day < 7) {
                const engine::Date next = plus_days(engine::Date{in.today.year, in.today.month, dim}, 1);
                months[n++] = next;
            }
            bool any = false;
            for (size_t i = 0; i < n && !out.abandoned; ++i) {
                const engine::Date first = months[i];
                const engine::Date last{first.year, first.month, engine::days_in_month(first.year, first.month)};
                const engine::Date from = i == 0 ? in.today : first;
                if (!has_gap(scratch.merged, from, last)) continue;
                any = true;
                if (!run.available(ProviderId::Aladhan)) break;
                run.fetch(ProviderId::Aladhan, first, last, true);
            }
            if (!any && al.skip == Skip::NotNeeded) al.skip = Skip::Covered;
        }
    }

    if (out.abandoned) {
        out.outcome = store::RefreshOutcome::Skipped;
        out.diyanet_stored = false;
        return out;
    }
    run.update_state();

    // The goal (FR-012, research R6).
    out.days_ahead = diyanet_days_ahead(scratch.merged, in.today);
    const bool diyanet_failed = run.needed_failed(ProviderId::Imsakiyem) || run.needed_failed(ProviderId::Emushaf);
    const bool goal = out.days_ahead >= kGoalDaysAhead || !diyanet_failed;
    bool asked_any = false;
    for (const ProviderReport& r : out.providers) asked_any = asked_any || r.asked;

    // Save once, if anything changed (FR-008).
    if (run.changed()) {
        out.changed = true;
        scratch.merged.last_refresh_utc = out.diyanet_stored ? in.now_utc : kept_refresh;
        if (control != nullptr) control->before_save();
        out.save = store::save(fs, scratch.merged);
        if (out.save == store::SaveResult::Ok) {
            s = scratch.merged;
            out.saved = true;
        } else {
            out.diyanet_stored = false;
            out.days_ahead = diyanet_days_ahead(s, in.today);
        }
    }

    if (!asked_any) {
        out.outcome = store::RefreshOutcome::Skipped;
    } else if (goal && (out.saved || !out.changed)) {
        out.outcome = store::RefreshOutcome::Complete;
    } else if (out.diyanet_stored) {
        out.outcome = store::RefreshOutcome::Partial;
    } else {
        out.outcome = store::RefreshOutcome::Failed;
    }
    return out;
}

}  // namespace nvp::providers
