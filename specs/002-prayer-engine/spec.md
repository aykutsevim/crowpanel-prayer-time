# Feature Specification: Prayer-time engine

**Feature Branch**: `feat/002-prayer-engine`

**Created**: 2026-11-24

**Status**: Draft

**Input**: User description: "Slice 2 on new feat branch" (Slice 2 of `docs/specification/06-roadmap.md`; brief in `docs/development/prompts/slice-02-prayer-engine.md`)

## Context

Everything the main screen shows is derived from one question: given the current local time and the published times for yesterday, today and tomorrow, which prayer period are we in, when did it start, when does it end, what comes next, and how long until then. If the answer is wrong by a minute at a boundary, the product is wrong. This slice decides that question once, in one place, and proves it with automated tests on the development machine. No board, network or drawing is involved.

Sources: `docs/specification/03-functional-spec.md` §3 (ENG-01 to ENG-08, the shared fixture), `docs/specification/02-ui-spec.md` §4.5 (columns), §5 (hero states) and §9 (text), `docs/specification/01-product-brief.md` (glossary).

Terms: a **vakit** is one of the six published times (İmsak, Güneş, Öğle, İkindi, Akşam, Yatsı) and the **period** that starts at it. The period that starts at Güneş is called Kuşluk. The Yatsı period runs from Yatsı to the next day's İmsak.

## User Scenarios & Testing *(mandatory)*

The "user" of this slice is the product itself (Slice 3 draws what the engine returns) and the owner, who must be able to trust that the screen is right at every second. All scenarios are checked by automated tests on the development machine.

### User Story 1 - Know the running period and the countdown at any moment (Priority: P1)

At any second of the day, the screen needs the running period, its start and end, the next vakit, the time remaining until it, the time elapsed, and how far through the period we are.

**Why this priority**: The hero of the main screen (period name, bounds, progress, countdown) is built from this and nothing else. Without it the product shows nothing useful.

**Independent Test**: Feed the shared fixture and a series of moments; compare the period, bounds, next vakit, remaining and elapsed seconds and the elapsed fraction with hand-computed values.

**Acceptance Scenarios**:

1. **Given** the shared fixture (yesterday Yatsı 19:15; today 06:16, 07:46, 12:54, 15:27, 17:51, 19:14; tomorrow İmsak 06:17), **When** the time is 14:32:07, **Then** the period is Öğle, its bounds are 12:54–15:27, the next vakit is İkindi, remaining is 00:54:53 and the elapsed fraction is 0.641 (to three decimals).
2. **Given** the fixture, **When** the time is one second before any of the six vakits (for example 12:53:59), **Then** the previous period is still running with 1 second remaining.
3. **Given** the fixture, **When** the time is exactly a vakit (for example 12:54:00), **Then** the new period is running, elapsed is 0 and remaining is the full length of the new period.
4. **Given** the fixture, **When** the time is 03:30:00, **Then** the period is Yatsı with bounds yesterday 19:15 to today 06:16, and the next vakit is today's İmsak.
5. **Given** the fixture, **When** the time is 21:30:00, **Then** the period is Yatsı with bounds today 19:14 to tomorrow 06:17, and the next vakit is tomorrow's İmsak.
6. **Given** a soon threshold of 15 minutes, **When** 15 minutes or less remain (for example 17:36:00 before Akşam at 17:51), **Then** the result is flagged "soon"; at 17:35:59 it is not.
7. **Given** any period, **When** less than 5 minutes have elapsed since it began (for example 12:58:59), **Then** the result is flagged "entered"; from 12:59:00 it is not.

---

### User Story 2 - Show each of the six times in the right state (Priority: P1)

The six columns under the timeline each show a time and a state (past, current, next, later) with a short note, so the owner sees at a glance what has passed and what comes next.

**Why this priority**: The columns are half of the main screen, and their special cases around midnight are where errors are most likely.

**Independent Test**: For the fixture at a set of moments, compare each column's displayed time, state, note and seconds-until with the expected values from `02-ui-spec.md` §4.5.

**Acceptance Scenarios**:

1. **Given** the fixture at 14:32:07, **Then** İmsak and Güneş are past, Öğle is current with the note `şu an`, İkindi is next with the note `54 dk sonra`, Akşam is later with `3 sa 18 dk sonra`, and Yatsı is later with `4 sa 41 dk sonra`.
2. **Given** the fixture at 12:55:00, **Then** Öğle is current with the note `vakit girdi`.
3. **Given** the fixture at 03:30:00 (before İmsak), **Then** no column is current, İmsak is next and the other five are later, and all six show today's times.
4. **Given** the fixture at 21:30:00 (after Yatsı), **Then** Yatsı is current, the İmsak column shows tomorrow's time 06:17 with state next and the note `yarın`, and the other four are past.
5. **Given** the fixture, **When** the time crosses any vakit, **Then** the column states change at that exact second and not before.

