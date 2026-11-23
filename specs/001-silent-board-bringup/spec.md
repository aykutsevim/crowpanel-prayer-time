# Feature Specification: Silent Board Bring-up

**Feature Branch**: `feat/001-silent-board-bringup`

**Created**: 2026-11-20

**Status**: Draft (clarified with the owner on 2026-11-20)

**Input**: User description: "Slice 1 on a new feat branch" — Slice 1 of `docs/specification/06-roadmap.md`, as described in `docs/development/prompts/slice-01-board-bringup.md` (Step 1).

**Requirements covered**: MODE-01, SIL-01, SIL-02, SIL-03, SIL-04, SIL-05, SIL-06, SEC-01, SEC-02, SEC-04, NET-02 (access-point identity only), NET-08, and the hardware risks in `docs/specification/05-architecture.md` §2.

**Board under test**: CrowPanel Advance 7.0", model **DIS02170A**, hardware revision **V1.4** (owner, read from the board, 2026-11-20).

## Context

Every later slice depends on four hardware basics that nobody has verified on this unit yet: a stable picture, working touch, a controllable backlight, and total silence. The implementer cannot see or hear the device, so this feature also gives the owner the means to verify each point and report back. The owner's answers decide the display driver choice for the rest of the project; the backlight is fixed fully on (OD1).

The board is the Advance model, which `05-architecture.md` §1.2 treats as a second profile and documents only up to revision V1.3. For this board, silence covers more than one part. The amplifier, the buzzer, the audio-routing DIP switches and anything the board's auxiliary controller can drive must all be identified for V1.4 and held silent.

## Owner decisions recorded in this spec

| Date | Decision | Effect on the project documents |
|---|---|---|
| 2026-11-20 | The target board is DIS02170A, revision V1.4. | The Advance profile becomes the project's build target. The constitution's "default target" line (basic, DIS08070H) and `05-architecture.md` §1.2 must be updated in this branch. The 2.9 MB image budget stays as a discipline. |
| 2026-11-20 | Stress mode joins the home network so the encrypted request really runs. Wi-Fi details are entered from another phone through the device's own hotspot, started from an on-device settings screen, and stored on the device. | This moves a minimal form of Wi-Fi provisioning from Slice 4 into Slice 1. The roadmap's Slice 1 out-of-scope line ("Wi-Fi connection to a real network") must be updated in this branch. Slice 4 builds the full setup flow (QR code, captive portal, location, classified errors, product screens) on top of the stored-credentials mechanism from this slice. |
| 2026-11-20 | No backlight dimming, ever (OD1, plan review). The board's auxiliary controller dims at an undocumented frequency, so the ≥ 20 kHz rule (SIL-04) cannot be confirmed; the backlight is always fully on. | User Story 5 becomes "backlight fully on, never dimmed". SET-04 brightness and night dimming are dropped from Slice 8; `01-product-brief.md` open question 4 is answered "no"; `03-functional-spec.md` SET-04 and `06-roadmap.md` Slice 8 must say so (FR-033). |
| 2026-11-20 | The diagnostic and settings screens stay in the product firmware for good, reachable through a hidden gesture or a serial command. | From Slice 3 on, the device boots to the product screen and the diagnostics become a hidden mode. Their flash cost counts against the image budget in every later slice. `05-architecture.md` §4 and the Slice 3 prompt must say so. |

## Clarifications

### Session 2026-11-20

