# Feature Specification: Support both the CrowPanel Basic and Advance 7.0" boards

**Feature Branch**: `feat/011-dual-board-support`

**Created**: 2026-11-26

**Status**: Draft

**Input**: User description: "Create Slice 11 that specifies how this app can both support Elecrow Basic and Advance ESP32-S3 hardware versions, update all necessary documents, on new feature branch"

**Slice**: 11 (`docs/specification/06-roadmap.md`). The directory number follows the slice number, as in specs 001 to 007, not the next free sequential number (008), which belongs to Slice 8.

## Context

Since 2026-11-20 the only build target is the CrowPanel Advance 7.0" (DIS02170A, revision V1.4). The basic model (DIS08070H) was dropped in Slice 1 because the owner had none, and the constitution says that adding it back is a new slice. This is that slice.

The two boards share the processor family, the 800×480 resolution and the touch controller family, so the product, its screens and all of its data logic are the same. What differs is the board around the processor:

| | Advance DIS02170A V1.4 (current) | Basic DIS08070H (to add) |
|---|---|---|
| Flash | 16 MB | 4 MB |
| Panel | IPS | TN (narrower viewing angle), different panel driver and timing |
| Screen, touch and bus wiring | one set of pins | an entirely different set of pins |
| Amplifier and buzzer control | through an auxiliary controller on the shared bus | amplifier on the processor's own pins, its shutdown line not yet identified |
| Backlight | through the auxiliary controller | a processor pin |
| Battery-backed clock | built in | none documented |
| Touch start-up | reset through the auxiliary controller | revision V3.0 needs an I/O-expander sequence |

Sources: `docs/specification/05-architecture.md` §1.1 and §1.2; the Elecrow wiki page and repository for the basic 7.0" board, read again on 2026-11-26. Several basic-board facts are not stated in those sources (see Assumptions and FR-003).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Silent on either board (Priority: P1)

As the owner, whichever of the two boards I put the panel firmware on, it never makes a sound: not at power-on, reset, Wi-Fi setup, refresh, touch, or under stress.

**Why this priority**: Silence is the product's first non-negotiable rule (constitution I). On the basic board the amplifier is controlled differently, and a mistake there is the one failure that cannot be tolerated.

**Independent Test**: Flash the basic-board firmware to a basic board and run the owner's listening checks of Slice 1 (power-on, reset, stress, Wi-Fi setup, touch), with and without a speaker connected.

**Acceptance Scenarios**:

1. **Given** a basic board with a speaker connected, **When** it is powered on, reset, put through Wi-Fi setup, stress mode and touch, **Then** the owner hears nothing.
2. **Given** an Advance board, **When** the same checks are repeated with the firmware of this slice, **Then** the owner hears nothing (no regression).

---

### User Story 2 - The same panel on the basic board (Priority: P1)

As an owner of a basic board, I flash the basic-board firmware, set the device up from my phone, and get the same panel as on the Advance board: the same screens, text and layout, working touch, a stable picture, real Diyanet times kept up to date, and the settings page.

**Why this priority**: This is the purpose of the slice. Without it there is nothing to ship.

**Independent Test**: On a basic board, run the owner checks of Slices 1, 3, 4, 6 and 7 that need eyes on the panel: picture stability under stress, touch at the four corners, setup by QR code, real times for the chosen district, and fallback when sources are blocked.

**Acceptance Scenarios**:

1. **Given** a freshly flashed basic board, **When** it starts, **Then** it shows the setup screen with its QR code, laid out as reference render 07.
2. **Given** a basic board set up with home Wi-Fi and a district, **When** it is connected, **Then** it shows the Diyanet times for that district on the main screen, matching the reference renders within the tolerances of `02-ui-spec.md` §12.
3. **Given** a basic board showing the stress screen, **When** Wi-Fi scans, HTTPS requests and flash writes run for 10 minutes, **Then** the picture does not shift, tear or flicker.
4. **Given** a basic board, **When** the owner taps the four corner targets of the touch test, **Then** each is registered within 10 px.

---

### User Story 3 - The Advance board keeps working exactly as before (Priority: P2)

As the owner of the existing Advance panel, I update to the firmware of this slice and notice no difference.

**Why this priority**: The Advance board is the one in use. Adding a second board must not cost the first one anything.

**Independent Test**: All host tests pass; the Advance image is built, flashed and given the short regression checks (silence, picture, touch, times).

**Acceptance Scenarios**:

1. **Given** the Advance board with the firmware of this slice, **When** it runs for a day, **Then** its behaviour, report lines and screens are the same as before, apart from the board line naming its model as before.

---

### User Story 4 - Honest about time without a battery-backed clock (Priority: P2)