---

### User Story 3 - Stay correct across midnight and with missing days (Priority: P2)

The device runs for months. Each midnight it must move to the new day without a jump in the running night period, and it must behave sensibly when the day before or after is unknown, or when today itself is unknown.

**Why this priority**: These cases happen every night and whenever data is incomplete; getting them wrong shows wrong times at night or after an outage.

**Independent Test**: Run the engine at 23:59:59 and 00:00:00 with records for both days, and with yesterday, tomorrow or today missing or invalid; check the results against the rules below.

**Acceptance Scenarios**:

1. **Given** distinct records for today and tomorrow, **When** the time goes from 23:59:59 to 00:00:00, **Then** the Yatsı period continues with identical start and end at both seconds, remaining drops by exactly one second, and the six displayed times change to the new day's.
2. **Given** yesterday's record is missing, **When** the time is before today's İmsak, **Then** the period starts at today's Yatsı time taken as yesterday's, and the result is flagged approximate.
3. **Given** tomorrow's record is missing, **When** the time is after today's Yatsı, **Then** the period ends at today's İmsak time taken as tomorrow's, the İmsak column shows that time, and the result is flagged approximate.
4. **Given** today's record is missing, **When** the engine runs, **Then** it reports "no data" and nothing else.
5. **Given** a record whose six times are not strictly increasing, or a time outside 00:00–23:59, **When** it is offered, **Then** it is rejected and treated as missing.
6. **Given** yesterday, today and tomorrow have identical times, **When** the engine runs at any moment, **Then** all results follow the normal rules and none is flagged approximate.
7. **Given** tomorrow's İmsak differs from today's by plus or minus 2 minutes, **When** the time crosses midnight, **Then** the bounds stay continuous across midnight (scenario 1).

---

### User Story 4 - Produce the exact Turkish text the screen shows (Priority: P2)

Every word and number format on the main screen comes from the design specification and must match it byte for byte: period names, countdown labels, column notes, durations, times, Gregorian and Hijri dates.

**Why this priority**: The design is fixed (constitution III), and text produced in many places drifts. One table, tested once, prevents that.

**Independent Test**: Compare every produced string with the text in `02-ui-spec.md` §9, including the U+2019 apostrophe.

**Acceptance Scenarios**:

1. **Given** each of the six periods, **Then** the period name and the countdown label are exactly those of `02-ui-spec.md` §9.2 (for example `Kuşluk vakti` and `Öğleye kalan süre` for the period from Güneş).
2. **Given** a duration of 0 to 59 seconds, **Then** the note reads `1 dk’dan az sonra`; exactly 60 seconds reads `1 dk sonra`; 59 minutes reads `59 dk sonra`; exactly one hour reads `1 sa sonra`; one hour one minute reads `1 sa 1 dk sonra`; 10 hours 5 minutes reads `10 sa 5 dk sonra`.
3. **Given** the date 17 November 2026, **Then** the Gregorian date reads `17 Kasım 2026 Salı`; 31 December 2026 reads `31 Aralık 2026 Perşembe` and the next day `1 Ocak 2027 Cuma`; the last day of February in a leap year is formatted correctly.
4. **Given** the Hijri date 7 / month 6 / 1448, **Then** it reads `7 Cemaziyelahir 1448`.
5. **Given** a time of day and a remaining time, **Then** they read `HH:MM` and `HH:MM:SS`, zero-padded, 24-hour.

---

### User Story 5 - Mark the kerahat intervals (Priority: P3)

The day strip shows three hatched intervals in which prayer is discouraged: after sunrise, before noon prayer and before evening prayer.

**Why this priority**: It is an optional layer, on by default; the screen is usable without it.

**Independent Test**: With the fixture and given lengths, compare the three intervals with hand-computed bounds, including lengths of zero and the layer switched off.

**Acceptance Scenarios**:

1. **Given** the fixture and lengths 45/45/45 minutes, **Then** the intervals are 07:46–08:31, 12:09–12:54 and 17:06–17:51.
2. **Given** a length of zero for one interval, **Then** that interval is absent and the other two are unchanged.
3. **Given** the kerahat layer is switched off, **Then** no interval is returned.
4. **Given** an interval that would extend past midnight or before 00:00, **Then** it is clipped to the day.

---

### Edge Cases