- Q: If the device cannot confirm at start-up that the audio path is off, what should it do? → A: Retry a few times, then mark the silence self-test failed in the alert colour; the diagnostic screen and report keep working, but stress mode and Wi-Fi setup stay blocked until a restart passes the silence step.
- Q: How often may the stress test make its secure web request to one of the project's allowed hosts? → A: At most one request every 30 seconds, with a small response, to one allowed host; the Wi-Fi scan and storage write keep running every cycle.
- Q: For the four-corner touch test, should the device judge accuracy itself or only show the coordinates? → A: The device judges each corner target (pass within 10 px or fail), shows the error in pixels, and adds the results to the serial report; the owner answers one yes/no question.
- Q: After this slice, should the diagnostic screens stay in the product firmware, or live in a separate test firmware? → A: They stay inside the product firmware, reachable through a hidden gesture or a serial command.
- Q: What should protect actions like "forget Wi-Fi" and "start Wi-Fi setup" on the settings screen from accidental taps? → A: Every action that changes stored data or the network needs an explicit on-screen confirmation; backlight and stress controls do not.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Silence (Priority: P1)

As the owner, when I power the device, restart it, set up Wi-Fi, or leave it running while it uses Wi-Fi and storage, it never produces any sound. That covers the speaker port, any built-in speaker, the buzzer, and the backlight circuit.

**Why this priority**: Silence is the product's first non-negotiable constraint (SIL-01). Nothing else may be built on a board that can make a noise. The Advance board has more sound-capable parts than the basic board.

**Independent Test**: Flash the firmware, then power-cycle and reset the device in a quiet room with an ear close to the speaker port, the buzzer and the board. The owner reports whether any sound was heard. The firmware's own report confirms that the audio-off step ran first.

**Acceptance Scenarios**:

1. **Given** the device is unpowered, **When** the owner applies power, **Then** no click, pop, hiss, chirp, tone or whine is audible at any point of the boot, and the serial report states that the audio path and the buzzer were disabled as the first step after reset (SIL-02, MODE-01).
2. **Given** the device is running, **When** the owner presses reset, **Then** no sound is heard during the restart.
3. **Given** stress mode is running (Story 3), **When** ten minutes pass, **Then** no sound is heard at any point.
4. **Given** the backlight is fully on (Story 5), **When** the owner listens close to the board, **Then** no whine is heard (SIL-04).
5. **Given** the owner touches the screen or uses the settings screen, **When** touch is registered, **Then** there is no audible or haptic feedback (SIL-06).
6. **Given** the hotspot is running and a phone submits Wi-Fi details (Story 7), **When** the device connects or fails to connect, **Then** no sound is heard.
7. **Given** the source tree of this feature, **When** the project's check command runs, **Then** the silence check passes (SIL-05).

---

### User Story 2 - Stable picture (Priority: P2)

As the owner, I see a boot screen and then a diagnostic screen that lets me judge the display. It shows full-screen colour bars in a known order, each labelled with its colour name. It also shows a one-pixel white border on all four edges, a field of alternating one-pixel vertical lines, and the text "800 x 480".

**Why this priority**: The whole product is a display; every later slice assumes the panel shows exactly the pixels it is given.

**Independent Test**: Power the device and compare the diagnostic screen against the expected layout. The implementer also renders that layout on the development machine so the owner has a reference image.

**Acceptance Scenarios**:

1. **Given** the device is powered, **When** boot completes, **Then** a boot screen appears first, followed by the diagnostic screen, without waiting for any network (MODE-01).
2. **Given** the diagnostic screen is shown, **When** the owner reads the colour bars, **Then** each bar's colour matches its on-screen label.
3. **Given** the diagnostic screen is shown, **When** the owner looks at the orientation hint, **Then** the picture is landscape and the right way up, with the USB connector on the side the hint names.
4. **Given** the diagnostic screen is shown, **When** the owner inspects each edge, **Then** the one-pixel border is visible on all four edges.
5. **Given** the diagnostic screen is shown and idle, **When** the owner watches the vertical-line field for one minute, **Then** it does not shift, tear, crawl or flicker.

---

### User Story 3 - Stability under load (Priority: P3)

As the owner, I can start a stress mode that keeps the diagnostic picture on screen for at least ten minutes. During that time the device continuously scans for Wi-Fi networks, makes encrypted web requests with certificate verification over the stored home network, and writes to its storage.

