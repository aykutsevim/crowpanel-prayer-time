# Feature Specification: Main screen with sample data

**Feature Branch**: `feat/003-main-screen`

**Created**: 2026-11-24

**Status**: Draft

**Input**: User description: "Slice 3 on new feat branch" (Slice 3 of `docs/specification/06-roadmap.md`; brief in `docs/development/prompts/slice-03-main-screen.md`)

## Context

The main screen is the product. It was designed and approved in advance (`docs/specification/02-ui-spec.md`, reference renders `docs/design/screens/01` to `14`), so this feature is faithful reproduction, not design. It is built before networking exists: the screen takes its content from the prayer-time engine of Slice 2 and from sample data with a simulated clock, so later slices only have to supply real data.

Order of authority for the look (02 §0): the reference renders first, then the written specification, then `docs/design/reference.html` and `docs/design/tokens.json`.

## Owner decisions

- **OD3-1 (2026-11-24): the clock comes from the board's real-time clock.** The board has a battery-backed real-time clock (owner: I²C address 0x51, CR1220 cell). The device reads the time from it at every start, so the main screen shows real time from this slice on, without the network. This brings forward the reading half of TIME-08 (planned for Slice 10); writing the clock after a network sync stays in Slice 5.
- **OD3-2 (2026-11-24, revised the same day): after a restart the device shows the main screen with the real time from the board's clock and the `no_data` state, exactly reference 14, because no prayer times are loaded in this slice.** The sample day is shown only when a fixture is called up over the serial port. History: the owner first chose "fixture 01, clock running" (sample times shown as today's); `/speckit-analyze` found that this breaks the non-negotiable truthful-display principle (II) (finding K1); the owner then approved the recommended truthful option ("fix all"). When the clock time is not valid, the `no_time` state is shown instead.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Glance from across the room (Priority: P1)

As someone in the room, I can read the current time, the name and bounds of the current prayer period, and the time remaining until the next one from three metres away.

**Why this priority**: This is what the device is for; everything else supports it.

**Independent Test**: Render the sample day at 14:32:07 on the development machine and compare it with reference `01-main-ogle-1432.png`; then show the same fixture on the panel and ask the owner.

**Acceptance Scenarios**:

1. **Given** the sample day at 14:32:07 with system state `ok`, **When** the main screen is drawn, **Then** it matches reference 01 within the tolerances of 02 §12 (colours exact after 16-bit quantisation, non-text geometry ±1 px, text baselines and anchors ±2 px, strings exact).
2. **Given** the same fixture on the panel, **When** the owner looks from three metres, **Then** the clock, countdown and six times are readable.

---

### User Story 2 - The whole day at a glance (Priority: P1)

As someone in the room, I see the whole day as one strip with a marker at the present moment, and six columns, each time connected by a line to its place on the strip.

**Why this priority**: It is the second half of the screen and the design's signature; it must be right before time-driven behaviour is added.

**Independent Test**: Measure, in the rendered sample day, the positions of strip segments, separators, hatch, hour scale, marker and leader lines, and compare them with the positions the spec's formulas give.

**Acceptance Scenarios**:

1. **Given** the sample day, **Then** the seven strip segments, the six separators, the three hatched kerahat intervals, the hour labels and ticks, the marker and the six leader lines are at the positions given by 02 §3, §4.2 to §4.4, within ±1 px.
2. **Given** the kerahat layer switched off, **Then** no hatch and no "Kerahat vakti" legend are drawn.

---

### User Story 3 - The screen follows time (Priority: P1)

As the owner, when time passes the screen follows it: the clock and countdown every second, the marker and notes every minute, and the column states, highlighted column and hero texts at each prayer time, including before İmsak, after Yatsı and at midnight.

**Why this priority**: A screen that does not move correctly is wrong for most of the day.

**Independent Test**: Render the fixtures for references 02 to 09 and compare each; run the simulated clock fast through a whole day on the panel.

**Acceptance Scenarios**:

1. **Given** the sample day, **When** the simulated time is that of reference 02 to 09 (03:30, 06:50, 10:00, 12:55, 16:20, 17:42, 18:20, 21:30), **Then** the screen matches that reference within tolerance.
2. **Given** 15 minutes or less remain, **Then** the countdown is drawn in the alert colour (reference 07); during the first 5 minutes of a period the current column's note reads `vakit girdi` (reference 05).
3. **Given** the simulated clock runs at high speed through a full day and across midnight, **Then** the owner sees no flicker, no leftover pixels and no sideways movement of the ticking digits.

---

### User Story 4 - Honest states (Priority: P2)

As the owner, when the device has no valid time, no data for today, only calculated times, no network for a while, or a nearly exhausted store of times, the screen shows the defined state. In this feature these states are forced by a developer command, since nothing real produces them yet.

**Why this priority**: The constitution's truthful-display rule depends on these states existing and looking right before real data arrives.

**Independent Test**: Render fixtures 10 to 14 and compare them with references 10 to 14.

**Acceptance Scenarios**:

1. **Given** each of the states `offline`, `cache_ending` (5 days), `calculated`, `no_time` and `no_data`, **When** forced over the sample day at 14:32:07, **Then** the screen matches references 10, 11, 12, 13 and 14 respectively.
2. **Given** several conditions at once, **Then** only the first matching state in the order of 02 §6 is shown.

---

### User Story 5 - Developer control from the serial port (Priority: P2)

As the developer, I drive the device from the serial port: set the simulated date and time, set how fast it runs, force a system state, and jump to any of the fourteen reference fixtures by number.

**Why this priority**: It is how the owner is shown each state on the panel, and how later slices test the screen.

**Independent Test**: For each fixture number 1 to 14, one command shows it on the panel; other commands change time, speed and state, and the screen follows.

**Acceptance Scenarios**:

1. **Given** the device is running, **When** the developer sends the fixture command with a number from 1 to 14, **Then** the panel shows that fixture.
2. **Given** a simulated time and a speed of, for example, 60×, **Then** the clock advances 60 seconds per real second and the screen follows.
3. **Given** a forced state, **When** the developer clears it, **Then** the screen returns to the state the inputs give.

---

### User Story 6 - Self-checking without the device (Priority: P2)

As the developer, I render each fixture to an image file on the development machine, so the screen is compared with the reference before the owner is asked to look.

**Why this priority**: The implementer cannot see the panel; this is how fidelity is checked repeatedly and cheaply.

**Independent Test**: One command renders fixtures 01 to 14 into image files; a measurement step reports the positions of the strip, the current column and the marker.

**Acceptance Scenarios**:

1. **Given** the development machine, **When** the render command runs, **Then** fourteen 800×480 images are produced, one per fixture, with the reference file's name.
2. **Given** a rendered image, **Then** the measurement step reports the bounding boxes of the strip, the current column and the marker, and the automated checks compare them with the formulas.

---

### User Story 7 - Diagnostics become hidden (Priority: P3)

As the owner, the device now starts on the main screen, and I can still reach the diagnostic and settings screens from Slice 1 through a hidden touch gesture or the serial command `diag`.

**Why this priority**: Required by Slice 1's decision that diagnostics stay in the product (spec 001 FR-007a), but not part of normal use.

**Independent Test**: Restart the device; perform the gesture; perform a single tap; send `diag`; leave the diagnostic screen.

**Acceptance Scenarios**:

1. **Given** a restart, **Then** the main screen appears, never the diagnostic screen.
2. **Given** the main screen, **When** the owner holds two fingers on it for 5 seconds, **Then** the diagnostic screen opens.
3. **Given** the main screen, **When** the owner taps once with one finger, or touches with two fingers for less than 5 seconds, **Then** nothing changes.
4. **Given** the diagnostic screen, **When** the owner leaves it (on-screen control or serial command), **Then** the main screen returns, and stress mode, if running, stops.
5. **Given** the main screen, **When** the serial command `diag` is sent, **Then** the diagnostic screen opens.

---

### User Story 8 - Real time from the board's clock (Priority: P1)

As the owner, after a restart or a power cut the screen shows the correct local time without any network, because the board keeps time on its own battery.

**Why this priority**: Owner decision OD3-1; without it the screen cannot show the right moment after a restart in this slice.

