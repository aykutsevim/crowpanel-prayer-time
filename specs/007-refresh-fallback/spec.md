# Feature Specification: Automatic refresh and fallback

**Feature Branch**: `feat/007-refresh-fallback`

**Created**: 2026-11-26

**Status**: Draft

**Input**: User description: "Slice 7 on new feature branch, Diyanet data sources have always highest priority" (Slice 7 of `docs/specification/06-roadmap.md`; brief in `docs/development/prompts/slice-07-refresh-fallback.md`)

## Context

Slice 6 put Diyanet's published times on the screen from one source, refreshed only at start, after a failure once a minute later, and on a development command. A device that is correct today but needs attention next month, or goes blank when a volunteer-run service is down, has failed. The owner should never think about data.

This feature makes the device keep its data current by itself, use several sources in a fixed order, fall back to calculated times only as a last resort, and say clearly when what it shows is approximate.

**Owner direction for this slice: Diyanet data always has the highest priority.** Every Diyanet-derived source is tried before any calculated one; a calculated value never stands where a Diyanet-derived value for the same day exists or can still be obtained; and as soon as Diyanet-derived data is available again it replaces calculated values without any action from the owner.

Sources: `docs/specification/03-functional-spec.md` §6 and §7; `docs/specification/04-data-sources.md` (all of it, in particular §2–§7 and §12); `docs/specification/02-ui-spec.md` §6 and references 01, 10, 11 and 12.

Requirements covered: DATA-03, DATA-04, DATA-05, DATA-06, LOC-03, and every row of the "what the owner sees" table of 04 §12.

What earlier slices provide: the location with its Diyanet district id (Slice 4), a valid clock (Slice 5), the store with its atomic save, the first Diyanet-derived source, batch validation, the merge rules and the `ok`, `offline` and `no_data` states (Slice 6), the engine and the main screen (Slices 2 and 3).

## Clarifications

### Session 2026-11-26

- Q: What does "Diyanet data sources have always highest priority" mean in behaviour? → A: (owner direction, with the brief) Diyanet-derived sources are always tried before calculated ones; a calculated record never replaces or hides a Diyanet-derived record for the same day; calculation fills only days no Diyanet-derived source could supply; Diyanet-derived sources keep being retried while calculated times are in use, and their data replaces calculated data at once.
- Defaults taken from the brief (not asked): the numbers of 04 §5 are the defaults; the `calculated` state depends only on the source of today's record; while calculated records are in use for today, the Diyanet-derived sources are retried hourly; calibration needs at least 14 published days.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Looks after itself (Priority: P1)

As the owner, I never refresh anything. The device updates its stored times weekly at night, straight after setup or a location change, at start-up when needed, and whenever fewer than thirty days ahead are stored.

**Why this priority**: Without it the stored times run out and the screen fails weeks later (DATA-03).

**Independent Test**: In host tests that simulate weeks of time, refreshes happen at the specified moments and nowhere else, and the request count stays within the budget.

**Acceptance Scenarios**:

1. **Given** a store covering more than 30 days ahead and a last successful refresh 7 days ago, **When** the device's own night minute (between 03:00 and 04:59 local time) arrives, **Then** one refresh runs, and none runs at any other time of that week.
2. **Given** a completed setup or a location change, **When** Wi-Fi and the clock are up, **Then** a refresh starts at once.
3. **Given** fewer than 30 days ahead stored, **When** the night window comes, **Then** a refresh runs once that night, every night until the coverage is restored.
4. **Given** a start-up or a Wi-Fi reconnection, **When** today has no Diyanet-derived record, or fewer than 30 days ahead are stored, or the last successful refresh is more than 7 days old, **Then** a refresh runs (on reconnection at most once per 10 minutes); otherwise none runs.
5. **Given** a normal week, **When** the requests are counted, **Then** there are one or two; no single refresh makes more than 12.

---

### User Story 2 - One source down goes unnoticed (Priority: P1)

As the owner, when the first Diyanet-derived source is unavailable or its data ends (for example at year end), the device gets the same published times from the second one, and I see no difference.

**Why this priority**: The free sources have no guarantees; one of them being down must never show (DATA-04).

**Independent Test**: With the first source blocked by a development command, force a refresh.