**Why this priority**: Picture instability under Wi-Fi and flash activity is the largest known risk on this class of board (`05-architecture.md` §2). It decides the display driver stack for the whole project.

**Independent Test**: With Wi-Fi set up (Story 7), start stress mode, watch the screen for ten minutes, and read the cycle and error counters at the end.

**Acceptance Scenarios**:

1. **Given** the diagnostic screen is shown, **When** the owner starts stress mode from the on-screen control or from the serial console, **Then** stress mode starts and the screen shows that it is running.
2. **Given** stress mode is running and the device is connected to the home network, **When** ten minutes pass, **Then** the picture stays as stable as in Story 2 (no shift, tear, crawl or flicker in the vertical-line field), and the encrypted-request counter shows about one success per 30 seconds.
3. **Given** stress mode is running, **When** each cycle completes, **Then** the screen and the serial port show a running count of completed cycles and of errors for each activity (Wi-Fi scan, encrypted request, storage write).
4. **Given** no Wi-Fi details are stored, or the stored network is not reachable, **When** an encrypted request is due, **Then** that step is reported as skipped with the reason ("no Wi-Fi set up" or "not connected"), not as passed or failed.
5. **Given** stress mode is running, **When** the owner stops it from the screen or the serial console, **Then** the activity stops and the final counts remain visible.

---

### User Story 4 - Touch (Priority: P4)

As the owner, when I touch the screen I see a marker under my finger and the coordinates of the touch.

**Why this priority**: Touch is needed for the settings screen in this slice and for the product from Slice 4 onward. On some board revisions it only works after a specific start-up sequence.

**Independent Test**: Touch each of the four corner targets on the diagnostic screen and check that all four are marked passed; the serial report gives the exact errors.

**Acceptance Scenarios**:

1. **Given** the diagnostic screen is shown, **When** the owner touches anywhere outside a control, **Then** a marker appears under the finger and the screen shows the touch coordinates.
2. **Given** four corner targets are shown, **When** the owner touches the centre of each target, **Then** the target is marked passed (within 10 pixels in both directions) or failed, the error in pixels is shown next to it, and the result is added to the serial report.
3. **Given** the owner lifts the finger, **When** no touch is present, **Then** the marker no longer follows and the screen indicates no active touch.

---

### User Story 5 - Backlight fully on, never dimmed (Priority: P5)

As the owner, I see the backlight come on at full brightness at every boot and stay there. The device never dims it (owner decision OD1, 2026-11-20).

**Why this priority**: The backlight must work for anything to be visible. Dimming was dropped because its frequency on this board cannot be shown to be inaudible (SIL-04).

**Independent Test**: Power the device and check that the backlight is on and steady; the report states "backlight full, dimming disabled".

**Acceptance Scenarios**:

1. **Given** the device is powered, **When** the silence step has run, **Then** the backlight is switched fully on and stays fully on.
2. **Given** the device is running, **When** the owner looks for a brightness control on the screen or in the serial commands, **Then** there is none, and the screen and report state "backlight full, dimming disabled".
3. **Given** the backlight is on, **When** the owner listens close to the board in a quiet room, **Then** no whine is heard (SIL-04).

---

### User Story 6 - Evidence (Priority: P6)

As the developer, I get a short report on screen and on the serial port, so the owner can read it out or paste it. It gives the board model and revision (configured, and detected where the board allows), the Wi-Fi state, free memory, and the result of each self-test.

**Why this priority**: The implementer cannot see the device; this report is the main channel of evidence besides the owner's answers.

**Independent Test**: Connect a serial console, restart the device, request the report, and compare its fields with the board label.

**Acceptance Scenarios**:

1. **Given** the device boots, **When** start-up finishes, **Then** the serial port prints the report without being asked, and the report is also shown on the diagnostic screen.
2. **Given** a serial console is connected, **When** the owner sends the report command, **Then** the report is printed again with current values.
3. **Given** a self-test fails (for example, touch controller not found), **When** the report is produced, **Then** that test is marked failed (in the alert colour on screen) with a short reason, and all other self-tests still run and report.
4. **Given** the display shows nothing, **When** the device boots, **Then** the serial report still appears in full.
5. **Given** Wi-Fi details are stored, **When** the report is produced, **Then** it shows the network name and connection state but never the password.

---

### User Story 7 - Wi-Fi setup from another phone (Priority: P7)

As the owner, I open a settings screen on the device and start Wi-Fi setup. The device opens its own hotspot and shows its name and password. I join it with a phone, open the page the screen names, and enter my home network name and password. The device stores them, joins the network, and reconnects to it after every restart. From the same settings screen I can also forget the stored network.

**Why this priority**: It makes the encrypted-request part of the stress test (Story 3) real. Stories 1, 2 and 4 to 6 work without it.

**Independent Test**: From the settings screen, start Wi-Fi setup and join the hotspot from a phone. Submit the home network details, see the device connect, restart it, and see it reconnect without setup.

**Acceptance Scenarios**:

1. **Given** the diagnostic screen is shown, **When** the owner opens the settings screen, starts Wi-Fi setup and confirms, **Then** the device starts a password-protected hotspot and shows the hotspot name, its password and the address of the setup page on screen.
2. **Given** a phone has joined the hotspot, **When** the owner opens the setup page, **Then** the page lists the visible networks and lets the owner pick one or type a name, and enter a password.
3. **Given** the owner submits a network name and password, **When** the device connects successfully, **Then** the details are stored on the device, the hotspot closes, and the device screen shows "connected" with the network name.
4. **Given** the owner submits details that do not work, **When** the connection attempt fails, **Then** the details are discarded, the hotspot stays up, and the device screen and the setup page say the connection failed.
5. **Given** Wi-Fi details are stored, **When** the device restarts, **Then** it rejoins the network in the background without blocking the diagnostic screen (MODE-01).
6. **Given** Wi-Fi details are stored, **When** the owner chooses "forget Wi-Fi" on the settings screen and confirms, **Then** the stored details are erased and the device disconnects.
7. **Given** the hotspot is running, **When** the owner cancels setup on the settings screen, **Then** the hotspot closes and any previously stored details are kept.

---

### Edge Cases

- **Touch controller absent or not responding**: the touch self-test fails with a reason. The diagnostic screen and stress mode remain usable through serial commands. Wi-Fi setup can also be started and stopped from the serial console.
- **Display initialises but shows nothing**: the serial report still appears and states the display self-test result. While the hotspot runs, the serial console also prints the hotspot name and the setup-page address, but never the hotspot password (FR-024). Wi-Fi setup therefore needs a working display; with a dead display the owner records the failure instead. Silence still holds.
- **No Wi-Fi networks in range**: the scan step completes with zero networks and counts as a completed scan, not as an error.
- **No Wi-Fi details stored, or the stored network is unreachable**: the encrypted-request step is reported as skipped with the reason on every cycle. Reconnection is retried in the background.
- **Encrypted request fails certificate verification or times out**: it is counted as an error with a short reason, and stress mode continues. Certificate checks are never relaxed (SEC-01).
- **Storage write fails or storage is full**: it is counted as an error, and the scratch data never grows without bound.
- **Wi-Fi details stored while stress mode is running**: stress mode picks up the new connection on its next cycle.
- **Hotspot started while stress mode is running**: stress mode pauses its Wi-Fi activity while the hotspot is up and resumes afterwards. The pause shows on the counters.
- **Hidden network or open network (no password)**: the setup page accepts a typed name and an empty password.
- **Backlight audible even at full brightness**: recorded as a silence failure (checklist A3); the slice is not done until it is resolved with the owner.
- **Board revision not detectable by the firmware**: the report shows the configured revision and states that it was not detected.
- **V1.4 not covered by vendor documentation**: planning stops and asks the owner to inspect the board. Nothing is assumed from V1.3.
- **DIP switches set so that the speaker path is selected**: the firmware still holds the amplifier and buzzer inactive. The owner checklist includes the recommended switch position.
- **Audio-off step not confirmed**: the silence self-test fails in the alert colour with its reason, and stress mode and Wi-Fi setup are blocked until a restart passes the step (FR-001a). Already-stored Wi-Fi details are not used for joining a network either.
- **Confirmation dismissed or ignored**: nothing changes. An unanswered confirmation closes on its own after a short time.
- **Serial command not recognised**: a one-line help is printed; nothing else changes.

