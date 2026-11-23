# Slice 5: Real time

Goal: the clock on screen is the real local time, and when the device cannot know the time it says so. See `docs/specification/06-roadmap.md`.

## Step 1: Specify

**Paste into Claude Code**

```text
/speckit.specify Real time for the Namaz Vakti Panel.

Read first: docs/specification/03-functional-spec.md §2; docs/specification/02-ui-spec.md §6 (state no_time) and reference render 13.

Why: every value on the screen depends on the current local time. The board has no battery-backed clock, so after each power-on the device knows nothing until it has reached a time source. It must get the time quickly and reliably once Wi-Fi is up, keep it accurate, and be honest in the meantime.

User stories, in priority order:

1. Correct clock. As someone in the room, the time and date on the screen are right to the second. Acceptance: within ten seconds of Wi-Fi coming up the screen shows the real local time; compared with a reference clock it is never more than one second off over 24 hours.

2. Honest waiting. As the owner, from power-on until the device has obtained the time, the screen shows the defined "no time" state instead of a wrong clock: dashes for the clock and countdown, the texts from the UI spec, no position marker. If stored prayer times exist they are still listed. Acceptance: reference 13.

3. Stubborn about getting the time. As the owner, if the usual time servers cannot be reached on my network, the device still gets the time by another route and keeps trying the normal one. Acceptance: with network time blocked at my router, the clock becomes valid within about a minute and is within two seconds of real time.

4. Midnight. As someone in the room, at midnight the date changes and the screen carries on without a glitch.

5. Time zone. As the owner in Türkiye I never have to set a time zone. An advanced setting exists for another zone and takes effect at once. (The setting is stored now; its input field arrives with the settings page.)

Until real prayer data arrives in the next feature, the screen keeps using the sample day from the previous feature, now driven by the real clock.

Requirements covered: TIME-01 to TIME-07, and system state no_time.

Edge cases: time servers reachable but answering late; the clock stepping backwards or forwards by minutes at a resync; Wi-Fi lost for days after a successful sync (the clock keeps running and stays valid); a time earlier than the firmware's build time is never accepted; the simulated clock from the developer commands still overrides real time when switched on.

Out of scope: a battery-backed clock module, automatic time-zone detection, daylight-saving rules beyond what a POSIX time-zone string expresses.
```

## Step 2: Clarify

```text
/speckit.clarify Focus on when time counts as valid and what the screen does when a resync moves the clock.
```

Suggested answers if asked: valid only after a successful sync since power-on and later than the build time; a resync is applied immediately and the countdown may jump; the fallback route is used after 60 s without success.

## Step 3: Plan

```text
/speckit.plan Plan real time.

Constraints and research:
- Research first, with sources: the current time-synchronisation API of the framework chosen in Slice 1 (multiple servers, sync notification callback, resync interval, behaviour when Wi-Fi drops); how to set the POSIX time zone; the resolution and drift of the system clock between syncs on this chip.
- A clock interface in lib/core or src/app with three implementations: real (system time + validity rule), simulated (developer commands, from Slice 3), and a test double. Everything that needs "now" asks this interface; nothing reads the system clock directly.
- Validity rule per TIME-03, unit-tested with the test double.
- Fallback per TIME-02: an HTTP request to one of the hosts already listed in 04-data-sources.md §11, reading the Date response header. Plain HTTP is acceptable for this purpose only; say why TLS cannot be used before the clock is valid and how the result is replaced by network time as soon as that succeeds.
- Second-boundary alignment: the UI update is scheduled so the displayed second changes within 100 ms of the real second (TIME-07). Describe the mechanism and how you will measure it using the serial log.
- Midnight handling goes through the engine; add a device-level test that runs the simulated clock across midnight at real speed and at 60x.
- Time zone stored in settings with the default from the spec.
- Logging per SYS-05: one line per sync with source and correction applied.
- System-state selection: introduce the real evaluation order from 02-ui-spec.md §6 for the states this feature can produce (no_time, ok); the others stay forced-only until their features arrive.
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

## Step 7: Owner checklist

- [ ] Power on with Wi-Fi available: the “waiting for time” screen appears briefly and matches `13-state-time-not-set.png`, then the real time and date appear.
- [ ] The clock agrees with a phone showing seconds.
- [ ] The date line shows today’s date and weekday in Turkish.
- [ ] With time servers blocked on the router (or the device on a guest network that blocks them), the clock still becomes valid. Skip if you cannot block them; Claude Code will say how it tested this instead.
- [ ] Power on with the router off: the screen stays in the “waiting for time” state.
- [ ] Left on overnight, the date changed at midnight and the clock is still right in the morning.
- [ ] No sound.

## Step 8: Converge and merge

```text
/speckit.converge
```

## Done when

The checklist is complete and `check` passes.

Next: `slice-06-prayer-data.md`.
