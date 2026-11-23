# Slice 6: Real prayer times

Goal: today’s Diyanet-published times for the chosen district, fetched once and kept on the device. See `docs/specification/06-roadmap.md`.

This slice introduces one provider and the store. Scheduling and fallbacks come in Slice 7; here a refresh happens after setup, at boot when needed, and on a developer command.

## Step 1: Specify

**Paste into Claude Code**

```text
/speckit.specify Real prayer times for the Namaz Vakti Panel: first data source and on-device store.

Read first: docs/specification/03-functional-spec.md §6 and §7; docs/specification/04-data-sources.md §1 to §4, §7, §8, §10, §13; docs/specification/02-ui-spec.md §6 (states ok, offline, no_data) and reference renders 01, 10 and 14.

Why: until now the screen has shown a sample day. The product's purpose is to show the times Diyanet publishes for the owner's district, and to keep showing them when the network is not there. That needs a trustworthy fetch and a store that survives restarts and power cuts.

User stories, in priority order:

1. Real times. As the owner, shortly after setup the six times on screen are the ones Diyanet publishes for my district today, and the Hijri date is the one published with them. Acceptance: for three different dates within the stored range, the device's times equal those on Diyanet's own website for the district.

2. Kept on the device. As the owner, the device stores the times for as far ahead as the source provides (normally to the end of the year) and shows them from that store. After a restart without Wi-Fi it shows the same times as soon as it knows the time of day. Acceptance: with the router off and the clock set by the developer command, today's stored times are shown and the state is "offline".

3. Never half-written. As the owner, a power cut at any moment, including during a refresh, never leaves the device with damaged or mixed data: after restart it has either the previous data or the new data. A damaged store is detected and rebuilt from the network. Acceptance: demonstrated by a test that interrupts the write at every step, and on the device by corrupting the stored file through a developer command.

4. Nothing wrong gets in. As the owner, data that is malformed, for another place, for the wrong dates or internally inconsistent is rejected as a whole and the previous data stays. Acceptance: each rejection reason is covered by an automated test using altered copies of real responses.

5. Honest when empty. As the owner, if there is no stored record for today, the screen shows the "no data" state from the UI spec, and the status panel says what the device knows. When data arrives the screen switches to normal by itself.

6. Visible provenance. As the owner, the status panel shows the source of today's times, when they were last updated, and the last date stored.

Requirements covered: DATA-01, DATA-02, DATA-06, DATA-07, DATA-08 (store hand-over on location change is exercised through start-over setup), SEC-01 to SEC-03, LOC-01. Provider: the first Diyanet-derived mirror in 04-data-sources.md.

Edge cases: the source is unreachable at first setup; the response is very large; the response ends in the middle; the source has no data past 31 December; the district has been changed through start-over setup; today's record exists but tomorrow's does not; the device's clock is valid but wrong by a day.

Out of scope: scheduled refreshes, other sources, calculated times, the "calculated" and "cache ending" states, coordinates, the official Diyanet API, the settings page.
```

## Step 2: Clarify

```text
/speckit.clarify Focus on what counts as a successful refresh in this feature and on the exact conditions for the ok, offline and no_data states.
```

Suggested answers if asked: success means today’s record and everything the provider offered through year end is stored; `offline` after Wi-Fi has been down for ten minutes; `no_data` only when today has no record; a failed fetch with a store that covers today shows nothing.

## Step 3: Plan

```text
/speckit.plan Plan real prayer times: first provider and store.

Constraints and research:
- Do the live verification in docs/specification/04-data-sources.md §13 items 1, 3 and 6 now, for my configured district, and write the results into research.md: fetch the provider's API description and check its version, call its version 2 times endpoint for a date range and its version 1 range endpoint, record the real responses as test fixtures under test/fixtures (trim only by cutting whole records), decide per 04-data-sources.md §3.1 which is primary and which is the fallback, and diff 30 days against the second mirror. If the mirrors disagree, stop and tell me before planning further. Correct 04-data-sources.md wherever the live service differs from it.
- Layering per 05-architecture.md §4: the parser in lib/providers is a pure function from bytes to records with streaming, field-filtered parsing (records repeat a large nested object; never hold the whole body). The HTTPS client in src/platform implements an abstract fetch interface. Both are testable separately; parser tests run on the host against the recorded fixtures and against truncated, reordered and corrupted variants.
- HTTPS with certificate verification against a bundled root store, client rules per 04-data-sources.md §10. Research how the chosen framework bundles root certificates and what it costs in flash; never disable verification. Requests run only when the clock is valid.
- Validation per 04-data-sources.md §7 except the plausibility comparison with calculated times, which arrives in Slice 7.
- Store per 04-data-sources.md §8 on LittleFS: define the on-disk format with a version and checksum, and the replace procedure that satisfies DATA-07. Put the format and the replace logic in lib/core behind a small file-system interface so power-loss behaviour is tested on the host by failing the fake file system at every write and rename.
- Merge rules per 04-data-sources.md §4, implemented in lib/core with tests, even though only one source exists yet.
- Refresh in this slice: one function "refresh now" run by the app task after setup, at boot if today is not covered or the store is empty, and by a developer serial command. One request at a time, never on the UI task. Simple retry: once after one minute, then leave it to the developer command; the scheduler replaces this in Slice 7.
- The engine now takes yesterday, today and tomorrow from the store. The Hijri date shown comes from the record.
- System states: add real evaluation of ok, offline and no_data per 02-ui-spec.md §6. The status text "Son güncelleme" uses the last successful refresh.
- Status overlay: fill source, last update and last stored date.
- Developer serial commands: refresh now, dump store summary (dates, count, source, checksum state), corrupt store (development builds only), erase store.
- Memory: state the peak heap use of a fetch (TLS handshake plus parsing) and how it was measured.
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
/speckit.implement Stage 1, host only: record fixtures, parser, validation, merge rules, store format and atomic replace with the power-loss tests. Show me the test list and results.
```

```text
/speckit.implement Stage 2, device: HTTPS client, refresh function, engine fed from the store, system states, overlay fields, developer commands. Then give me the owner checklist together with the three dates I should compare on Diyanet's website and the values the device holds for them.
```

## Step 7: Owner checklist

- [ ] After setup, real times appear within about twenty seconds.
- [ ] Today’s six times equal Diyanet’s website for my district (`https://namazvakitleri.diyanet.gov.tr/`).
- [ ] The two other dates Claude Code listed also match.
- [ ] The Hijri date on screen equals the one on Diyanet’s site.
- [ ] Status panel shows the source, the update time and the last stored date.
- [ ] Router off, device restarted, clock set by the developer command: same times, status says offline.
- [ ] After the “corrupt store” command and a restart with Wi-Fi on, the device recovers by itself.
- [ ] After erasing the store with Wi-Fi off, the screen matches `14-state-no-data.png`.
- [ ] No sound.

## Step 8: Converge and merge

```text
/speckit.converge
```

## Done when

The checklist is complete, `04-data-sources.md` reflects what the live services actually returned, and `check` passes.

Next: `slice-07-refresh-fallback.md`.
