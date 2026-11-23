# Slice 7: Automatic refresh and fallback

Goal: the device keeps its own data current, switches between sources without anyone noticing, and labels the screen honestly when it has to fall back to calculated times. See `docs/specification/06-roadmap.md`.

This is the largest slice. Most of it is logic that runs and is tested on the development machine.

## Step 1: Specify

**Paste into Claude Code**

```text
/speckit.specify Automatic refresh and fallback of prayer-time data for the Namaz Vakti Panel.

Read first: docs/specification/03-functional-spec.md §6 and §7; docs/specification/04-data-sources.md completely; docs/specification/02-ui-spec.md §6 and reference renders 10, 11 and 12.

Why: a device that shows correct times today but needs attention next month, or goes blank when a volunteer-run service is down, has failed. The owner should never think about data. The free sources this product relies on have no guarantees, so the device must refresh itself on a schedule, use several sources in a fixed order, fall back to calculation as a last resort, and say clearly when what it shows is approximate.

User stories, in priority order:

1. Looks after itself. As the owner, I never refresh anything. The device updates its stored times weekly at night, straight after setup or a location change, at start-up when needed, and whenever fewer than thirty days ahead are stored. Acceptance: over simulated weeks the refreshes happen at the specified moments and nowhere else; steady-state traffic stays within the stated request budget.

2. One source down goes unnoticed. As the owner, when the first Diyanet-derived source is unavailable or its data ends (for example at year end), the device gets the same published times from the second one, and I see no difference. Acceptance: with the first source blocked, the store still reaches at least thirty days ahead with Diyanet-derived records.

3. Approximate, and says so. As the owner, if no Diyanet-derived source can supply today's times, the device shows calculated times, marks every time with the approximate sign, and says so in the status line. As soon as a Diyanet-derived source responds again, the published times replace the calculated ones and the marking disappears, with no action from me. Acceptance: reference 12 while calculated; return to reference 01 afterwards.

4. Works with no internet at all. As the owner, if the internet is unreachable and the store has run out, the device calculates the times itself from my district's coordinates, tuned to the published times it has seen for my district, and marks them approximate.

5. Warned in time. As the owner, when the device has been unable to refresh and the stored times run out within a week, the status line tells me how many days are left. Acceptance: reference 11.

6. Published times always win. As the owner, a calculated value never replaces a published one for the same day, and published values from the device's sources are cross-checked; a disagreement is recorded and visible to a developer.

7. Wrong data is caught. As the owner, a response that would put clearly wrong times on screen (another place, shifted hours, a different day) is rejected even if it is well-formed, by comparison with what calculation says is plausible for my district.

8. Gentle on the services. As a user of free services, my device calls them rarely, never in parallel, backs off when they fail or ask it to slow down, and spreads its scheduled calls so that many such devices would not all call at the same minute.

Requirements covered: DATA-03, DATA-04, DATA-05, DATA-06, LOC-03, and 04-data-sources.md §2 to §7 and §12 (every row of the "what the owner sees" table). The official Diyanet API is excluded and stays an optional later feature; its place in the order must be kept open.

Edge cases: year end, when the first source stops at 31 December; coordinates cannot be resolved; coordinates resolve to a place outside Türkiye; the clock is invalid (no refresh, no calculation); Wi-Fi flapping every few seconds; a source that answers with an error page and status 200; a source that is slow; both mirrors disagree for today; calculated offsets from calibration are implausibly large; location changed while a refresh is running.

Out of scope: the official Diyanet API, settings UI for any of this (manual refresh and coordinate override arrive with the settings page), locations outside Türkiye.
```

## Step 2: Clarify

```text
/speckit.clarify Focus on the refresh goal and stop conditions, and on the precise moments the screen enters and leaves the calculated and cache-ending states.
```

Suggested answers if asked: the numbers in `04-data-sources.md` §5 are the defaults; the `calculated` state depends only on the rank of today’s record; while calculated records are in use the Diyanet-derived sources are retried hourly; calibration needs at least 14 published days.

## Step 3: Plan

