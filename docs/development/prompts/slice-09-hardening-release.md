# Slice 9: Hardening and release

Goal: a version 1.0.0 that can be left running for months. This slice adds little that is visible; it measures, proves and documents. See `docs/specification/06-roadmap.md`.

## Step 1: Specify

**Paste into Claude Code**

```text
/speckit.specify Hardening and first release of the Namaz Vakti Panel.

Read first: docs/specification/03-functional-spec.md §10, §11 and §12; docs/specification/01-product-brief.md ("What success looks like"); docs/specification/05-architecture.md §5, §6 and §8.

Why: this device will sit on a shelf, powered for years, with nobody watching it. It has to recover from anything by itself, never wear out its storage, never slowly run out of memory, and never show a wrong time. The owner needs evidence of that before trusting it, and a short guide in Turkish for whoever sets it up next.

User stories, in priority order:

1. Recovers by itself. As the owner, if the device ever hangs, it restarts within seconds and returns to the main screen showing correct times, with no action from me. Acceptance: a developer command that deliberately stalls the screen loop, and another that stalls the network task, each lead to an automatic restart and normal operation.

2. Power cuts do no harm. As the owner, I can cut the power at any moment, any number of times, and the device never loses its settings or stored times and never needs to be set up again. Acceptance: a scripted test on the development machine covers interrupted writes of settings and of the store; on the device, the owner pulls the plug twenty times at random moments, including during a refresh, without ill effect.

3. Runs for a long time. As the owner, I leave the device on and it is still correct weeks later. Acceptance: a 24-hour unattended run produces a log that shows no restart, no growth in memory use beyond the allowed margin, on-time refresh behaviour and a clock within one second of real time at the end.

4. Measured, not assumed. As the owner, I get a one-page report with the measured values for every target in the specification: start-up time to the main screen, time to first real data after setup, firmware size against the budget, requests per day, flash writes per day, clock error, peak and steady memory use.

5. Diagnosable. As a developer, I can read restart count and reason, uptime, memory low-water marks, last refresh results per source and planner state from the status interface and the serial port, with no secrets in any of it.

6. Ready to hand over. As the next person to set this device up, I find a short Turkish guide in the repository: what the device does, how to set it up from a phone, what each status message means, how to change settings, how to start over. As a developer, I find how to build, flash and test in English.

7. Released. The repository has a tagged version 1.0.0 whose firmware reports that version on the status panel.

Requirements covered: SYS-01 to SYS-05, the targets in 03-functional-spec.md §12, SEC-04, SEC-05, and the constitution's principles on resource discipline and silence, re-verified on the release build.

Edge cases: restart loops (a fault that recurs immediately after start must not wear out flash or flicker the screen forever; after repeated early restarts the device starts in a minimal safe mode that shows the main screen from the store without networking, and says so in the status panel); brown-outs on weak USB supplies; storage full; the place list or pages missing or damaged in the image.

Out of scope: over-the-air update, new features, the optional extras of Slice 10.
```

## Step 2: Clarify

```text
/speckit.clarify Focus on the safe-mode rule after repeated restarts and on what the 24-hour run must log.
```

Suggested answers if asked: three restarts within five minutes of start-up trigger safe mode until the next clean hour of operation or a power cycle; the soak log records, every ten minutes, uptime, free memory and low-water marks, Wi-Fi state, clock correction at each sync and planner events.

## Step 3: Plan

```text
/speckit.plan Plan hardening and release.

Constraints and research:
- Research the chosen framework's watchdog facilities (task watchdog, interrupt watchdog, panic behaviour), reset-reason reporting, brown-out detector settings and heap diagnostics, with current documentation, and choose settings that meet SYS-01 without false triggers during TLS handshakes or flash writes.
- Restart-loop protection with a small counter in non-volatile storage, written at most a few times per incident; design it so it cannot itself wear flash.
- Audit every flash write path against SYS-03 and report writes per day in steady state.
- Audit every heap allocation in steady-state paths; add a development-only memory report command. Run a one-hour accelerated soak on the device with the simulated clock at high speed crossing several days, then the real 24-hour soak.
- Soak tooling: a script in tools/ that captures the serial log to a file with timestamps and a second script that summarises it into the report for story 4. The owner starts it and leaves the laptop connected.
- Measure each target in 03-functional-spec.md §12 and write the report into the feature directory; any miss becomes a task, not a footnote.
- Re-run the silence check on the release configuration and repeat the owner's listening test once on the final build.
- Review logs and the status interface for secrets (SEC-04).
- Documentation: docs/KULLANIM.md in Turkish for owners, covering setup, the meaning of each status line from 02-ui-spec.md §6, the status panel, the settings page and starting over; README.md in English for developers. Keep both short.
- Versioning: one version constant used by the boot screen, status panel, User-Agent and tag. Release build configuration with development serial commands that can alter data or simulate time compiled out, and read-only diagnostics kept.
- Update the documents in `docs/specification/` with anything that changed during the project so that the documents describe the product as built.
```

## Steps 4 to 6

```text
/speckit.tasks
```

```text
/speckit.analyze
```

```text
/speckit.implement Stage 1: watchdogs, restart-loop protection, diagnostics, the write and memory audits with fixes, and the soak tooling. Then tell me how to start the 24-hour run.
```

After the soak:

```text
/speckit.implement Stage 2: summarise the soak log I am attaching, measure the remaining targets, write the report, fix any miss, then the documentation and the release build. Give me the final checklist.
```

## Step 7: Owner checklist

- [ ] Both deliberate stall commands lead to an automatic restart and the main screen.
- [ ] Twenty power cuts at random moments: the device always came back to the main screen with the right times and never asked for setup.
- [ ] The 24-hour report shows no restart and flat memory.
- [ ] Every target in the report is met.
- [ ] `docs/KULLANIM.md` reads correctly and matches what the device does.
- [ ] The status panel shows version 1.0.0.
- [ ] Final listening test on the release build: no sound at boot, during refresh, during dimming.

## Step 8: Converge, merge, tag

```text
/speckit.converge
```

```text
Merge the feature branch, tag v1.0.0, and give me the path of the release firmware image and the command to flash it.
```

## Done when

The checklist is complete, the report is committed, and the tag exists.

Optional next: `slice-10-optional-extras.md`.
