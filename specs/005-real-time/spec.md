# Feature Specification: Real time

**Feature Branch**: `feat/005-real-time`

**Created**: 2026-11-24

**Status**: Draft

**Input**: User description: "Slice 5 on a new feat branch" (Slice 5 of `docs/specification/06-roadmap.md`; brief in `docs/development/prompts/slice-05-time-sync.md`)

## Context

Every value on the screen depends on the current local time. Since Slice 3 the device reads the time from the board's battery-backed clock at every start (owner decision OD3-1). That clock was set once by hand over the serial port; it drifts, and nothing corrects it. Since Slice 4 the device joins the owner's Wi-Fi. This feature makes the network the source of truth: the device gets the time from time servers as soon as Wi-Fi is up, keeps it accurate, writes it back to the board's clock, and is honest whenever it cannot know the time.

Sources: `docs/specification/03-functional-spec.md` §2 (TIME-01 to TIME-08), `docs/specification/02-ui-spec.md` §6 (state `no_time`) and reference `13-state-time-not-set.png`, `docs/specification/04-data-sources.md` §11 and §12.

Requirements covered: TIME-01 to TIME-07, the writing half of TIME-08 (moved here by OD3-1), and the system states `no_time` and `no_data` chosen from real inputs.

### Differences from the brief

The brief for this slice was written before the board's clock was known (it says "the board has no battery-backed clock") and before Slice 3's truthful start screen. Two of its statements are therefore replaced, following decisions already recorded:

- **The board's clock counts as a time source** (OD3-1, TIME-03, TIME-08). After a power cut without Wi-Fi the device shows the time from that clock if it is valid, instead of `no_time`. `no_time` appears only when neither the network nor a valid board clock has given a time.
- **No sample day on the main screen** (OD3-2, constitution II). Until real prayer data arrives in Slice 6, the main screen shows the real clock and date with the `no_data` state (reference 14), as it does since Slice 3. The sample day appears only when a fixture is called up over the serial port.

## Owner decisions