```text
/speckit.plan Plan automatic refresh and fallback.

Constraints and research:
- Do the live verification in docs/specification/04-data-sources.md §13 items 2, 4 and 6 now, for my district: the second mirror's schema, the calculation service's parameters and response format, and how far its results are from the published times over a month (report the per-prayer differences). Record real responses as fixtures. Correct 04-data-sources.md where reality differs.
- Research and choose the calculation library per 04-data-sources.md §3.5: maintained, tested, licence compatible, small. Pin it. Compare its output for my district over the last 30 stored days with the published times and report the per-prayer median difference before and after calibration.
- Refresh planner in lib/providers as a deterministic state machine driven by an injected clock and an abstract fetch interface, with no knowledge of Wi-Fi or tasks. It implements the triggers, the ordered steps, the stop conditions, the request cap, backoff, cool-downs and the night-window scheduling with per-device jitter from 04-data-sources.md §5. Its host tests script time and responses: all sources up; each source down in turn; both mirrors down with a full store, with a nearly empty store, with an empty store; rate-limit responses; malformed data; year end; Wi-Fi reconnect storms; location change mid-refresh. Assert the request sequence and the resulting store for each.
- Providers: add parsers for the second mirror and the calculation service in lib/providers with fixture-based tests; the on-device calculation and calibration maths in lib/core with tests.
- Coordinates per 04-data-sources.md §6, stored with the location.
- Plausibility check per 04-data-sources.md §7 added to validation.
- Merge and mismatch logging per §4.
- System states: complete the real evaluation order of 02-ui-spec.md §6 with calculated and cache_ending. The approximate marking and the 44 px time size apply only in the calculated state.
- Persistence: last successful refresh, per-source failure counters, cool-down deadlines and calibration offsets survive restarts.
- Device integration: the app task feeds events (boot complete, Wi-Fi up/down, minute tick, location changed) to the planner and executes the fetches it asks for, one at a time.
- Developer serial commands: block or unblock a named source in software (so fallbacks can be exercised without touching the router), show planner state and next scheduled refresh, show calibration offsets, show mismatch count.
- Request budget: state the expected requests per week in the normal case and the worst case, and add a test that fails if a scenario exceeds the cap.
```

## Steps 4 and 5

```text
/speckit.tasks
```

```text
/speckit.analyze
```

## Step 6: Implement

```text
/speckit.implement Stage 1, host only: the refresh planner with its scenario tests against a scripted fetch layer, merge rules with ranks, persistence of planner state. Show me the scenario list with pass or fail and the request sequence for the year-end scenario.
```

```text
/speckit.implement Stage 2, host only: second mirror parser, calculation service parser, on-device calculation, calibration, coordinates, plausibility check. Report the per-prayer differences you measured for my district.
```

```text
/speckit.implement Stage 3, device: wire the planner into the app task, complete the system states, add the developer commands. Give me the owner checklist with the exact commands for each step.
```

## Step 7: Owner checklist

Use the developer command that blocks sources in software.

- [ ] Normal operation: status line shows a recent update; status panel shows a Diyanet-derived source and a last stored date at least thirty days ahead.
- [ ] First mirror blocked, refresh forced: no change on screen; status panel now names the second mirror as source for newly fetched days.
- [ ] Both mirrors blocked, store erased, refresh forced: the screen matches `12-state-calculated.png` in layout; times are within a few minutes of Diyanet’s website.
- [ ] Mirrors unblocked: within the retry interval the approximate marking disappears and the times equal Diyanet’s website again, without my doing anything.
- [ ] Wi-Fi off and store erased while the clock stays valid: calculated times from the device itself are shown, marked approximate.
- [ ] Simulated date moved to within a week of the last stored day with sources blocked: status line matches `11-state-cache-ending.png` with the right number of days.
- [ ] The next scheduled refresh shown by the developer command falls between 03:00 and 05:00.
- [ ] No sound.

## Step 8: Converge and merge

```text
/speckit.converge
```

## Done when

Every row of the table in `04-data-sources.md` §12 is demonstrated by a host test, the checklist is complete, and `check` passes.

Next: `slice-08-settings-page.md`.