## Requirements *(mandatory)*

### Functional Requirements

**Silence**

- **FR-001**: The first action after reset, before any other peripheral is started, MUST put the audio amplifier into shutdown, hold every audio signal line at a fixed level, hold the buzzer inactive, and keep anything the auxiliary controller drives that can make sound inactive (SIL-02, MODE-01).
- **FR-001a**: If the audio-off step cannot be confirmed (for example, a control line or the auxiliary controller does not acknowledge), the device MUST retry a bounded number of times. If it still cannot confirm, it MUST mark the silence self-test failed in the alert colour, with the reason. The diagnostic screen and report MUST keep working, but stress mode and Wi-Fi setup MUST refuse to start, on screen and from the serial console, until a restart passes the silence step.
- **FR-002**: The firmware MUST NOT start any audio peripheral, generate tones on any pin, or include any audio library (SIL-01, SIL-03, SIL-05).
- **FR-003**: The firmware MUST NOT generate PWM on any pin, and MUST NOT ask the board's auxiliary controller to dim the backlight (SIL-04, owner decision OD1).
- **FR-004**: Touch MUST produce no audible or haptic feedback (SIL-06).

**Boot and display**

- **FR-005**: After the audio-off step, the device MUST bring up display, touch and backlight and show a boot screen, then the diagnostic screen. It MUST NOT wait for any network (MODE-01).
- **FR-006**: The diagnostic screen MUST show all of the following:
  - full-screen colour bars in a fixed, documented order, each labelled with its colour name
  - a one-pixel white border on all four edges
  - a field of alternating one-pixel vertical lines
  - the text "800 x 480"
  - an orientation hint naming the side of the USB connector
  - four corner touch targets
  - live touch coordinates
  - free memory
  - backlight state ("full, dimming disabled")
  - Wi-Fi state
  - stress-mode status and counters
  - the self-test results
  - a control that opens the settings screen
- **FR-007**: In this slice, with no product screen yet, the device MUST boot to the diagnostic screen and stay there until restarted, apart from visits to the settings screen, which return to it.
- **FR-007a**: The diagnostic and settings screens MUST remain part of the product firmware after this slice. From the slice that adds the product screen, they MUST be reachable only through a hidden touch gesture and through a serial command. The gesture MUST be deliberate enough not to be triggered by everyday touches, and MUST NOT collide with any touch interaction defined in `02-ui-spec.md`. Leaving the diagnostic mode MUST return to the product screen, and a restart MUST always boot to the product screen. Stress mode MUST stop when the diagnostic mode is left.
- **FR-008**: The expected appearance of the diagnostic and settings screens MUST be renderable on the development machine, so the implementer can compare them before asking the owner.

**Stress mode**

- **FR-009**: The owner MUST be able to start and stop stress mode from an on-screen control and from a serial command.
- **FR-010**: Each stress cycle MUST scan for Wi-Fi networks and make a small write to a scratch file in device storage. Stress mode MUST also make encrypted web requests with certificate verification enabled, to one host from the allowed list in `04-data-sources.md` §11, asking for a small response (SEC-01, SEC-02). These requests MUST be at most one every 30 seconds and one at a time. Cycles in which no request is due MUST NOT count the request as skipped or failed.
- **FR-011**: When no Wi-Fi details are stored or the device is not connected, the encrypted-request step MUST be reported as skipped with its reason.
- **FR-012**: Stress mode MUST keep the diagnostic picture on screen. It MUST show a running count of completed cycles and of errors per activity, on screen and on the serial port.
- **FR-013**: The scratch file MUST be bounded in size, and stress mode MUST NOT touch any other stored data.

