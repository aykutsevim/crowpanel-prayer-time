# Feature Specification: Real prayer times

**Feature Branch**: `feat/006-prayer-data`

**Created**: 2026-11-24

**Status**: Draft

**Input**: User description: "Slice 6 on a new feat branch" (Slice 6 of `docs/specification/06-roadmap.md`; brief in `docs/development/prompts/slice-06-prayer-data.md`)

## Context

Until now the main screen has shown the real clock with the `no_data` state, and the sample day only on a development command. The product's purpose is to show the times Diyanet publishes for the owner's district, and to keep showing them when the network is not there. That needs a trustworthy fetch and a store that survives restarts and power cuts.

This feature adds the first Diyanet-derived source and the on-device store. Scheduled refreshes, the second mirror, calculated times and the `calculated` and `cache_ending` states come in Slice 7. Here a refresh happens after setup, at start when needed, and on a development command.

Sources: `docs/specification/03-functional-spec.md` §6 and §7; `docs/specification/04-data-sources.md` §1–§4, §7, §8, §10, §13; `docs/specification/02-ui-spec.md` §6 (states `ok`, `offline`, `no_data`), §7.4 (overlay) and references 01, 10, 14.

Requirements covered: DATA-01, DATA-02, DATA-06, DATA-07, DATA-08 (store hand-over on a location change, exercised through start-over setup), SEC-01 to SEC-03, LOC-01, and the overlay rows `Vakit kaynağı`, `Son güncelleme`, `Kayıtlı son gün` of 02 §7.4.