**Independent Test**: Set the board's clock once over the serial port, cut the power, wait, power on again, and compare the clock on screen with a reference clock.

**Acceptance Scenarios**:

1. **Given** the board's clock holds a valid time, **When** the device starts, **Then** the main screen shows that time converted to local time (Türkiye, UTC+3) within 2 seconds of a reference clock.
2. **Given** the board's clock reports that its time may be corrupt (for example after the battery was removed), or does not answer, or holds a time earlier than the firmware's build time, **When** the device starts, **Then** the `no_time` state is shown (reference 13) and the log says why.
3. **Given** the developer sets the time over the serial port, **Then** the board's clock is written in one operation, the "may be corrupt" mark is cleared, and the screen follows at once.
4. **Given** the power is cut for several minutes and restored, **Then** the time on screen is still correct within 2 seconds.

---

### Edge Cases

- **Marker at the edges**: at 00:00:00 and 23:59:59 the marker and its gap stay inside the strip's rounded ends and are fully visible.
- **Nearly vertical leader lines**: a prayer time whose strip position is almost straight above its column still draws a clean line.
- **Longest strings in each slot**: for example `Çevrimdışı, kayıtlı vakitler kullanılıyor` in the status slot, `11 sa 56 dk sonra` in a column note, `Cemaziyelevvel` in the Hijri date, `Kuşluk vakti` as the period name, and a countdown above ten hours; none overlaps its neighbour.
- **Long city name**: a city wider than 300 px is cut with `…` and never runs into the status text.
- **Calculated state**: every column time carries the `≈` prefix at the smaller size, and the widest column still fits its box.
- **Midnight**: the night period continues; the six columns change to the new day's times in the same second.
- **Speed changes and jumps**: a time jump forward or back redraws everything for the new moment with no stale element.
- **Two fingers, one lifted**: if one of the two fingers lifts before 5 seconds, the gesture is cancelled.
- **Real-time clock never set**: a new or battery-less board reports its time as possibly corrupt; the screen shows `no_time` until the time is set.
- **Clock in a later century**: a stored time with the century mark set, or any field outside its valid range, is treated as not valid.
- **Midnight on the real clock**: in `no_data` the date line changes at midnight and nothing else does.

## Requirements *(mandatory)*

### Functional Requirements

**Look (02 §1 to §5, §9, UI-01)**

- **FR-001**: The main screen MUST reproduce `02-ui-spec.md` §4 (elements M1 to M21) at the specified positions, sizes, colours and type, on a flat 800×480 canvas with the 24 px side margin, no gradients, shadows, images or animation (02 §1).
- **FR-002**: Colours MUST be the tokens of 02 §2.1, exact after 16-bit quantisation; yellow (`now`) MUST be used only for "now / next" elements.
- **FR-003**: Text MUST use Barlow (400, 500, 600) and Barlow Condensed SemiBold at the sizes and with the glyph sets of 02 §2.2, using only the consolidations that section allows. The font files MUST be kept in the repository with their licence and a way to regenerate them, and their combined flash cost MUST stay within the 300 kB font budget (05-architecture §5).
- **FR-004**: The clock, seconds, countdown, period bounds and column times MUST be laid out one fixed cell per character (digit cell = round(0.475 × size), colon cell = round(0.252 × size), glyph centred), so ticking digits never move sideways (02 §2.2).
- **FR-005**: Every horizontal position on the timeline MUST follow X(t) = 24 + t / 86 400 × 752, and column centres Cx(k) (02 §3).
- **FR-006**: The day strip MUST draw its seven segments, the six 2 px separators, the hour labels and ticks, and, when the kerahat layer is on, the hatch over the three intervals and the `Kerahat vakti` legend (02 §4.2, M9, M16, M17).
- **FR-007**: The now marker (gap, line, triangle) MUST be drawn at X(now) above the strip (02 §4.3).
- **FR-008**: Each prayer time MUST have its four-point leader line; the current column's line is drawn in `now` at 2.5 px (02 §4.4).
- **FR-009**: The six columns MUST show name, time and note in the states `past`, `current`, `next`, `later` of 02 §4.5, including the after-Yatsı and before-İmsak cases.
- **FR-010**: The hero MUST show the normal, soon and entered states of 02 §5; the progress fill MUST equal the elapsed fraction of the period.
- **FR-011**: All on-screen strings MUST be exactly those of 02 §6 and §9, including U+2019 and U+2026; the city name MUST be truncated with `…` when wider than 300 px.