**Acceptance Scenarios**:

1. **Given** the first source unavailable, **When** a refresh runs, **Then** the store still reaches at least 30 days ahead with Diyanet-derived records, the screen stays `ok`, and the status panel names the second source for the days it supplied.
2. **Given** the first source ends at 31 December and today is in late December, **When** a refresh runs, **Then** the second source extends the store into January.
3. **Given** both sources supply the same day with different times, **When** they are merged, **Then** the newer one is kept and a mismatch is recorded with both sets of times, visible to a developer.

---

### User Story 3 - Approximate, and says so (Priority: P1)

As the owner, if no Diyanet-derived source can supply today's times, the device shows calculated times, marks every time with the approximate sign and says so in the status line. As soon as a Diyanet-derived source responds again, the published times replace the calculated ones and the marking disappears, with no action from me.

**Why this priority**: Constitution II: approximate times must never look official, and official times must come back by themselves.

**Independent Test**: Block both Diyanet-derived sources, erase the store and refresh; then unblock them.

**Acceptance Scenarios**:

1. **Given** both Diyanet-derived sources unavailable and no Diyanet-derived record for today, **When** a refresh runs, **Then** calculated times from the online calculation service are stored for the missing days and the screen matches reference 12 (status `Yaklaşık vakitler, hesaplama ile`, every time prefixed `≈` and drawn smaller).
2. **Given** that state, **When** a Diyanet-derived source answers again, **Then** within the hourly retry its records replace the calculated ones and the screen returns to reference 01 with no action from the owner.

---

### User Story 4 - Works with no internet at all (Priority: P2)

As the owner, if the internet is unreachable and the store has run out, the device calculates the times itself from my district's coordinates, tuned to the published times it has seen for my district, and marks them approximate.

**Why this priority**: The last resort; rare but it keeps the screen useful (constitution IV).

**Independent Test**: Wi-Fi off and the store erased while the clock stays valid.

**Acceptance Scenarios**:

1. **Given** no network, a valid clock, known coordinates and no record for today, **When** the screen needs today's times, **Then** it shows times calculated on the device, in the `calculated` state.
2. **Given** at least 14 Diyanet-derived days seen for the district, **When** the device calculates, **Then** it applies per-prayer offsets learned from those days, so that over the most recent 30 published days the median difference of each time is at most 1 minute.

---

### User Story 5 - Warned in time (Priority: P2)

As the owner, when the device has been unable to refresh and the stored times run out within a week, the status line tells me how many days are left.

**Why this priority**: The owner can act (check the router) before the screen degrades.

**Independent Test**: Move the simulated date to within a week of the last stored day with the sources blocked.

**Acceptance Scenarios**:

1. **Given** the last refresh failed or Wi-Fi is down, and N ≤ 7 days are stored after today, **When** the screen is drawn, **Then** it matches reference 11 with `Kayıtlı vakitler N gün sonra bitiyor` (`… bugün bitiyor` when N = 0).

---

### User Story 6 - Published times always win (Priority: P1)

As the owner, a calculated value never replaces a published one for the same day, and published values from the device's sources are cross-checked; a disagreement is recorded and visible to a developer.

**Why this priority**: The owner's direction for this slice; DATA-05.

**Independent Test**: Host tests of the merge with every combination of source ranks.

**Acceptance Scenarios**:

1. **Given** a stored Diyanet-derived record for a day, **When** a calculated record for that day arrives, however recent, **Then** the stored record is kept.
2. **Given** a stored calculated record, **When** a Diyanet-derived record for that day arrives, **Then** it replaces the calculated one.
3. **Given** a refresh, **When** the providers are tried, **Then** every available Diyanet-derived provider is tried before any calculated one, and a calculated provider is asked only for days still without a Diyanet-derived record.

---

### User Story 7 - Wrong data is caught (Priority: P2)

As the owner, an answer that would put clearly wrong times on screen (another place, shifted hours, a different day) is rejected even if it is well-formed, by comparison with what calculation says is plausible for my district.

**Why this priority**: Wrong times shown as official are the worst failure (DATA-06).

**Independent Test**: Host tests with recorded answers altered by a shifted hour, a shifted day and another district's times.

