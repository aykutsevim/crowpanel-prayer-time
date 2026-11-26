# Slice 11: Basic and Advance boards

Goal: the same silent panel runs on the CrowPanel 7.0" basic board (DIS08070H) as well as on the Advance board (DIS02170A V1.4), with one firmware image per board and nothing lost on the Advance board. See `docs/specification/06-roadmap.md`.

Added on 2026-11-26 at the owner's request. Constitution 1.2.0 makes the basic board a second target subject to this slice. The specification is `specs/011-dual-board-support/spec.md`; it was written before Slices 8 to 10, and is implemented after Slice 9 unless the owner reorders.

Before starting: have a basic board at hand and read its hardware revision from the PCB.

## Step 1: Specify

Done on 2026-11-26 with:

```text
/speckit-specify Create Slice 11 that specifies how this app can both support Elecrow Basic and Advance ESP32-S3 hardware versions, update all necessary documents, on new feature branch
```

## Step 2: Clarify

```text
/speckit-clarify Focus on which basic-board revision is supported, what the wrong-board guard must do, and what the basic board shows after a power cut without Wi-Fi.
```

Suggested answers if asked: the revision on the owner's board only (expected V3.0); the guard keeps the other board silent and dark and names the expected board on serial; the basic board shows the waiting-for-time state until network time arrives.

## Step 3: Plan

```text
/speckit-plan Plan support for both boards.

Constraints and research:
- Read the vendor's documentation for the exact basic-board revision: the Elecrow wiki page, the repository Elecrow-RD/CrowPanel-7.0-HMI-ESP32-Display-800x480 pinned to a commit (schematic for that revision, example code, LCD datasheet) and the ESP32-S3-WROOM-1 datasheet. Settle every item in FR-003 with a citation in research.md. Any item the documents do not state goes to me as a yes/no or read-the-board question. Do not write basic-board code for an item that is not settled; if the amplifier shutdown line stays unknown, stop.
- Board profiles: one per board in src/platform, each fact cited; lib/core, lib/providers and lib/ui stay free of board knowledge.
- Silence on the basic board through the single silence function, first after reset (SIL-02); show why the silence step of each image is harmless on the other board.
- Wrong-board guard: how each image confirms its board after silence and before the panel, backlight and touch; what it does on a mismatch.
- Panel: timing for the basic board, the Slice 1 stress test and the mitigation ladder from research R8 of spec 001.
- Backlight on the basic board: fully on, no PWM (OD1).
- Clock: the basic board has no battery-backed clock unless the schematic says otherwise; waiting-for-time until network time (TIME-03, TIME-08).
- Flash: a 4 MB partition table (NVS, application, LittleFS for the day store and refresh.bin, core dump), the basic-board image budget, and the size today of the image built for it. Record the budget in the constitution and 03-functional-spec.md §12.
- Build: one PlatformIO environment, sdkconfig and partition table per board; tools/check.py builds and size-checks both; the silence check covers both.
- The Advance image must not grow by more than 16 kB.
```

## Steps 4 and 5

```text
/speckit-tasks
```

```text
/speckit-analyze
```

## Step 6: Implement

```text
/speckit-implement Stage 1, no hardware: board profiles, build environments, partition table, check for both images, wrong-board guard, clock without a chip. Report both image sizes against their budgets.
```

```text
/speckit-implement Stage 2, basic board: silence, panel, backlight, touch, then the full product. Give me the owner checklist with the exact commands for each step.
```

## Step 7: Owner checklist

One checklist per board, recorded in the feature directory.

Basic board:

- [ ] No sound at power-on, reset, Wi-Fi setup, stress mode and touch, with a speaker connected.
- [ ] Picture stable for 10 minutes in stress mode.
- [ ] Four corner touch targets register.
- [ ] Setup by QR code; the chosen district's Diyanet times appear.
- [ ] With sources blocked in software, the fallback behaves as on the Advance board.
- [ ] After a power cut with the router off: waiting-for-time, no wrong time; the real time appears when the router is back.
- [ ] The report names DIS08070H and its revision as configured.

Both directions:

- [ ] The Advance image on the basic board, and the basic image on the Advance board: no sound, dark screen, the expected board named on serial.

Advance board:

- [ ] Silence, picture, touch and real times as before.

## Step 8: Converge and merge

```text
/speckit-converge
```

## Done when

Both owner checklists are all yes, both images are within budget in `check`, and every project document names both boards.