**States (02 §6, UI-01, constitution II)**

- **FR-012**: Exactly one system state MUST be shown, chosen top to bottom from `no_time`, `no_data`, `calculated`, `cache_ending`, `offline`, `ok` (02 §6), each with its status icon, status text, colour and other changes as specified, and as the reference renders show where they add detail. Reference 14 keeps a Hijri date in `no_data`: it MUST come from the newest stored record when there is one (the sample day in fixture 14) and MUST be empty otherwise, since no Hijri date is calculated (spec 002).
- **FR-013**: In the `calculated` state every column time MUST carry the `≈` prefix and be drawn at 44 px (02 §6).

**Behaviour (UI-02 to UI-04)**

- **FR-014**: The screen MUST be drawn only from the engine's result for the current moment and a system-state input (location name, last update, days stored, kerahat setting); it MUST NOT read the network or storage while drawing (UI-02, UI-04).
- **FR-015**: Redraw work MUST be bounded as UI-03 requires: each second only the clock, seconds, countdown and progress fill; each minute also the marker and column notes; at a prayer time also column states, leaders and hero texts; at a new day, everything.
- **FR-016**: Value changes MUST appear without flicker, tearing or leftover pixels, also when the simulated clock runs fast or jumps.

**Sample data and developer control (SYS-06)**

- **FR-017**: Fourteen fixtures MUST be defined once, each with its moment, sample records, system state and reference file name (02 §11), and be used by both the development-machine renderer and the device.
- **FR-018**: A development build MUST offer serial commands to set the simulated date and time, set the speed of the simulated clock, force a system state (with the values it needs, such as the number of stored days), clear a forced state, and show fixture 1 to 14. These commands MUST NOT exist in a release build (SYS-06).
- **FR-019**: After a restart the device MUST show the main screen, never the diagnostic screen. With a valid time from the board's clock it MUST show the `no_data` state for that local time (reference 14: clock and date running, `Vakitler alınamadı`, placeholders), because no prayer times are loaded (OD3-2, constitution II). Without a valid time it MUST show the `no_time` state with no stored times. Sample prayer times appear only through the fixture command.

**Real-time clock (OD3-1, TIME-03, TIME-08)**

- **FR-026**: At every start the device MUST read date and time from the board's real-time clock in one single read of all time registers, and MUST accept it as valid only if the chip answers, its "clock integrity not guaranteed" mark is clear, every field is in range, and the time is not earlier than the firmware's build time (TIME-03). Otherwise the time is not valid and the reason is logged.
- **FR-027**: The board's clock MUST hold UTC. Local time MUST be derived with the configured zone, default UTC+3 (TIME-04); Türkiye has no daylight saving.
- **FR-028**: Between reads the time MUST run from the system's own timer (TIME-05); the board's clock is read at start and is not written except by an explicit command.
- **FR-029**: A development-build serial command MUST set the board's clock from a given local date and time, writing all time registers in one operation and clearing the integrity mark, and the screen MUST follow at once. Reading the clock MUST also be possible by command, printing the stored time and its validity.
- **FR-030**: Leaving a fixture (or clearing the simulated clock) MUST return the screen to the real-time clock.

**Development-machine rendering (05-architecture §7)**

- **FR-020**: A development-machine command MUST render fixtures 01 to 14 to 800×480 image files named after their references, from the same screen code as the device.
- **FR-021**: A measurement step MUST report the pixel bounding boxes of the strip, the current column and the marker in a rendered image, and automated checks MUST compare them with the formulas of 02 §3 to §4.5.

**Hidden diagnostics (spec 001 FR-007a)**