**Acceptance Scenarios**:

1. **Given** known coordinates and a valid clock, **When** an answer's first, middle or last record differs from the uncalibrated calculation by more than 20 minutes on any time, **Then** the whole answer is rejected with the reason logged and the store is unchanged.

---

### User Story 8 - Gentle on the services (Priority: P2)

As a user of free services, my device calls them rarely, never in parallel, backs off when they fail or ask it to slow down, and spreads its scheduled calls so that many such devices would not all call at the same minute.

**Why this priority**: Constitution VIII; the services are run by volunteers.

**Independent Test**: Host scenarios with rate-limit answers, repeated failures and Wi-Fi reconnect storms; the request sequence is asserted.

**Acceptance Scenarios**:

1. **Given** a provider answers "too many requests" or "unavailable", **When** the next 30 minutes pass, **Then** it is not called again.
2. **Given** a provider failed five refreshes in a row, **When** the next 24 hours pass, **Then** it is skipped, and tried again afterwards.
3. **Given** a refresh that did not reach its goal, **When** it is retried, **Then** the retries come after 5 min, 15 min, 1 h, 3 h and then every 6 h, with one extra attempt allowed per Wi-Fi reconnection.
4. **Given** Wi-Fi reconnecting every few seconds, **When** an hour passes, **Then** at most one refresh per 10 minutes is caused by reconnections.
5. **Given** two devices with different hardware addresses, **When** their weekly minute is computed, **Then** the minutes differ in general and both lie between 03:00 and 04:59.

---

### Edge Cases

- **Year end**: the first source stops at 31 December; the second extends coverage into January. If neither has the new year yet, the existing days are used until they run out; then `cache_ending`, then calculated times.
- **Coordinates cannot be resolved**: the calculated fallbacks and the plausibility check are skipped; with no Diyanet-derived record for today the screen shows `no_data` (04 §12).
- **Coordinates resolve outside Türkiye**: rejected; treated as unresolved.
- **The clock is invalid**: no refresh and no calculation; the screen shows `no_time` as before.
- **Wi-Fi flapping every few seconds**: at most one refresh per 10 minutes from reconnections; a refresh in progress is not restarted.
- **A source answers with an error page and success status**: parsing fails; counted as a failure of that provider.
- **A slow source**: the request times out (10 s connect, 15 s read) and counts as a failure; the next provider is tried.
- **The two mirrors disagree for today**: the newer is kept and a mismatch is recorded; a difference of more than 2 minutes on today is flagged for the settings page (Slice 8).
- **Calibration offsets implausibly large** (beyond ±10 minutes): rejected as a sign of wrong coordinates; the uncalibrated calculation is used and the rejection logged.
- **Two Diyanet-derived sources agree with each other but both fail the plausibility check**: the published data is trusted over the calculation (owner direction); the coordinates are treated as suspect, are not used for calculation or plausibility until resolved again, and the event is logged.
- **The location changes while a refresh runs**: the running refresh is abandoned without storing anything for the old district, and a new refresh starts for the new district.
- **Power cut during a refresh**: the store and the persisted refresh state are the old or the new version (DATA-07 as in Slice 6).

## Requirements *(mandatory)*

### Functional Requirements

**Priority of Diyanet data (owner direction)**

- **FR-001**: Within one refresh, every enabled Diyanet-derived provider MUST be tried before any calculated provider, in the order of 04 §5.2: the official API (kept as an empty slot in this feature), İmsakiyem, emushaf; then the online calculation service; then the device's own calculation.
- **FR-002**: A calculated record MUST NOT replace a stored Diyanet-derived record for the same day, regardless of fetch time; a Diyanet-derived record MUST replace a stored calculated one (DATA-05, 04 §4). Calculated providers are asked only for days that still have no Diyanet-derived record after the Diyanet-derived steps.
- **FR-003**: While today's record is calculated, the Diyanet-derived providers MUST be retried hourly (04 §5.3), each retry asking only İmsakiyem's main interface and emushaf (analysis I2); a successful answer replaces the calculated records and the screen leaves the `calculated` state within one second of the store being written, with no action from the owner.

**Triggers and scheduling**

