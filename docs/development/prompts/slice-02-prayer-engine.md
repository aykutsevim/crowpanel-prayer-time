# Slice 2: Prayer-time engine

Goal: the time logic of the product, correct and proven on the development machine, with no hardware involved. See `docs/specification/06-roadmap.md`.

This slice needs no board. It can run while Slice 1 waits for the owner.

## Step 1: Specify

**Paste into Claude Code**

```text
/speckit.specify Prayer-time engine for the Namaz Vakti Panel.

Read first: docs/specification/01-product-brief.md (glossary), docs/specification/03-functional-spec.md §3, docs/specification/02-ui-spec.md §4.5, §5 and §9.

Why: the whole screen is derived from one question: given the current local time and the published times for yesterday, today and tomorrow, which prayer period are we in, when did it start, when does it end, what comes next and how long until then. If this is wrong by a minute at a boundary, the product is wrong. It must be decided once, in one place, and tested exhaustively without a device.

What it must do:

1. Day records. Hold one day's six times (İmsak, Güneş, Öğle, İkindi, Akşam, Yatsı) at minute precision with its date, an optional Hijri date and a source. Reject a record whose times are not strictly increasing within the day.

2. Current period. For any moment, report the running period, its start and end as absolute local times, the next time and its name, seconds remaining, seconds elapsed and the elapsed fraction. Before today's İmsak the running period is the Yatsı period that began yesterday; after today's Yatsı it runs until tomorrow's İmsak.

3. Missing neighbours. When yesterday's or tomorrow's record is absent, substitute today's Yatsı or İmsak time respectively and flag the result as approximate. When today's record is absent, report "no data".

4. Column states. For each of the six times shown on screen, report the time to display and whether it is past, current, next or later, with the special cases after Yatsı (the İmsak column shows tomorrow's time and is "next", labelled tomorrow) and before İmsak (no column is current).

5. Flags. "Soon" when the remaining time is at or below a configurable threshold (default 15 minutes). "Entered" during the first five minutes of a period.

6. Kerahat intervals. Three intervals of configurable length after sunrise, before Öğle and before Akşam, each of which can be switched off with a length of zero.

7. Turkish text. Produce exactly the strings the screen shows: period names, the countdown label for each next time, durations in the form defined in 02-ui-spec.md §9.1, column notes, Gregorian dates with Turkish month and weekday names, and Hijri dates with Turkish month names.

Acceptance scenarios (all must be automated):
- With the fixture in 03-functional-spec.md §3 at 14:32:07, every expected value listed there is produced.
- One second before and exactly at each of the six times, the period, next time and column states switch at the right second.
- 23:59:59 to 00:00:00 with distinct records for both days: the Yatsı period continues without interruption and the six displayed times change to the new day's.
- 03:30 before İmsak: period is Yatsı, bounds are yesterday's Yatsı and today's İmsak, no column is current, İmsak is next.
- 21:30 after Yatsı: Yatsı is current, the İmsak column shows tomorrow's time marked as tomorrow.
- Missing yesterday, missing tomorrow, and missing today each behave as described in point 3.
- Month end and year end dates format correctly, including 31 Aralık to 1 Ocak.
- Durations: 0 to 59 seconds, exactly one minute, 59 minutes, exactly one hour, one hour one minute, more than ten hours.
- A record with equal or decreasing times is rejected.

Requirements covered: ENG-01 to ENG-08, text rules in 02-ui-spec.md §9.

Out of scope: where records come from, storage, clocks, time zones, networking, drawing. The engine receives local date-times and returns plain data.
```

## Step 2: Clarify

```text
/speckit.clarify Focus on boundary behaviour at second precision and on the exact text outputs.
```

Suggested answers if asked: a time begins at second 00 of its minute; at the exact boundary the new period is current; remaining time shown at the boundary is the full length of the new period; durations round down to whole minutes.

## Step 3: Plan

```text
/speckit.plan Plan the prayer-time engine.

Constraints:
- It lives in lib/core as described in docs/specification/05-architecture.md §4: standard C++ only, no Arduino, ESP-IDF or LVGL headers, no heap allocation in the functions called every second, no global mutable state. It must compile for the host test environment and for the ESP32-S3.
- Inputs are local civil date-times as plain fields (year, month, day, hour, minute, second) or a day number plus second-of-day; the engine never reads a clock or a time zone. Choose one representation and justify it for midnight and year-end arithmetic.
- Output is one plain struct that later becomes the UI's view model. Design it now with the fields the screen needs (02-ui-spec.md §4 to §6) so Slice 3 can consume it unchanged.
- All user-visible strings come from one table in lib/core with the exact Turkish text and the U+2019 apostrophe. Source files are UTF-8.
- Test-first: write the tests for every acceptance scenario before the implementation, using the host test framework set up in Slice 0. Put the shared fixture in one place so later slices and the UI harness reuse it.
- No third-party dependencies in this slice.
```

## Steps 4 to 6

```text
/speckit.tasks
```

```text
/speckit.analyze
```

```text
/speckit.implement
```

## Step 7: Check

No device is involved. Ask Claude Code:

```text
Run the full check command. Then show me a table of the engine's output for the shared fixture at these times: 03:30:00, 06:15:59, 06:16:00, 06:50:00, 10:00:00, 12:54:00, 12:55:00, 14:32:07, 16:20:00, 17:42:00, 17:51:00, 18:20:00, 19:14:00, 21:30:00, 23:59:59. Columns: period name, bounds, countdown label, remaining, soon, entered, and the state and note of each of the six columns.
```

Compare a few rows with the reference renders in `docs/design/screens/` (files 01 to 09 show seven of these moments).

## Step 8: Converge and merge

```text
/speckit.converge
```

## Done when

All scenarios pass in the host environment, the table matches the reference renders, and the code builds for the device environment as well.

Next: `slice-03-main-screen.md`.