- **Exactly at a vakit**: the new period is running; remaining is the full length of the new period; elapsed is 0; the column of the new vakit is current (or, at İmsak, the night period gives way to the İmsak period).
- **At 00:00:00**: the night period continues; the "before İmsak" column rules apply; the displayed times are the new day's.
- **Soon threshold of 0**: "soon" is never set.
- **Neighbour record with the wrong date** (for example a "yesterday" record that is not the day before today): treated as missing.
- **Time near the end of a minute at a boundary** (for example 12:53:59 vs 12:54:00): only the second matters; there is no rounding to the minute.
- **A remaining time of more than 10 hours** (the night period on winter evenings): the countdown still reads `HH:MM:SS`.
- **İmsak at 00:00 or Yatsı at 23:59**: valid; the rules still hold.
- **Leap years and month ends**: 29 February in a leap year, 28 February otherwise, and every month's last day roll over correctly; weekdays stay correct across years.
- **Kerahat lengths outside 0–90 minutes**: limited to that range.

## Requirements *(mandatory)*

### Functional Requirements

**Day records**

- **FR-001**: A day record MUST hold a calendar date, the six vakit times at minute precision in the order İmsak, Güneş, Öğle, İkindi, Akşam, Yatsı, an optional Hijri date (day, month, year) and a source. (03 §3)
- **FR-002**: A record MUST be accepted only if its six times are strictly increasing and each lies between 00:00 and 23:59; any other record MUST be treated as missing. (ENG-01)
- **FR-003**: A record offered as yesterday's or tomorrow's whose date is not the day before or after today MUST be treated as missing.

**Current period**

- **FR-004**: A vakit MUST begin at second 00 of its minute. At the exact second a vakit begins, the period that starts there MUST be the running period. (ENG-02)
- **FR-005**: The running period MUST be determined as follows (ENG-03):
  - before today's İmsak: the Yatsı period that began yesterday, from yesterday's Yatsı to today's İmsak, next vakit İmsak (today);
  - from today's vakit *k* up to (not including) vakit *k+1*: period *k*;
  - from today's Yatsı: the Yatsı period, from today's Yatsı to tomorrow's İmsak, next vakit İmsak (tomorrow).
- **FR-006**: For the running period the engine MUST report: the period, its start and end as absolute local date-times, the next vakit and whether it falls tomorrow, remaining seconds, elapsed seconds and the elapsed fraction from 0 to just below 1. Remaining plus elapsed MUST equal the period's length. (ENG-05)
- **FR-007**: The result MUST be flagged **soon** when remaining is at or below the soon threshold (minutes, default 15; a threshold of 0 never sets it), and **entered** when elapsed is below 5 minutes. (ENG-05, 02 §5)

**Missing records**

- **FR-008**: If yesterday's record is missing, today's Yatsı time MUST be used as yesterday's; if tomorrow's record is missing, today's İmsak time MUST be used as tomorrow's. Either substitution MUST set an **approximate bounds** flag in the result. (ENG-04)
- **FR-009**: If today's record is missing, the engine MUST report "no data" and no period, columns or intervals. (ENG-08)

**Columns**

- **FR-010**: For each of the six columns the engine MUST report the time to display, its state (`past`, `current`, `next`, `later`), its note and the seconds until it (zero for past and current columns). (ENG-05, 02 §4.5)
- **FR-011**: Column states MUST follow `02-ui-spec.md` §4.5:
  - `past`: its time is at or before now and it is not the current period;
  - `current`: now is inside its period and that period started today;
  - `next`: the first upcoming time;
  - `later`: every other upcoming time.
- **FR-012**: After Yatsı, Yatsı MUST be current, the İmsak column MUST show tomorrow's İmsak time with state `next` and note `yarın`, and the other four MUST be past. Before İmsak, no column MUST be current, İmsak MUST be next and the other five later, all showing today's times. (02 §4.5)
- **FR-013**: Column notes MUST be: `vakit girdi` for the current column during the first 5 minutes of its period, then `şu an`; `<duration> sonra` for next and later columns (except `yarın` above); empty for past columns. (02 §4.5, §9.3)

**Kerahat**

- **FR-014**: The engine MUST report up to three kerahat intervals: from Güneş to Güneş + *a*, from Öğle − *b* to Öğle, and from Akşam − *c* to Akşam, with *a*, *b*, *c* from the settings (default 45 minutes each, limited to 0–90). A length of zero MUST omit that interval; the layer switched off MUST omit all three; each interval includes its start and excludes its end, and MUST be clipped to the day (start ≥ 00:00, end ≤ 24:00). (ENG-06, 03 §8)

**Text**

- **FR-015**: All user-visible text MUST come from one place and match `02-ui-spec.md` §9 byte for byte, including the U+2019 apostrophe:
  - period names and countdown labels (§9.2);
  - column names `İmsak`, `Güneş`, `Öğle`, `İkindi`, `Akşam`, `Yatsı`;
  - column notes (§9.3).