- **FR-004**: A refresh MUST start when (a) setup completes or the location changes; (b) at start-up after Wi-Fi and a valid clock, if today has no Diyanet-derived record, or fewer than 30 days ahead are stored, or the last successful refresh is more than 7 days old; (c) 7 days after the last successful refresh at the device's night minute; (d) once per night in the night window while fewer than 30 days ahead are stored; (e) on a Wi-Fi reconnection after an outage, only if a start-up condition holds and at most once per 10 minutes; (f) on a development command (the settings page's `Şimdi güncelle` arrives in Slice 8; the same path is limited to once per minute) (DATA-03, 04 §5.1).
- **FR-005**: The night minute MUST lie between 03:00 and 04:59 local time and be derived from the device's hardware address, so that different devices call at different minutes.
- **FR-006**: No refresh MAY run while the clock is invalid; fetches run one at a time and never block the screen (as in Slice 6).

**One refresh**

- **FR-007**: A refresh's goal is Diyanet-derived records from today through at least today + 30 days, and as far ahead as the first provider offers. Steps: İmsakiyem (one request to the end of the source's data, up to a year, with its fallback interface as in Slice 6); emushaf if the coverage is still under 30 days ahead or İmsakiyem failed; the online calculation service for the current month (and the next when today is within 7 days of month end) if today or tomorrow still has no Diyanet-derived record; the device's own calculation for today through today + 7 when today still has no record at all, done outside the refresh and kept in memory so that it also works offline (research R8). The calculated steps need coordinates (04 §5.2).
- **FR-008**: The store MUST be written once at the end of a refresh, and only if something changed; a refresh MUST NOT make more than 12 requests.
- **FR-009**: The second mirror and the online calculation service MUST be read in the same streaming way as the first source, and validated by the rules of Slice 6 plus FR-016 before anything is stored.

**Failures and politeness**

- **FR-010**: A provider call fails on a connection timeout of 10 s, a read timeout of 15 s, a non-success status, a body over 512 kB, a parse error or a validation failure (04 §5.3).
- **FR-011**: After a "too many requests" or "unavailable" answer, that provider MUST NOT be called again for 30 minutes.
- **FR-012**: The goal counts as reached when the store holds Diyanet-derived records through today + 30, or when every Diyanet-derived provider asked answered validly (nothing more is published yet, for example at year end) (planning, research R6). A refresh that ends without reaching its goal MUST be retried after 5 min, 15 min, 1 h, 3 h, then every 6 h; a Wi-Fi reconnection allows one extra attempt; FR-003's hourly retry replaces this schedule while today is calculated.
- **FR-013**: A provider that failed five refreshes in a row MUST be skipped for 24 hours, then tried again. **Exception (owner direction, analysis C1):** while today's record is calculated, the skip does not apply to Diyanet-derived providers; they keep the hourly retry of FR-003, so published times return within about an hour of a mirror recovering.
- **FR-014**: The last successful refresh, the per-provider failure counters, the cool-down deadlines, the retry step and the calibration offsets MUST survive a restart and a power cut (DATA-07).

**Coordinates, calculation and plausibility**

- **FR-015**: The district's coordinates MUST come from the place list bundled with the firmware, which carries them for every place (planning, research R4); only for a place missing from it are they resolved once over the network from İmsakiyem's district detail, accepting high or medium confidence. A result outside Türkiye's bounding box is rejected. They belong to the location (LOC-03); a location change replaces them. (The online calculation service's address lookup of 04 §6 was dropped in planning: it returned placeholder coordinates.)
- **FR-016**: With coordinates and a valid clock, a fetched Diyanet-derived batch MUST be rejected when, for its first, middle or last record, any time differs from the uncalibrated device calculation by more than 20 minutes (04 §7). Exception (owner direction): when two Diyanet-derived sources agree with each other on the checked days but both fail this check, the published data is kept, the coordinates are marked suspect and not used again until re-resolved, and the event is logged.
- **FR-017**: The device MUST calculate the six times for any date from the stored coordinates by a maintained, tested method matching the Diyanet parameters of 04 §3.5, chosen and pinned in the plan.
- **FR-018**: When at least 14 Diyanet-derived records are stored, the device MUST compute per-prayer offsets (median of published minus calculated over the most recent 30 such days) and apply them to its own calculation; offsets beyond ±10 minutes are rejected and logged (04 §3.5).