What earlier slices provide: the stored district (Slice 4; for districts without their own Diyanet times the centre's id, OD4-3), a valid clock and certificate checks with dates (Slice 5), the prayer-time engine (Slice 2) and the main screen (Slice 3).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Real times (Priority: P1)

As the owner, shortly after setup the six times on screen are the ones Diyanet publishes for my district today, and the Hijri date is the one published with them.

**Why this priority**: It is the product's purpose.

**Independent Test**: Compare the device's times for three dates in the stored range with Diyanet's website for the district.

**Acceptance Scenarios**:

1. **Given** a completed setup with Wi-Fi and a valid clock, **When** the first refresh succeeds, **Then** within about twenty seconds the main screen leaves `no_data`, shows today's six times and the Hijri date of the record, and the state is `ok` with `Son güncelleme HH:MM`.
2. **Given** the stored times, **When** three dates in the stored range (today and two others) are compared with `https://namazvakitleri.diyanet.gov.tr/` for the district, **Then** all six times and the Hijri date are equal.

---

### User Story 2 - Kept on the device (Priority: P1)

As the owner, the device stores the times for as far ahead as the source provides (up to a year) and shows them from that store. After a restart without Wi-Fi it shows the same times as soon as it knows the time of day.

**Why this priority**: The screen must not depend on the network once the data is there (DATA-02, constitution IV).

**Independent Test**: With the router off, restart the device; the board clock (or the development clock) gives the time.

**Acceptance Scenarios**:

1. **Given** a stored range covering today, **When** the device restarts with the router off and the clock is valid, **Then** today's stored times are shown at once.
2. **Given** that situation, **When** Wi-Fi has been down for more than ten minutes, **Then** the state is `offline` (reference 10) and the times are unchanged.

---

### User Story 3 - Never half-written (Priority: P1)

As the owner, a power cut at any moment, including during a refresh, never leaves the device with damaged or mixed data: after a restart it has either the previous data or the new data. A damaged store is detected and rebuilt from the network.

**Why this priority**: A corrupt store would show wrong times as official (constitution II) or nothing at all.

**Independent Test**: A host test fails the write at every step; on the device a development command corrupts the stored file.

**Acceptance Scenarios**:

1. **Given** a store and a refresh in progress, **When** the write is interrupted at any step, **Then** after a restart the store is exactly the previous version or exactly the new one.
2. **Given** a store whose contents were damaged, **When** the device starts, **Then** it detects the damage, does not show the damaged data, and rebuilds the store from the network when Wi-Fi is up.

---

### User Story 4 - Nothing wrong gets in (Priority: P1)

As the owner, data that is malformed, for another place, for the wrong dates or internally inconsistent is rejected as a whole and the previous data stays.

**Why this priority**: Wrong times shown as official are the worst failure of this product.

**Independent Test**: Automated tests with altered copies of recorded real answers, one per rejection reason.

**Acceptance Scenarios**:

1. **Given** an answer whose record breaks the order of the six times, has a duplicate or far-away date, is for another district, is empty although marked successful, or ends in the middle, **When** it is received, **Then** the whole answer is rejected, the reason is logged, and the store is unchanged.

---

### User Story 5 - Honest when empty (Priority: P2)

As the owner, if there is no stored record for today, the screen shows the `no_data` state, and the status panel says what the device knows. When data arrives the screen switches to normal by itself.

**Why this priority**: The truthful empty state already exists; this story keeps it correct with real data.

**Independent Test**: Erase the store with Wi-Fi off; then switch Wi-Fi on and refresh.

**Acceptance Scenarios**:

1. **Given** no record for today, **When** the screen is drawn, **Then** it matches reference 14.
2. **Given** that state, **When** a refresh succeeds, **Then** the screen switches to the normal main screen without a restart.

---

### User Story 6 - Visible provenance (Priority: P2)

As the owner, the status panel shows the source of today's times, when they were last updated, and the last date stored.

**Why this priority**: The owner can see why the times are trustworthy, or not.

**Independent Test**: Open the overlay after a refresh.

**Acceptance Scenarios**:

1. **Given** stored times, **When** the overlay is opened, **Then** `Vakit kaynağı` shows `Diyanet (imsakiyem)` (02 §9.5), `Son güncelleme` the date and time of the last successful refresh, and `Kayıtlı son gün` the last stored date, in the formats of 02 §7.4.
2. **Given** no stored times, **Then** those three rows show the grey dash of spec 004.

---

### Edge Cases

- **The source is unreachable at the first setup**: the device stays in `no_data`, retries once after a minute, and can be refreshed by the development command; the scheduled retries are Slice 7.
- **A very large answer**: the device never holds the whole answer in memory; it reads it record by record and keeps only the fields it needs.
- **The answer ends in the middle**: rejected as a whole (User Story 4).
- **The source has no data past 31 December**: the store ends where the source ends. In late November 2026 the source already offered 2027 (research R1); if a year is published late, as in January 2026, there is no record for today until it is, and the screen shows `no_data` (the second mirror and the calculated times are Slice 7).
- **The district has been changed through start-over setup**: the old district's store is discarded only after the first record for the new district has been stored (DATA-08); until then the old district's times are not shown as the new district's.
- **Today's record exists but tomorrow's does not**: today is shown normally; after Yatsı the night period has no next İmsak, which the engine already handles (Slice 2).
- **The clock is valid but wrong by a day**: the device shows the record for the date it believes; the network time of Slice 5 corrects the clock, and the screen follows at once. A batch is not rejected for a one-day clock error (the date window allows two days behind).
- **A refresh while the store is being read for the screen**: the screen always sees one complete version.
- **The board's power is cut while a refresh writes**: User Story 3.

## Requirements *(mandatory)*

### Functional Requirements

**Fetching**

- **FR-001**: The device MUST fetch the day records for the stored district from the first Diyanet-derived source of 04 §3.1, from today as far ahead as the source offers, up to 366 days (DATA-01). (Checked live on 2026-11-24: the source already offers 2027; research R1.)
- **FR-002**: A fetch MUST use an encrypted connection with full certificate checks (SEC-01), contact only listed hosts (SEC-02), send no identifier of the owner or the device beyond what the protocol needs, and name the project and version in the user agent (SEC-03). A fetch MUST start only when the clock is valid.
- **FR-003**: A fetch MUST read the answer record by record and keep only the fields it needs; the whole answer is never held in memory.
- **FR-004**: Fetches MUST run one at a time and never block the screen.
- **FR-005**: In this feature a refresh MUST run after a successful setup, at start when the store does not cover today or is empty, and on a development command; after a failure it MUST be retried once after one minute.

**Validating**

- **FR-006**: A fetched batch MUST be rejected as a whole, with the reason logged, when any record breaks ENG-01, a date is missing, malformed, duplicated, more than 400 days ahead or more than 2 days behind today, the batch is empty or has no date from today on, or the place in the answer is not the stored district (DATA-06, 04 §7; the comparison with calculated times arrives in Slice 7).
- **FR-007**: A rejected batch MUST leave the store unchanged.

**Storing**

- **FR-008**: Accepted records MUST be merged into the store by the rules of 04 §4 (one record per date; a higher or equal rank with a later fetch replaces; records before yesterday pruned) and stored on flash with the district, a format version, a checksum and the time of the last successful refresh.
- **FR-009**: A store write MUST survive a power cut at any moment: after a restart the store is the previous version or the new one, never a mix (DATA-07). A store with a bad checksum or an unknown format version MUST NOT be used; it is rebuilt by the next refresh.
- **FR-010**: On a location change the old district's store MUST be kept until the first record of the new district has been stored, and MUST NOT be shown for the new district meanwhile (DATA-08).
- **FR-011**: No storage write MAY overlap a Wi-Fi scan (Slice 1 rule).

**Showing**

- **FR-012**: The engine MUST take yesterday's, today's and tomorrow's records from the store; the Hijri date shown MUST be the record's.
- **FR-013**: The system state MUST be chosen by 02 §6 from real inputs for `no_time`, `no_data`, `offline` and `ok`: `no_data` when the store has no record for today; `offline` when Wi-Fi has been down for more than ten minutes and today is stored; otherwise `ok`. `Son güncelleme` MUST use the last successful refresh (`HH:MM` when it was today, else `DD.MM`). `calculated` and `cache_ending` stay forced-only until Slice 7.
- **FR-014**: The status overlay MUST fill `Vakit kaynağı` (02 §9.5 name of today's record's source), `Son güncelleme` (`DD.MM.YYYY HH:MM`) and `Kayıtlı son gün` (`DD.MM.YYYY`) from the store; with no store they show the dash.
- **FR-015**: When a refresh changes the stored data, the screen MUST follow within one second, without a restart.

**Development tools and logging**

- **FR-016**: Development serial commands MUST let a developer refresh now, print a store summary (district, dates, count, source, checksum state, last refresh), corrupt the stored file (development builds only), and erase the store (SYS-06).
- **FR-017**: Each refresh attempt MUST be logged as one line with provider, result, number of records and duration; each rejection with its reason (SYS-05).
- **FR-018**: The device MUST stay silent (SIL-01).

### Key Entities

- **Day record**: date, six times (minutes after midnight), Hijri date, source (provider id and rank). The fetch time is kept once per store (the last successful refresh), not per record.
- **Store**: one district's records (at most about 400), format version, district id, checksum, time of the last successful refresh.
- **Fetched batch**: the records of one answer, with the place it names; accepted or rejected as a whole.
- **Refresh attempt**: provider, start, duration, result (stored N records, or the rejection or failure reason).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: After setup, real times appear on the main screen within 20 seconds of the device connecting to the owner's network.
- **SC-002**: For three dates in the stored range, all six times and the Hijri date equal Diyanet's website for the district (owner check).
- **SC-003**: After a restart with the router off and a valid clock, today's stored times appear within 2 seconds of the start, and the state becomes `offline` after ten minutes without Wi-Fi.
- **SC-004**: In the host test that interrupts the store write at every step, 100 % of the outcomes are exactly the old or exactly the new store.
- **SC-005**: Every rejection reason of FR-006 is covered by at least one automated test on altered copies of recorded real answers, and each leaves the store unchanged.
- **SC-006**: After the store is corrupted and the device restarts with Wi-Fi, the device shows `no_data` (not the damaged data) and recovers real times by itself within one minute of the connection.
- **SC-007**: After the store is erased with Wi-Fi off, the screen matches reference 14.
- **SC-008**: The device makes no sound (owner confirmation).

## Assumptions

- The first Diyanet-derived source is İmsakiyem (04 §3.1); which of its two interfaces is primary is decided in the plan after the live check of 04 §13.
- The district id fetched is the stored one; for a district without its own Diyanet times it is the centre's id (OD4-3), so the answer names the centre, which matches the stored id.
- Wi-Fi being "down" for the `offline` state is counted from the moment the device lost its connection in running mode.
- Rank: Diyanet-derived records have rank 40 (04 §2); the merge rules are implemented in full although only one source exists.
- Until Slice 7 there is no weekly schedule: a store covering today is not refreshed again unless the developer asks or the device restarts with today uncovered.

## Out of Scope

- Scheduled refreshes, the second mirror and other sources, calculated times, coordinates, the `calculated` and `cache_ending` states (Slice 7).
- The official Diyanet API; the settings page (Slice 8).