**Touch**

- **FR-014**: The device MUST show a marker at each touch position and the touch coordinates, and indicate when no touch is present.
- **FR-014a**: Each of the four corner targets MUST be judged by the device when touched: passed if the touch is within 10 pixels of the target centre in both directions, otherwise failed. The result and the error in pixels MUST be shown at the target and included in the serial report. A target can be retried, and the latest result counts.
- **FR-015**: If the board needs a start-up sequence for touch to work, the device MUST perform that sequence as the vendor documents it for this revision.

**Backlight**

- **FR-016**: The device MUST NOT dim the backlight. No on-screen control, serial command or setting may change its brightness (owner decision OD1).
- **FR-017**: The backlight MUST be switched fully on right after the silence step at every boot, and stay fully on.
- **FR-018**: The only backlight command the firmware can send MUST be "full on"; every dimming value MUST be rejected before it reaches the hardware. The screen and report state "backlight full, dimming disabled".

**Settings screen and Wi-Fi setup**

- **FR-019**: A settings screen, reached from the diagnostic screen, MUST offer the following, and nothing that implies sound:
  - start Wi-Fi setup
  - cancel Wi-Fi setup
  - forget stored Wi-Fi
  - the current Wi-Fi state
- **FR-019a**: On the device screen, "start Wi-Fi setup" and "forget Wi-Fi" MUST each ask for an explicit confirmation before acting, and cancelling MUST leave everything unchanged. Stress start and stop and cancelling an open hotspot act at once. Serial commands act without confirmation.
- **FR-020**: Wi-Fi setup MUST start a hotspot that is WPA2-protected and named per NET-02. Its password MUST be generated on the device per NET-02 and shown on the device screen.
- **FR-021**: The setup page reached through the hotspot MUST list visible networks and accept a network name (picked or typed) and a password (possibly empty).
- **FR-022**: Submitted details MUST be tried. On success they MUST be stored on the device and the hotspot closed. On failure they MUST be discarded and the failure shown on the device screen and the setup page.
- **FR-023**: With stored details, the device MUST join the network in the background after every boot and retry when the connection is lost, without blocking the screen.
- **FR-024**: The stored Wi-Fi password and the hotspot password MUST never appear in a log line, serial output, report or page response (SEC-04, NET-08). The hotspot password appears only on the device screen.
- **FR-025**: The stored Wi-Fi details MUST be kept where Slice 4 will read them, so that the full setup flow reuses them rather than replacing them.

**Evidence and reporting**

- **FR-026**: At the end of start-up, and on a serial report command, the device MUST print a report containing:
  - firmware version
  - board model and revision (configured, and detected where possible)
  - whether the full silence sequence (amplifier mute, then the buzzer-off command) ran first and was acknowledged. The report states this as one `audio-off` line that names no individual part, because the silence check (SIL-05) keeps part names out of everything but the silence function.
  - Wi-Fi state, with the network name but never the password
  - free memory
  - the pass / fail / skipped result of each self-test (display, touch, backlight, storage, Wi-Fi), with a short reason for any failure
- **FR-027**: A failed self-test MUST be shown in the alert colour and MUST NOT stop the other self-tests or the diagnostic screen.
- **FR-028**: Serial commands MUST include at least: report, stress on, stress off, Wi-Fi setup start and cancel, forget Wi-Fi, and help. There is no backlight command.

**Board profile and decisions**