- **FR-016**: Durations MUST be formatted from whole minutes *m* = floor(seconds / 60): *m* < 1 → `1 dk’dan az`; *m* < 60 → `M dk`; otherwise `H sa M dk`, or `H sa` when M = 0. (02 §9.1)
- **FR-017**: Times of day MUST be formatted `HH:MM` and remaining times `HH:MM:SS`, zero-padded, 24-hour. (02 §9.1)
- **FR-018**: Gregorian dates MUST be formatted `D MMMM YYYY dddd` with Turkish month and weekday names, and Hijri dates `D MMMM YYYY` with the Turkish Hijri month names of `02-ui-spec.md` §9.1. The weekday MUST be derived from the date.

**Behaviour and reuse**

- **FR-019**: The engine MUST be a pure computation: it MUST NOT read a clock, a time zone, storage or the network; the same inputs MUST always give the same result. (03 §3, constitution VI)
- **FR-020**: The result MUST carry every value the main screen needs for its hero, columns and day strip (`02-ui-spec.md` §4 and §5), so that Slice 3 can draw it without further time logic.
- **FR-021**: The shared fixture of `03-functional-spec.md` §3 MUST be defined once and be reusable by later slices and the screen-rendering harness.
- **FR-022**: A development-machine command MUST print the engine's output for the fixture at the moments listed in the slice prompt (03:30:00, 06:15:59, 06:16:00, 06:50:00, 10:00:00, 12:54:00, 12:55:00, 14:32:07, 16:20:00, 17:42:00, 17:51:00, 18:20:00, 19:14:00, 21:30:00, 23:59:59), as a table of period name, bounds, countdown label, remaining, soon, entered and the state and note of each column, for comparison with the reference renders.
- **FR-023**: The engine MUST be usable unchanged on the device: evaluating it once per second MUST NOT grow memory use.

### Key Entities

- **Day record**: one calendar day's six vakit times (minute precision), the date, an optional Hijri date and the source it came from. Valid or treated as missing.
- **Moment**: the current local date and time to the second. The engine receives it; it never reads a clock.
- **Settings for the engine**: soon threshold (minutes), kerahat layer on/off and the three kerahat lengths.
- **Engine result**: either "no data", or the running period (index, start, end, next vakit, tomorrow flag, remaining, elapsed, fraction, soon, entered, approximate bounds), six column entries (time, state, note, seconds until) and up to three kerahat intervals.
- **Shared fixture**: the yesterday/today/tomorrow records of `03-functional-spec.md` §3 with the expected values at 14:32:07.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Every acceptance scenario above, and each boundary listed in ENG-07 (one second before and at each of the six vakits; 23:59:59 → 00:00:00; missing yesterday; missing tomorrow; all three records identical; a record that changes by ±2 minutes at midnight; the last day of a month and of a year), is covered by an automated test that passes on the development machine.
- **SC-002**: For the fixture at 14:32:07, every expected value in `03-functional-spec.md` §3 is reproduced exactly.
- **SC-003**: A sweep over every second of a full day (86 400 moments, including the midnight on either side) finds no moment where: start ≤ now < end fails; remaining + elapsed differs from the period length; the period or a column state changes at any second other than a vakit or midnight; or the columns do not have exactly one `next` and at most one `current`.
- **SC-004**: Every user-visible string the engine produces is identical, byte for byte, to the text in `02-ui-spec.md` §9.
- **SC-005**: The printed table for the 15 listed moments matches the reference renders in `docs/design/screens/` for the moments they show (period name, bounds, countdown, column states and notes), as checked by the implementer before the slice is closed.
- **SC-006**: The engine builds for the device as well as for the development machine, with no change to its code.
- **SC-007**: Evaluating the engine once per second for a simulated day leaves memory use unchanged.

## Assumptions

- Answers suggested in the slice prompt are taken as decisions: a vakit begins at second 00 of its minute; at the exact boundary the new period is current; the remaining time shown at the boundary is the full length of the new period; durations round down to whole minutes.
- The engine receives local civil date-times. Time zones, daylight saving and clock validity (the `no_time` state) are outside this slice (Slice 5); Türkiye has no daylight saving.
- The Hijri date is taken from the day record when present. Computing a Hijri date is out of scope (it belongs with the calculated-data fallback in a later slice); without one, the Hijri line is empty.
- System states other than "no data" (`no_time`, `calculated`, `cache_ending`, `offline`, `ok`) and their strings belong to the screen (Slice 3) and the data slices, not to the engine.
- "Soon" uses the threshold as given; the settings page (Slice 8) limits it to 0, 5, 10, 15 or 30.
- Where records come from, how they are stored, and drawing are out of scope.
- No device, owner or network is needed to finish this slice; the owner's role is to accept the result.