As an owner of a basic board, after a power cut when the internet is not back yet, the device says it is waiting for the time instead of showing a wrong one, and shows the real time as soon as the network gives it.

**Why this priority**: The basic board has no documented battery-backed clock, so the Advance behaviour (time from the board's clock after a power cut) cannot be offered. The constitution requires that this is shown truthfully (principle II, TIME-03, TIME-08).

**Independent Test**: Power-cycle a set-up basic board with the router off; then switch the router on.

**Acceptance Scenarios**:

1. **Given** a set-up basic board and the router off, **When** power returns, **Then** the screen shows the waiting-for-time state (`no_time`) and no time of day or prayer countdown.
2. **Given** that state, **When** the router comes back, **Then** the real time and the stored Diyanet times appear without any action.

---

### User Story 5 - The wrong firmware on a board does no harm (Priority: P2)

As the owner or a developer, if the Advance firmware is flashed to a basic board or the other way round, nothing makes a sound, nothing is damaged, and the serial output says which board the firmware was built for.

**Why this priority**: Two images that look alike will be mixed up sooner or later. The pins of one board mean different things on the other, so an unchecked mix-up could switch on an amplifier.

**Independent Test**: Flash each image to the other board and listen; read the serial output.

**Acceptance Scenarios**:

1. **Given** the basic-board firmware on an Advance board (or the reverse), **When** it starts, **Then** there is no sound, the screen stays dark or blank, and within 5 seconds the serial output has one line naming the board the firmware expects.

---

### User Story 6 - A developer always knows which board and image (Priority: P3)

As a developer, one check command builds and tests both images and reports each image's size against its own budget, and the device report names the board model and revision it was built for.

**Why this priority**: Keeps both boards healthy in every later change without extra effort.

**Independent Test**: Run the check command; read the `report` output on each board.

**Acceptance Scenarios**:

1. **Given** the repository, **When** the check command runs, **Then** it builds both images, runs the host tests once, and prints each image's size against its board's budget, failing if either is over.
2. **Given** a basic board, **When** the `report` command runs, **Then** it prints the configured board model and revision, marked as configured, not detected.

### Edge Cases

- **Basic board revision**: the vendor lists V1.0, V2.0 (automatic download) and V3.0 (touch timing through an I/O expander). Only the revision the owner has, and whose documentation was read, is supported; another revision is refused in the documentation until it is checked.
- **Amplifier shutdown line not identifiable** from the vendor documentation and schematic: work stops and the question goes to the owner (constitution I and V); no basic-board firmware is flashed before this is settled.
- **The silence step of one image run on the other board**: it must not drive a line that enables sound on the other board (FR-005).
- **4 MB flash full**: the image, the day store for a whole year, the settings and the crash record must all fit; if the image grows past its basic-board budget, the check fails rather than the store shrinking silently.
- **The panel's pixel clock is on a boot-strapping pin** on the basic board: flashing and start-up must still work without pressing buttons on revision V2.0 and later.
- **TN panel**: the approved design is not changed for the narrower viewing angle; the owner checks readability from the usual viewing positions.
- **A battery-backed clock module added to the basic board's I²C connector** (TIME-08 mentions a DS3231): out of scope; the basic board is treated as having no battery-backed clock, without error messages.
- **Picture instability** under Wi-Fi load on the basic board, as seen on the Advance board in Slice 1: the same stress test and mitigation ladder apply.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The product MUST support two boards: the CrowPanel Advance 7.0" DIS02170A revision V1.4 (as today) and the CrowPanel 7.0" basic DIS08070H in the revision the owner has, expected to be V3.0. Other models and revisions are not supported.
- **FR-002**: There MUST be one firmware image per board, chosen when the firmware is built. The check command MUST build and size-check both images in every run; host tests run once, as they contain no board-specific behaviour.
- **FR-003**: Every basic-board fact the firmware relies on MUST come from the vendor's documentation and schematic for that exact revision, or from the owner's inspection of the board, and be recorded with its source in this feature's `research.md` before code uses it (constitution V). At least: the amplifier shutdown line and its inactive level; whether the PSRAM is octal or quad; the touch controller's reset and interrupt lines and bus address; what the I/O expander's lines drive; the panel's pixel clock, porches and polarities; whether a battery-backed clock exists.
- **FR-004**: On the basic board, the first action after reset MUST put the amplifier in shutdown and park its audio data lines, before any other peripheral, through the single silence function (SIL-02). No audio peripheral is initialised (SIL-01, SIL-03), and the silence check of every build covers both images (SIL-05).
- **FR-005**: Each image MUST confirm, after its silence step and before driving the panel, backlight or touch, that it is running on the board it was built for. On a mismatch it MUST stay silent, leave the screen dark, and write one serial line naming the expected board, then do nothing further. The silence step itself MUST be harmless on the other board: it must not drive any line that enables sound there.
- **FR-006**: On the basic board the screens MUST look as specified in `02-ui-spec.md` and the reference renders, within its §12 tolerances. The approved design is unchanged; there are no board-specific layouts, strings or colours (constitution III).
- **FR-007**: On both boards the backlight MUST be switched fully on and never dimmed (owner decision OD1, SIL-04). On the basic board no PWM is used for it.
- **FR-008**: On the basic board, touch MUST work from every start, including the start-up sequence that revision V3.0 needs; the four corner targets of the touch test register within 10 px.
- **FR-009**: On the basic board the picture MUST stay stable during 10 minutes of the Slice 1 stress mode (Wi-Fi scans, HTTPS requests, flash writes). The rule "no flash write during a Wi-Fi scan" applies on both boards.
- **FR-010**: The basic board's 4 MB of flash MUST hold the firmware image, the settings storage, a day store large enough for one district's full year (the 400-record store of Slice 6) plus the refresh state, and the crash record. The basic-board image MUST stay within its own size budget, set in this slice's plan and recorded in the constitution and `03-functional-spec.md` §12; the Advance budget stays 2.9 MiB.
- **FR-011**: On a board without a battery-backed clock, the device MUST show the waiting-for-time state after start-up until a network time is accepted, never a guessed or stale time (constitution II, TIME-03). The missing clock MUST NOT produce error lines or a degraded report entry (TIME-08).
- **FR-012**: Everything delivered by the slices up to and including the current one MUST work on the basic board, except what the board physically lacks (the battery-backed clock). The list of differences between the boards MUST be kept in `05-architecture.md` §1.
- **FR-013**: The device report and serial start-up lines MUST name the board model and revision the image was built for, marked "configured; not detected", on both boards.
- **FR-014**: The Advance image MUST behave as before this slice. Its size MUST NOT grow by more than 16 kB because of this slice.
- **FR-015**: The project documents MUST describe both boards: the constitution's hardware constraints and resource budget, `05-architecture.md` §1, §2 and §5, `01-product-brief.md`, `03-functional-spec.md` §12, `06-roadmap.md`, `docs/README.md`, the repository `README`, and the board-specific parts of the decision records. Statements that the basic board is not supported MUST be removed or marked as superseded.
- **FR-016**: Each board MUST have its own owner checklist in this feature directory with recorded yes/no answers: silence, picture stability, touch, setup, real times, fallback, waiting-for-time after a power cut (basic only), and the wrong-image check in both directions (constitution VII).

### Key Entities

- **Board profile**: everything the firmware needs to know about one board: model, revision, flash size, pins, panel timing, how silence, backlight and touch reset are done, whether a battery-backed clock exists, and the image size budget. One profile per supported board; each fact carries its source.
- **Firmware image**: the build for one board profile, with its size and budget.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In the owner's listening checks on both boards (power-on, reset, setup, stress, touch, wrong image), zero sounds are heard.
- **SC-002**: On the basic board the picture stays stable for 10 minutes of stress mode, as confirmed by the owner.
- **SC-003**: A basic board goes from first power-on to showing the chosen district's Diyanet times in under 5 minutes of the owner's time, the same setup steps as on the Advance board.
- **SC-004**: Every check run builds both images and reports each size against its budget; both are within budget at the end of the slice.
- **SC-005**: All host tests pass, and the Advance image grows by at most 16 kB compared with the last Slice 7 build (1 809 488 bytes).
- **SC-006**: With the wrong image on either board, no sound is heard, the screen stays dark and the expected board is named on the serial output within 5 seconds.
- **SC-007**: After a power cut with the router off, a basic board shows the waiting-for-time state within 30 seconds of power-on and never shows a wrong time; once the router is back, the real time appears without any action.

## Assumptions

- The owner obtains a basic 7.0" board (DIS08070H), expected revision V3.0, before the implementation is finished; owner checks for that board cannot be answered without it, and no basic-board behaviour is reported as verified before then (constitution V and VII).
- The vendor's V3.0 schematic, its example code and the module datasheet answer the open facts in FR-003. If one is not answered there, the owner checks the board; if the amplifier shutdown line remains unknown, the slice stops.
- The constitution was amended to version 1.2.0 together with this specification, making the basic board a second target subject to this slice; the exact basic-board budget and partition layout are fixed in the plan.
- The slice is specified now at the owner's request; its implementation follows the roadmap order (after Slice 9) unless the owner reorders it.
- Over-the-air update, an external clock module, the ESP32-P4 Advance board and other screen sizes are out of scope.
