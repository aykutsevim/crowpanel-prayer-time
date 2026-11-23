# Slice 10: Optional extras

Two independent features. Build either, both or neither, and only on the owner’s request. Each is its own Spec Kit feature and follows the usual loop (`/speckit.specify`, `/speckit.clarify`, `/speckit.plan`, `/speckit.tasks`, `/speckit.analyze`, `/speckit.implement`, owner checklist, `/speckit.converge`).

---

## 10a: Diyanet’s official service

Prerequisite: the owner has applied for and received credentials for the Awqat Salah service (`https://awqatsalah.diyanet.gov.tr/`) and has entered them in the settings page.

### Specify

```text
/speckit.specify Use Diyanet's official prayer-time service on the Namaz Vakti Panel when the owner has credentials.

Read first: docs/specification/04-data-sources.md §2, §3.3, §4, §5 and §13 item 5; docs/specification/03-functional-spec.md §6 and the settings table in §8.

Why: the community mirrors republish Diyanet's tables and have served well, but the authority's own service is the most direct source. An owner who has obtained credentials should get their times from it first, with the mirrors remaining as automatic fallbacks. The service has small usage quotas, so the device must use it sparingly.

User stories:

1. Official first. As an owner with credentials, my device takes its times from Diyanet's service first, and the status panel names it as the source. If it is unavailable, refuses my credentials or my quota is used up, the mirrors take over as before and I see no disruption.

2. Sparing. As an account holder, my device uses only a small fraction of my quota: at most one sign-in and one timetable request per weekly refresh, never polling, never retrying a refused sign-in more than once a day.

3. Clear about credentials. As the owner, the settings page tells me whether the last sign-in worked, failed because of the credentials, or failed for another reason, and how much quota remains if the service reports it. My password is never shown or logged.

4. Consistent. As the owner, I can see whether the official times and the mirror times for my district agree; any difference for today is shown in the settings page.

Requirements covered: DATA-01, DATA-04, SEC-01, SEC-04, and 04-data-sources.md §3.3.

Edge cases: credentials entered but wrong; account locked; quota exhausted mid-month; the service's place identifier for my district differs from the mirrors'; token expiry between sign-in and fetch; the service returning fewer days than expected.

Out of scope: applying for an account, any use of the service beyond prayer times for the configured district.
```

### Plan

```text
/speckit.plan Plan the official-service provider.

- Read the live API reference first and record in research.md the exact sign-in request and response, token lifetimes, the monthly timetable response, the quota endpoint's response, error bodies for bad credentials and exhausted quota, and the stated quota rules. Then make one sign-in and one monthly request with my credentials, record the responses as fixtures with secrets removed, and compare the month with the mirrors' data for my district. Report differences before going further. Correct docs/specification/04-data-sources.md §3.3 where the live service differs.
- Confirm how the service identifies my district and map it to the stored district id; do not assume the ids are the same.
- Add the provider as rank 50 in the planner from Slice 7 as the first step of a refresh when credentials exist, with its own stricter call budget and cool-downs, covered by planner scenario tests: success, bad credentials, quota exhausted, service down, partial data.
- Tokens are kept in memory only. Credentials stay in non-volatile storage and never appear in logs, the status interface or the settings page.
- Settings page: sign-in status and remaining quota; overlay source name per 02-ui-spec.md §9.5.
```

### Owner checklist

- [ ] With credentials saved, the status panel shows `Diyanet (resmî servis)` after the next refresh.
- [ ] Times equal Diyanet’s website.
- [ ] With a deliberately wrong password, the settings page reports a credentials problem and the device carries on with the mirrors.
- [ ] The quota shown after a week of use has barely moved.
- [ ] No sound.

---

## 10b: Battery-backed clock

Prerequisite: either the board is the Advance model, which has a real-time clock with coin-cell backup built in (fit the coin cell), or the owner has connected a DS3231 module with its coin cell to the basic board’s I²C connector. Tell Claude Code which case applies.

### Specify

```text
/speckit.specify Keep time through power cuts on the Namaz Vakti Panel with a battery-backed clock (built into the Advance board, or an optional module on the basic board).

Read first: docs/specification/03-functional-spec.md §2 (TIME-03, TIME-08); docs/specification/05-architecture.md §1.

Why: the board forgets the time when it loses power, so after a power cut without Wi-Fi the device can only show the "waiting for time" state even though it has months of prayer times stored. A small battery-backed clock module removes that gap for owners who want it. Devices without the module must behave exactly as before.

User stories:

1. Works after a power cut without Wi-Fi. As an owner with the module fitted, when power returns and Wi-Fi does not, the main screen appears with the correct time and the stored prayer times within seconds, and the status line says the device is offline.

2. Stays accurate. As the owner, whenever the device synchronises with network time it corrects the module, so the module never drifts far.

3. Invisible when absent. As an owner without the module, nothing changes: no error, no delay at start-up, no extra log noise.

4. Sanity. As the owner, a module with a dead battery or a nonsensical time is ignored rather than trusted.

Requirements covered: TIME-08, TIME-03.

Edge cases: module present but never set; module time earlier than the firmware build time; module time ahead of network time by more than a day; the module sharing the bus with the touch controller.

Out of scope: any other clock chip, alarms or outputs of the module (it must not drive any sounder).
```

### Plan

```text
/speckit.plan Plan the optional clock module.

- Identify the clock chip: on the Advance board from the vendor schematic for my revision, on the basic board the DS3231 module. Research its register interface and the I²C addresses already in use on this board (touch controller, and the I/O expander on revisions that have one) to confirm there is no conflict; confirm bus speed and pull-ups from the vendor schematic.
- Probe once at start-up with a short timeout. Add a third clock source behind the clock interface from Slice 5, with the validity rule extended per TIME-08: module time is accepted only if later than the build time; network time always overrides and corrects the module.
- Host tests for the validity rule and the correction logic with a fake module. A development serial command reports module presence and time.
- The module's square-wave and alarm outputs are left disabled.
```

### Owner checklist

- [ ] With the module fitted and the router off, a power cycle brings back the main screen with the right time.
- [ ] With the module removed, the device behaves as before and shows no error.
- [ ] No sound.