- **FR-022**: The diagnostic and settings screens of Slice 1 MUST stay in the firmware and MUST open only through (a) two fingers held on the main screen for 5 seconds, or (b) the serial command `diag`. A single tap, a short two-finger touch, or one finger held MUST NOT open them, and in this slice a tap on the main screen MUST change nothing (the status overlay of 02 §7.4 is not part of this slice).
- **FR-023**: Leaving the diagnostic mode MUST return to the main screen and stop stress mode if it runs.

**Constraints carried forward**

- **FR-024**: The device MUST stay silent; state changes are visual only (constitution I, 02 §5).
- **FR-025**: The firmware image size MUST be reported after each build and stay within the 2.9 MB budget, with the fonts' share reported separately.

### Key Entities

- **Fixture**: a numbered sample (1–14) with a moment, sample day records, a system state with its values, and the reference image it must match.
- **System state input**: the state to show and the values it needs: location name, time of the last update, days of stored times, whether the clock is valid, whether today's times are calculated, whether the network is down.
- **Simulated clock**: a date and time that advances at a chosen speed, settable from the serial port, used by the fixtures and fast-forward tests.
- **Real-time clock reading**: the date and time held by the board's battery-backed clock (UTC), with its validity and, if not valid, the reason.
- **Screen view model**: the engine's result for the moment plus the system state input; everything the screen draws comes from it.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: All fourteen fixtures, rendered on the development machine, match their reference renders within the tolerances of 02 §12, and each has been reviewed side by side before the owner is asked to look.
- **SC-002**: Measured positions of the strip, separators, marker, leader lines and the current column box are within ±1 px of the formulas for every fixture that shows them.
- **SC-003**: The owner confirms on the panel that fixture 01 matches reference 01 and is readable from three metres, that fixtures 02 to 14 match their references, and that the six strip colours are distinguishable, also viewed from the side and from below.
- **SC-004**: Running the simulated clock through a full day at high speed, the owner sees no flicker, leftover pixels or moving digits.
- **SC-005**: Every fixture can be shown on the panel with one serial command.
- **SC-006**: After a restart the device shows the main screen; the hidden gesture opens the diagnostic screen; a single tap does not.
- **SC-007**: The device makes no sound at any point.
- **SC-008**: The firmware stays within the 2.9 MB budget and the fonts within 300 kB, both reported by the check command.
- **SC-009**: After the clock has been set once, the time on screen after any restart or power cut of up to one hour is within 2 seconds of a reference clock.
- **SC-010**: With the clock's integrity mark set (or the chip absent), the device shows `no_time` within 5 seconds of power-on and never shows a guessed time.

## Assumptions

- Fidelity is judged as the slice prompt suggests: the reference render wins over prose; text baselines within 2 px and non-text geometry within 1 px are acceptable; glyph shapes may differ slightly from the browser render; there is no animation.
- The sample data is the shared fixture of Slice 2 (`03-functional-spec.md` §3) with the location `İstanbul` and last update 03:00, as the references show. Fixture 11 uses 5 stored days; all state fixtures use 14:32:07.
- Network time (Slice 5), real data (Slice 6), Wi-Fi, the status overlay and its touch interactions (02 §7.4, §8), and the setup and connecting screens are out of scope. The touch input in this slice is the hidden gesture only.
- The two-finger 5-second hold is the slice prompt's proposal; the touch controller reports up to five points, and the number of points is available from its status register (spec 001 research R5).
- The device firmware in this slice is a development build; the release build without the simulated-time commands is produced in Slice 9.
- Slice 1's rule stays: no flash write while a Wi-Fi scan is in progress.
- The real-time clock is the BM8563 (U4) of the V1.4 schematic, register-compatible with the NXP PCF8563 (the parts list names PCF8563MDTR as its alternative), on the shared I²C bus (SDA 15, SCL 16) at address 0x51, battery BT1 (CR1220), interrupt line not connected. The vendor's factory firmware reads and sets it with the `I2C_BM8563` library at the same address. Register facts come from the NXP PCF8563 data sheet Rev. 11.1 (19 January 2026); details go in research.md.
- Whatever time the factory firmware left in the clock is not trusted for its zone: the owner sets the clock once with the serial command, and the owner checklist asks whether the time on screen is right.