**States and screen**

- **FR-019**: The system state MUST follow the full order of 02 §6 from real inputs: `no_time`, `no_data`, `calculated` (today's record is not Diyanet-derived), `cache_ending` (the last refresh failed or Wi-Fi is down, and stored days after today ≤ 7), `offline`, `ok`. In `calculated` every time is prefixed with `≈` and drawn at 44 px (reference 12); in `cache_ending` the status text gives the number of days (reference 11).
- **FR-020**: The status panel's `Vakit kaynağı` MUST name the source of today's record by 02 §9.5, including the second mirror and the two calculated sources.
- **FR-021**: Every row of the 04 §12 table MUST hold (demonstrated as SC-007).

**Development tools and logging**

- **FR-022**: Development serial commands MUST let a developer block and unblock a named source in software, show the refresh state (next scheduled refresh, retry step, cool-downs, failure counters), show the coordinates and the calibration offsets, and show the mismatch count since start (SYS-06).
- **FR-023**: Each provider call MUST be logged as one line (provider, result, records, duration); each rejection with its reason; each mismatch with both sets of times (SYS-05, 04 §4).
- **FR-024**: The device MUST stay silent (SIL-01).

### Key Entities

- **Provider**: id, rank (Diyanet-derived ≥ 40, calculated < 40), order, failure counter, cool-down deadline, blocked flag (development).
- **Refresh state**: last successful refresh, last attempt and its outcome, retry step, next scheduled refresh, night minute.
- **Coordinates**: latitude, longitude, origin (district detail, address lookup), confidence, suspect flag.
- **Calibration**: six offsets in minutes, number of days used, date computed.
- **Mismatch**: date, both sets of times, sources; count since start.
- **Day record and store**: as in Slice 6; the record's source now takes any of the five providers.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In simulated weeks of host tests, every refresh happens at a moment allowed by FR-004 and none elsewhere; a normal week costs at most 2 requests and no refresh exceeds 12.
- **SC-002**: With the first source blocked, a refresh still leaves at least 30 days ahead of Diyanet-derived records, and the main screen shows no change (owner check).
- **SC-003**: With both mirrors blocked and the store erased, the screen matches reference 12 and every time is within 5 minutes of Diyanet's website for the district (owner check); after unblocking, the screen returns to reference 01 within 70 minutes with no action from the owner.
- **SC-004**: With the network off, the store erased and the clock valid, times calculated on the device are shown, marked approximate; after calibration, the per-prayer median difference from the published times over the last 30 stored days is at most 1 minute (host measurement for the owner's district).
- **SC-005**: With the date moved to within a week of the last stored day and the sources blocked, the status line matches reference 11 with the right number of days.
- **SC-006**: In host tests, no combination of ranks lets a calculated record replace a Diyanet-derived record, and every Diyanet-derived provider is asked before any calculated one in every scenario.
- **SC-007**: Every row of the 04 §12 table is demonstrated by a host test.
- **SC-008**: Every rejection reason, including each plausibility failure (shifted hour, shifted day, another district), is covered by a host test on altered copies of recorded real answers.
- **SC-009**: The next scheduled refresh shown by the development command lies between 03:00 and 04:59 (owner check).
- **SC-010**: The device makes no sound (owner check).

## Assumptions

- The defaults are the numbers in 04 §5 (triggers, goal, cap, backoff, cool-downs).
- The official Diyanet API is not part of this feature; its place first in the order is kept so that adding it later changes no other rule.
- The second mirror, the online calculation service and the calculation library are verified live and chosen during planning (04 §13 items 2, 4 and 6); the plan corrects 04 where reality differs.
- "Last refresh failed" for `cache_ending` means the most recent refresh did not reach its goal.
- Stored days after today, for `cache_ending`, count consecutive days with any record after today.
- The night window uses the configured local time zone (Slice 5).
- Mismatches more than 2 minutes on today are recorded now and shown in the settings page in Slice 8.

## Out of Scope

- The official Diyanet API (an optional later feature).
- Settings-page controls: manual refresh button, coordinate override, mismatch display (Slice 8).
- Locations outside Türkiye.