- **OD5-1 (2026-11-24, during planning): certificate dates are checked; the time fallback uses plain HTTP.** The TLS library ignored certificate validity dates until now (the framework's default; the chain and host name were checked). From this feature on every encrypted request also checks the dates, and is made only when the clock is valid. The fallback of FR-002 therefore reads only the `Date` header of an unencrypted answer from a host the device already contacts; the value is plausibility-checked and replaced by network time as soon as that arrives (research R2, R3).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Correct clock (Priority: P1)

As someone in the room, the time and date on the screen are right to the second.

**Why this priority**: Every prayer time, countdown and period on the screen is judged against this clock.

**Independent Test**: Start the device with Wi-Fi available and compare the screen with a reference clock showing seconds, at start and again after 24 hours.

**Acceptance Scenarios**:

1. **Given** Wi-Fi is available, **When** the device starts, **Then** within ten seconds of the Wi-Fi connection the screen shows the real local time, synchronised from the network.
2. **Given** the device has synchronised, **When** it is compared with a reference clock at any moment over 24 hours, **Then** it is never more than one second off.
3. **Given** the clock is running, **When** a second boundary passes, **Then** the displayed seconds change within 100 ms of it.
4. **Given** the device has synchronised, **Then** the board's clock is set from the network time, so the next start without Wi-Fi shows the right time.

---

### User Story 2 - Honest waiting (Priority: P1)

As the owner, whenever the device does not know the time, the screen says so instead of showing a wrong clock.

**Why this priority**: A wrong clock shown as right breaks the truthful-display principle (constitution II).

**Independent Test**: Start the device with the board's clock invalid and the router off; then switch the router on.

**Acceptance Scenarios**:

1. **Given** the board's clock is not valid and no network time has been received since power-on, **When** the screen is drawn, **Then** it shows the `no_time` state of 02 §6 exactly as reference 13: `--:--` clock, the texts of the UI spec, no now marker, countdown dashes.
2. **Given** the `no_time` state, **When** network time arrives, **Then** the screen switches to the real time within one second.
3. **Given** the board's clock is valid and the router is off, **When** the device starts, **Then** it shows the time from the board's clock (OD3-1) and corrects it as soon as network time arrives.

---

### User Story 3 - Stubborn about getting the time (Priority: P2)

As the owner, if the usual time servers cannot be reached on my network, the device still gets the time by another route and keeps trying the normal one.

**Why this priority**: Some networks block time servers; the device must still work there.

**Independent Test**: Block network time on the router (or simulate it in development) and watch the clock become valid.

**Acceptance Scenarios**:

1. **Given** Wi-Fi is up but no time server answers for 60 s, **When** the fallback runs, **Then** the clock becomes valid within about a minute and is within two seconds of real time.
2. **Given** the clock was set by the fallback, **When** a time server answers later, **Then** network time replaces it.

---

### User Story 4 - Midnight (Priority: P2)

As someone in the room, at midnight the date changes and the screen carries on without a glitch.

**Why this priority**: The day changes every night; a glitch would be seen every night.

**Independent Test**: Run the clock across midnight at real speed on the device, and fast on the development machine.

**Acceptance Scenarios**:

1. **Given** the real clock at 23:59:59, **When** it passes midnight, **Then** the date line shows the new date and weekday, the night period continues, and nothing flickers or jumps.

---

### User Story 5 - Time zone (Priority: P3)

As the owner in Türkiye I never have to set a time zone. A stored setting allows another zone and takes effect at once.

**Why this priority**: Türkiye has one fixed zone; the setting is for completeness and for the settings page of Slice 8.

**Independent Test**: Change the stored zone with a development command and watch the clock change at once; restart and see it kept.

**Acceptance Scenarios**:

1. **Given** no zone is stored, **Then** the device uses `<+03>-3` (Türkiye, no daylight saving).
2. **Given** a different valid zone is stored, **Then** the displayed time follows it at once, without a restart, and after a restart.
3. **Given** an invalid zone string, **Then** it is refused and the current zone stays.

---

### Edge Cases

- **Time servers reachable but answering late**: an answer that arrives late is still used; the 60 s fallback timer counts from Wi-Fi coming up, not from each request.
- **A resync moves the clock by minutes, forwards or backwards**: applied immediately; the screen recomputes everything and the countdown may jump (TIME-05). Each correction is logged with its size.
- **Wi-Fi lost for days after a successful sync**: the clock keeps running from the device's own timekeeping and stays valid; it resyncs when Wi-Fi returns.
- **A time earlier than the firmware's build time**: never accepted, from any source (TIME-03); the board's clock is not written with it.
- **The board's clock holds a wrong but plausible time**: shown until network time arrives, then corrected and rewritten.
- **The board's clock fails to answer or is invalid**: the device waits in `no_time` for network time; no error repeats in the log.
- **The fallback's answer is malformed or missing the date header**: ignored; the next attempt follows.
- **The simulated clock from the developer commands is on**: it overrides real time on the screen; network sync continues in the background and is used again when the simulated clock is switched off.
- **Midnight during a resync, or a resync that crosses midnight**: the date follows the corrected clock; the day changes once.
- **Wi-Fi comes up during setup** (Slice 4): time sync starts as soon as the device is connected to the owner's network, at the end of a successful setup attempt or at the first join in running mode; it never runs on the device's own network (analysis I2).

## Requirements *(mandatory)*

### Functional Requirements

**Getting the time**

- **FR-001**: Once connected to the owner's network, the device MUST obtain the time from network time servers of at least three different operators, and MUST resynchronise at least every six hours (TIME-01).
- **FR-002**: If no time server has answered within 60 s of the Wi-Fi connection, the device MUST set the clock from the date of an answer from a well-known web host, accurate to ±2 s, and MUST keep trying the time servers; a later server answer replaces it (TIME-02).
- **FR-003**: Time MUST count as valid only after a successful network sync since power-on, or from the board's clock when that clock passes its integrity and range checks; and in every case only if it is not earlier than the firmware's build time (TIME-03, OD3-1).
- **FR-004**: After every successful network sync the device MUST write the time to the board's clock (TIME-08 writing half). A time that is not valid MUST NOT be written.
- **FR-005**: A sync that moves the clock MUST be applied immediately and everything derived from the time recomputed; the countdown may jump (TIME-05).
- **FR-006**: Between syncs the clock MUST keep running from the device's own timekeeping and stay valid while Wi-Fi is down.

**Showing the time**

- **FR-007**: The system state MUST be chosen in the order of 02 §6 from real inputs for the states this feature can produce: `no_time` when time is not valid, otherwise `no_data` while no prayer data is stored (Slice 6). The other states stay reachable only through the development command until their features arrive.
- **FR-008**: The `no_time` screen MUST match reference 13 (02 §6 row 1).
- **FR-009**: The displayed second MUST change within 100 ms of the real second boundary (TIME-07).
- **FR-010**: At local midnight the date line MUST change to the new date and weekday and the screen MUST continue without a visible glitch (TIME-06).
- **FR-011**: Everything that needs the current time MUST obtain it from one place, so the simulated clock of the development commands overrides it everywhere and tests can supply any time.

**Time zone**

- **FR-012**: The time zone MUST be a POSIX zone string stored in settings, default `<+03>-3`; a change MUST take effect at once and survive a restart; an invalid string MUST be refused (TIME-04).
- **FR-013**: Until the settings page exists (Slice 8), a development serial command MUST let the zone be read and set.

**Logging and development tools**

- **FR-014**: Each sync MUST be logged as one line with its source (network time server, fallback web host, board's clock) and the correction applied in milliseconds or seconds (SYS-05).
- **FR-015**: The serial report MUST show whether the time is valid, its source, and the age of the last sync.
- **FR-016**: The device MUST stay silent (SIL-01); certificate checks MUST stay on for every encrypted connection (SEC-01). If the fallback cannot use an encrypted connection because certificates cannot be checked without a valid clock, the reason MUST be recorded in the plan, and the fallback MUST be limited to reading a date.

### Key Entities

- **Clock reading**: the current time, whether it is valid, its source (network, fallback, board's clock, simulated) and when it was last synchronised.
- **Sync event**: source, time of the event, correction applied (signed), result (applied, refused as earlier than build time, malformed).
- **Time zone setting**: a POSIX zone string, default `<+03>-3`, stored with the other settings.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: With Wi-Fi available, the screen shows synchronised time within 10 s of the Wi-Fi connection.
- **SC-002**: Over 24 hours the displayed time is never more than 1 s off a reference clock.
- **SC-003**: The displayed second changes within 100 ms of the real second boundary in at least 99 % of seconds over 10 minutes (measured from the serial log).
- **SC-004**: With time servers blocked, the clock becomes valid within 75 s of the Wi-Fi connection and is within 2 s of real time.
- **SC-005**: With the board's clock invalid and no Wi-Fi, the screen matches reference 13 within the tolerances of 02 §12 for the whole wait.
- **SC-006**: Across midnight, at real speed on the device and fast on the development machine, the date changes exactly once and no frame shows a mixed old and new day.
- **SC-007**: A time-zone change shows on the screen within 1 s and is kept after a restart.
- **SC-008**: The device makes no sound (owner confirmation).

## Assumptions

- The board's clock reading and its checks come from Slice 3 (`specs/003-main-screen`, research R1); this feature adds writing it after a sync.
- "Well-known web host" for the fallback is one of the hosts already contacted by the device (04 §11), so no new host is introduced (SEC-02). Which host and why an unencrypted request is needed are decided in the plan.
- Time sync runs only in running mode on the owner's network (Slice 4 modes); in setup mode there is no internet.
- The owner may not be able to block time servers on the router; the fallback is then verified on the development machine and with a development command that disables the time servers, and the owner checklist says so.
- The time zone has no input field until the settings page (Slice 8).

## Out of Scope

- A battery-backed clock module other than the board's own; automatic time-zone detection; daylight-saving rules beyond what a POSIX zone string expresses.
- Prayer data from the network (Slice 6); the `calculated`, `cache_ending`, `offline` and `ok` states from real inputs (Slices 6 and 7).
- The settings page and its time-zone field (Slice 8).
