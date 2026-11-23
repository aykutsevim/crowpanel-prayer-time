# Slice 3: Main screen with sample data

Goal: the approved design on the real panel, faithful to the reference, showing every time-of-day and system state from sample data and a simulated clock. See `docs/specification/06-roadmap.md`.

Needs Slice 1 (display stack decided) and Slice 2 (engine).

## Step 1: Specify

**Paste into Claude Code**

```text
/speckit.specify Main screen of the Namaz Vakti Panel, shown with sample data and a simulated clock.

Read first: docs/specification/02-ui-spec.md completely (§1 to §6, §9, §11, §12 apply to this feature), and look at every image in docs/design/screens/ numbered 01 to 14. Open docs/design/tokens.json for the numbers. docs/design/reference.html is the interactive source of those renders.

Why: this screen is the product. It was designed and approved in advance, so the job is faithful reproduction, not design. Getting it right before networking exists means later features only have to supply data.

User stories, in priority order:

1. Glance. As someone in the room, I can read the current time, the name and bounds of the current prayer period, and the time remaining until the next one from three metres away. Acceptance: the main screen matches reference 01 for the shared sample day at 14:32:07 within the tolerances in 02-ui-spec.md §12.

2. The day at a glance. As someone in the room, I see the whole day as one strip with a marker at the present moment, and six columns with each time connected to its place on the strip. Acceptance: strip segments, separators, hatch, hour scale, marker and leader lines are at the positions the spec's formulas give for the sample day.

3. The day moves. As the owner, when time passes, the screen follows: the countdown and clock every second, the marker and notes every minute, and the column states, highlighted column and hero texts at each prayer time, including the cases before İmsak and after Yatsı and the change of day at midnight. Acceptance: references 02 to 09 are each reproduced at their stated times; the countdown turns to the alert colour at 15 minutes; the note reads "vakit girdi" for the first five minutes of a period.

4. Honest states. As the owner, when the device has no valid time, no data, calculated data, an offline condition or a nearly exhausted store, the screen shows the defined state. In this feature these states are forced by a developer command, since nothing real produces them yet. Acceptance: references 10 to 14 are reproduced.

5. Developer control. As the developer, I can drive the device from the serial port: set the simulated time, set how fast it runs, force a system state, and jump to any of the fourteen reference fixtures by number. Acceptance: each fixture can be shown on the panel with one command.

6. Self-checking. As the developer, I can render each fixture to an image file on the development machine without the device, so that the screen can be compared with the reference before the owner is asked to look.

Requirements covered: UI-01 to UI-05, SYS-06, and 02-ui-spec.md §1 to §6, §9, §11, §12.

Edge cases: the marker at the far left and far right of the strip; a prayer time whose leader line is nearly vertical; the longest strings in each text slot (for example "Çevrimdışı, kayıtlı vakitler kullanılıyor", "11 sa 56 dk sonra", "Cemaziyelevvel"); a city name too long for its slot is truncated with an ellipsis before the status text.

Out of scope: real time, real data, Wi-Fi, touch interaction, the setup, connecting and overlay screens, the boot screen beyond what Slice 1 delivered.
```

## Step 2: Clarify

```text
/speckit.clarify Focus on anything in 02-ui-spec.md that is ambiguous for implementation, and on how fidelity is judged.
```

Suggested answers if asked: the reference PNG wins over prose; text baselines within 2 px and non-text geometry within 1 px are acceptable; glyph shapes may differ from the browser render; no animation.

## Step 3: Plan

```text
/speckit.plan Plan the main screen.

Constraints:
- Structure per docs/specification/05-architecture.md §4: screen code in lib/ui depending only on LVGL and lib/core; it takes the engine's output struct plus a small system-state struct as its view model. No board, Wi-Fi or storage code in lib/ui.
- Fonts: convert Barlow and Barlow Condensed (SIL OFL) to the sizes, weights and glyph sets in 02-ui-spec.md §2.2. Research the current font conversion tool for the LVGL version chosen in Slice 1 and pin it. Keep the font sources' licence file, the converted fonts and a regeneration script in assets/fonts. Report the flash cost per font and in total against the 300 kB budget; propose consolidation only from the allowed list in the spec.
- Numbers that tick must not move: lay out clock and countdown with one fixed-width cell per character as specified (digit cell 0.475 em, colon cell 0.252 em).
- Drawing: the strip, hatch, separators, marker and leader lines are static geometry for a given day; decide between LVGL objects, a canvas, or custom draw events, and justify the choice by memory and redraw cost. Per-second updates must touch only the clock, countdown and progress fill; per-minute the marker and notes (UI-03).
- Host UI harness per 05-architecture.md §7: a native build of lib/ui + lib/core + LVGL with a headless display that writes PNG files, and a command that renders fixtures 01 to 14 into out/snapshots/. The fixture definitions (time, system state, expected file name) live in one table shared by the harness and the device's serial commands.
- Review loop: after rendering, open each snapshot beside its reference in docs/design/screens/ and compare them yourself; fix differences; only then prepare the owner checklist. Add a script that reports the pixel bounding boxes of the strip, the current column and the marker in a snapshot so geometry can be asserted in tests.
- Device: a simulated clock module in src/app that the serial commands control (time, speed multiplier, system state, fixture number). The real clock arrives in Slice 5 behind the same interface.
- Performance target: a full-screen redraw at a period change completes without visible tearing; steady-state CPU use leaves the UI task idle most of each second. State how you will measure both.
- Stay within the firmware size budget and report the size.
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
/speckit.implement Stage 1: fonts and the host UI harness with an empty screen, then the static layout of the main screen for fixture 01. Render the snapshot, compare it with docs/design/screens/01-main-ogle-1432.png yourself, and iterate until it is within tolerance. Show me both images' paths and a list of remaining differences.
```

```text
/speckit.implement Stage 2: time-driven behaviour and fixtures 02 to 09. Render all, compare each with its reference, fix, and report.
```

```text
/speckit.implement Stage 3: system states and fixtures 10 to 14, then the device integration: simulated clock and serial commands. Give me the owner checklist.
```

## Step 7: Owner checklist

Flash the device. Use the serial commands Claude Code lists.

- [ ] Fixture 01 on the panel looks like `01-main-ogle-1432.png`: same layout, colours and text.
- [ ] From three metres the clock, countdown and six times are readable.
- [ ] Colours of the six strip segments are distinguishable on this panel, also when viewed from the side and from below.
- [ ] Running the simulated clock at high speed through a full day shows no flicker, no leftover pixels and no jump in the position of the ticking digits.
- [ ] Fixtures 02 to 09 each match their reference.
- [ ] Fixtures 10 to 14 each match their reference.
- [ ] No sound.

Send photographs of fixtures 01, 09 and 12 to Claude Code if anything looks off, with a sentence on what differs.

## Step 8: Converge and merge

```text
/speckit.converge
```

## Done when

Snapshots 01 to 14 are within tolerance and reviewed, the owner checklist is complete, and `check` passes with the size reported.

Next: `slice-04-wifi-setup.md`.
