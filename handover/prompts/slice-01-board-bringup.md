# Slice 1: Silent board bring-up

Goal: prove that this board shows a stable picture, reads touch, controls its backlight and stays completely silent, and record which driver stack the project uses. See `handover/docs/06-roadmap.md`.

Before step 1 the owner reads the board: model number on the back label (`DIS08070H` or `DIS02170A`) and the hardware revision printed on the PCB. Put both into the first prompt where indicated.

## Step 1: Specify

**Paste into Claude Code**

```text
/speckit.specify Silent board bring-up for the Namaz Vakti Panel.

Read first: handover/docs/01-product-brief.md (hard constraints), handover/docs/03-functional-spec.md §1 and §9, handover/docs/05-architecture.md §1 and §2.

My board: model <DIS08070H or DIS02170A>, hardware revision <as printed on the PCB>.

Why: every later feature depends on four hardware basics that nobody has verified on this unit yet: a stable picture, working touch, a controllable backlight, and total silence. The implementer cannot see or hear the device, so this feature must also give the owner the means to verify each point and report back.

User stories, in priority order:

1. Silence. As the owner, when I power the device, restart it, or leave it running while it uses Wi-Fi and storage, it never produces any sound from the speaker connector, a built-in speaker, or the backlight circuit. Acceptance: the audio amplifier is disabled before anything else starts; in a quiet room, with my ear close to the device, I hear nothing during boot, during the stress mode, and while the backlight level changes.

2. Stable picture. As the owner, I see a boot screen and then a diagnostic screen that lets me judge the display: full-screen colour bars in a known order, a one-pixel white border on all four edges, a field of alternating one-pixel vertical lines, and the text "800 x 480". Acceptance: colours match the on-screen labels, orientation is landscape with the USB connector where the label says, the border is visible on every edge, and the picture does not shift, tear or flicker.

3. Stability under load. As the owner, I can start a stress mode that keeps the diagnostic picture on screen while the device continuously scans for Wi-Fi networks, performs encrypted web requests and writes to its storage, for at least ten minutes. Acceptance: the picture stays as stable as in story 2; the screen shows a running count of completed cycles and any errors.

4. Touch. As the owner, when I touch the screen I see a marker under my finger and the coordinates. Acceptance: touching the four corner targets reports positions within 10 pixels of each target.

5. Backlight. As the owner, I can step the backlight through several levels from the diagnostic screen. Acceptance: either the brightness visibly changes at each step without any audible noise, or the feature reports that dimming is not usable on this hardware and leaves the backlight fully on.

6. Evidence. As the developer, I get a short report on screen and on the serial port: board model and revision detected or configured, free memory, and the result of each self-test, so the owner can read it out.

Requirements covered: MODE-01, SIL-01 to SIL-06, and the risks in 05-architecture.md §2.

Edge cases: touch controller absent or not responding; display initialises but shows nothing (the serial report must still appear); stress mode with no Wi-Fi networks in range; backlight pin has no dimming effect.

Out of scope: any product screen, connecting to a real Wi-Fi network, storing settings, prayer times.

The feature ends with an owner checklist of yes/no questions covering stories 1 to 5; the answers are recorded in the feature directory and decide the backlight behaviour and the display driver choice for the rest of the project.
```

## Step 2: Clarify

**Paste into Claude Code**

```text
/speckit.clarify Focus on what the owner must observe and report for each story, and on what the feature does when a self-test fails.
```

Suggested answers if asked: the diagnostic screen stays until restarted; stress mode starts from an on-screen button and from a serial command; a failed self-test is shown in the alert colour and does not stop the others.

## Step 3: Plan

**Paste into Claude Code**

```text
/speckit.plan Plan the silent board bring-up.

Constraints and inputs:
- Follow handover/docs/05-architecture.md: §1 for board facts, §2 for the risks this slice must settle, §3 for the recommended stack and the alternative, §4 for where code goes (everything in this slice is in src/platform and src/app), §9 for silence.
- Research first, in research.md, citing sources: (a) the vendor's wiki page, repository examples and schematic for my exact model and revision: RGB pins and timing, touch wiring, the I/O expander sequence if the revision has one, the backlight pin and whether its driver accepts PWM, which signal enables or shuts down the audio amplifier, and on the Advance model also the buzzer, the DIP-switch positions that route audio, and anything an auxiliary controller on the board can drive; (b) the current documentation of each library you intend to use, with the versions you will pin; (c) known causes and fixes for picture instability on ESP32-S3 RGB panels with the chosen driver. Do not take pin numbers from memory. If the schematic leaves the amplifier control unclear, list the candidates and ask me to measure or inspect.
- Start from the vendor-verified panel configuration. State the alternative stack you would switch to and the evidence that would trigger the switch.
- Silence: one platform function that parks the audio pins and disables the amplifier, called first in startup. Backlight PWM at 20 kHz or higher, only on the backlight pin.
- LVGL is introduced in this slice only as far as needed to draw the boot and diagnostic screens; choose its buffer and flush strategy with the stress test in mind and justify it.
- The stress mode needs Wi-Fi scanning, an HTTPS request with certificate verification to any well-known host, and repeated small flash writes to a scratch file. It must not store credentials. If no network is joined, the HTTPS part is skipped and reported as skipped.
- Provide serial commands: report, stress on/off, backlight <percent>.
- Keep the firmware within the size budget and report the size.
- Write the owner checklist as a file in the feature directory with yes/no questions and a place for answers.
```

## Step 4 and 5: Tasks and analysis

**Paste into Claude Code**, one after the other:

```text
/speckit.tasks
```

```text
/speckit.analyze
```

Fix any finding at its source and rerun `/speckit.analyze` until it is clean.

## Step 6: Implement

**Paste into Claude Code**, one stage at a time, checking the result before the next:

```text
/speckit.implement Stage 1 only: the silence function and its call order, the board configuration for my model and revision, and the serial report. No display yet. Build, run the silence check, and show me the serial output I should expect so I can flash and compare.
```

```text
/speckit.implement Stage 2: display, boot screen and diagnostic screen, then touch, then backlight stepping. After each of the three, stop and give me the matching part of the owner checklist.
```

```text
/speckit.implement Stage 3: stress mode. Then give me the remaining checklist items.
```

## Step 7: Owner checklist

Flash, then answer in Claude Code. Expected items:

- [ ] No sound at power-on, at reset, or during ten minutes of stress mode (ear close to the device, quiet room). On the Advance model this includes the buzzer.
- [ ] Colour bars match their labels; picture is the right way up.
- [ ] One-pixel border visible on all four edges.
- [ ] Vertical-line field is steady: no crawling, shifting or flicker, idle and under stress.
- [ ] Touch marker lands on all four corner targets.
- [ ] Backlight: dims in visible steps and is silent at every step, or reported as not dimmable.
- [ ] Serial report shows the right model and revision.

If the picture is unstable, paste this:

```text
The picture is unstable: <describe what you see, idle or only under stress>. Apply the next mitigation from your research in order (pixel clock, buffering strategy, driver stack), one change at a time, and give me the same two checklist questions after each change. Record what was tried and the outcome in research.md.
```

## Step 8: Converge and merge

```text
/speckit.converge
```

Then:

```text
Update handover/docs/05-architecture.md with what we learned: confirmed pins and revision behaviour, the chosen display stack and why, backlight result, amplifier control. Mark the corresponding ⚠ items as verified with today's date. Record the owner's checklist answers in the feature directory. Commit and merge the feature branch.
```

## Done when

All checklist items are answered yes (or the backlight is recorded as fixed on), the stack decision is written down, and `check` passes.

Next: `slice-02-prayer-engine.md`.