- **FR-029**: The firmware MUST be built for DIS02170A revision V1.4. Every pin, timing, DIP-switch position and auxiliary-controller command it uses MUST come from vendor documentation for that model and revision, cited in the plan. Anything not documented goes to the owner as a question.
- **FR-030**: The feature MUST end with an owner checklist of numbered yes/no questions covering Stories 1 to 5 and 7 and the serial report. It MUST be stored in the feature directory with the owner's answers recorded there.
- **FR-031**: The display driver stack decision MUST be recorded in the feature directory with their evidence: the checklist answers and, if instability was seen, every mitigation tried and its outcome.
- **FR-032**: The firmware image size MUST be reported after each build and stay within the project budget. The size taken by the diagnostic mode MUST be reported separately, since it ships with the product.
- **FR-033**: In this branch, the project documents in `docs/` and the constitution MUST be updated to reflect the owner decisions recorded above.

### Key Entities

- **Board profile**: model and hardware revision. Chosen at build time and read by the report.
- **Self-test result**: test name, outcome (passed / failed / skipped), and a short reason.
- **Corner touch result**: target (top-left, top-right, bottom-left, bottom-right), passed / failed / not yet touched, and the error in pixels.
- **Stress counters**: completed cycles, elapsed time, and, for each activity (scan, encrypted request, storage write), the number of successes, errors and skips.
- **Stored Wi-Fi details**: network name and password, kept on the device until forgotten or factory reset. The password is write-only.
- **Hotspot identity**: name derived from the device, and a generated password kept on the device (NET-02).
- **Owner checklist**: numbered yes/no questions with recorded answers and date. It decides the display stack.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In a quiet room, with an ear close to the device, the owner hears no sound at power-on, at reset, with the backlight on, during Wi-Fi setup, or during ten continuous minutes of stress mode.
- **SC-002**: The owner confirms that every colour bar matches its label, the picture is the right way up, and the one-pixel border is visible on all four edges.
- **SC-003**: The vertical-line field shows no shift, tear, crawl or flicker, whether idle or during at least ten minutes of stress mode with the home network connected. At the end, the counters show at least one completed cycle per minute (the pass bar; the expected rate is about one cycle every 10 seconds), including successful encrypted requests.
- **SC-004**: All four corner targets are marked passed (within 10 pixels), as shown on screen and in the serial report.
- **SC-005**: The backlight is fully on at every boot, the report states "backlight full, dimming disabled", and the owner hears no whine from it.
- **SC-006**: The diagnostic screen appears within 5 seconds of power-on, with or without a network, as shown by the time the device logs and reports for it.
- **SC-007**: The owner completes Wi-Fi setup from a phone in under three minutes, and the device rejoins the network after a restart without setup.
- **SC-008**: No serial output or report captured during this feature contains the Wi-Fi or hotspot password.
- **SC-009**: The serial report shows model DIS02170A and revision V1.4, and the owner confirms it.
- **SC-010**: Every checklist question is answered and recorded, and the display stack decision is written down with its evidence.

## Assumptions

- In this slice the diagnostic screen stays until the device is restarted, and the settings screen is the only other screen. Later slices keep both as a hidden mode (FR-007a); the exact gesture is chosen in the plan.
- A failed self-test is shown in the alert colour and does not stop the others.
- Stress mode starts from an on-screen control and from a serial command.
- The boot, diagnostic and settings screens and the hotspot setup page are engineering screens, not product screens. Their wording is in English and they are outside `02-ui-spec.md`. Because they stay in the product firmware as a hidden mode, nothing on the product screens may hint at them. Slice 4 replaces the setup page and adds the product setup screen.
- The phone has to be pointed to the setup page by the address shown on screen. Automatic captive-portal opening, the QR code, location selection and classified error messages (NET-03 to NET-06, `02-ui-spec.md` §7, §10) stay in Slice 4.
- The owner has a USB cable, a serial console on the development machine, a phone, and a 2.4 GHz home network, and can stand in a quiet room next to the device.
- Out of scope: any product screen, location, prayer times, fonts for the product UI, and stored settings other than Wi-Fi details and the hotspot password.
