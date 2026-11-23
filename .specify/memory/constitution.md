<!--
Sync Impact Report
- Version change: 1.0.0 → 1.1.0 (MINOR: guidance materially changed for the hardware target)
- Reason: owner decisions recorded in specs/001-silent-board-bringup/spec.md (2026-11-20):
  the target board is the CrowPanel Advance 7.0" DIS02170A rev V1.4; the basic board is no
  longer a build target. Required by FR-033.
- Modified sections:
  IX. Resource Discipline: the image budget no longer refers to the 4 MB basic board; the
      2.9 MB image budget is kept as a discipline.
  Hardware and Product Constraints: target board and 16 MB partition layout.
- Principles I, II and III: unchanged (no owner approval needed under Governance).
- Added / removed sections: none
- Templates reviewed:
  ✅ .specify/templates/plan-template.md: no reference to the board; no edit needed
  ✅ .specify/templates/spec-template.md: no edit needed
  ✅ .specify/templates/tasks-template.md: no edit needed
  ✅ .specify/templates/checklist-template.md: no edit needed
  ✅ CLAUDE.md: names "Elecrow CrowPanel 7.0\", 800×480" only; still accurate, no edit needed
- Deferred TODOs: handover/docs/05-architecture.md §1, §5 and §10 are updated later in the
  same branch (FR-033, tasks.md Phase 10).
-->

# Namaz Vakti Panel Constitution

A silent ESP32-S3 touch display (Elecrow CrowPanel 7.0", 800×480) that shows Diyanet prayer times
for one district in Türkiye. This constitution binds every specification, plan, task and line of
code in this repository. `CLAUDE.md` and `handover/` give the detail; where they conflict with this
document, this document wins until it is amended.

## Core Principles

### I. Silence (NON-NEGOTIABLE)

The device never emits sound.

- No audio peripheral, audio library, tone or buzzer code may exist in the firmware. The I²S
  peripheral, DAC and any tone API are never initialised (SIL-01, SIL-03).
- The first action after reset, before any other peripheral, is a single platform function that
  puts the amplifier in shutdown, parks the I²S pins and, on boards that have one, holds the buzzer
  inactive (SIL-02). It is the only code that touches those pins.
- PWM runs only on the backlight pin and at 20 kHz or above. If the backlight is audible at any
  duty cycle, dimming is disabled and the backlight is held fully on (SIL-04).
- `tools/check_silence.py` runs before every firmware build and in the host test run, and fails the
  build on any banned token or audio dependency (SIL-05, `05-architecture.md` §9).
- No feature, setting, icon or text may be specified, planned or implemented that implies sound.
  A task that seems to need sound stops and goes to the owner.

Test: the silence check is green in every build, and the owner's listening check in each slice
that touches hardware is answered "no sound".

### II. Truthful Display (NON-NEGOTIABLE)

The screen never presents calculated, stale or unsynchronised information as official or current.

- Times whose source rank is below the Diyanet-derived rank are shown in the `calculated` state,
  marked approximate (DATA-05).
- A clock that is not valid per TIME-03 is shown as `no_time`, never as a guessed time.
- Every degraded condition has a defined visible state, evaluated in the order of
  `02-ui-spec.md` §6. No state may be silently skipped or merged.

Test: host tests exercise each system state from forced and real inputs; snapshots match
references 10 to 14.

### III. The Approved Design Is the Contract (NON-NEGOTIABLE)

`handover/docs/02-ui-spec.md` and the reference renders in `handover/design/screens/` define the
UI. They are implemented, not restyled, rearranged or re-worded.

- On-screen strings are Turkish and copied byte-for-byte, including U+2019 and U+2026.
- Acceptance uses the tolerances in `02-ui-spec.md` §12.
- Any deviation requires the owner's written approval, recorded in the feature's `spec.md`.

Test: every UI change is checked by rendering host-harness snapshots and reviewing them side by
side with the matching reference before the task is called done.

### IV. Offline First

The screen is always drawn from local state: engine output and the on-device store.

- Network and storage failures degrade the display in the defined steps and never block, freeze
  or crash it (UI-02, DATA-02).
- After any restart the device returns to the main screen from cache without user action (SYS-02).

Test: planner and state host tests cover every row of `04-data-sources.md` §12; device checklists
cover router-off operation.

### V. Verify, Do Not Assume

- Hardware facts (pins, timings, expanders, amplifier control, revisions) come from vendor
  documentation and schematics for the exact model and revision, or from the owner's inspection
  of the board, and are cited in the plan. Undocumented facts go to the owner as questions.
- API, SDK and library behaviour comes from current documentation and live calls, recorded in the
  feature's `research.md`. Versions and platforms are pinned to exact releases, never to moving
  links.
- Nothing about the physical device is reported as verified without the owner's confirmation or a
  log line that proves it.

Test: every plan cites its sources; `platformio.ini` contains no unpinned dependency.

### VI. Testable Core

- Time logic, parsing, validation, merging, calibration and refresh planning live in `lib/core` and
  `lib/providers`: standard C++ with no Arduino, ESP-IDF or LVGL headers. They build and pass
  their tests on the development machine with no hardware.
- Tests for `lib/core` are written before the code they test.
- `lib/ui` depends only on LVGL and `lib/core`, and the same sources render in the host UI harness
  and on the device. Only `src/platform` includes board, driver or SDK headers.

Test: `pio test -e native` passes in every build; layering is checked in review.

### VII. Owner-in-the-Loop Verification

Each feature that needs human eyes, ears or a phone defines a short numbered checklist of yes/no
questions for the owner. The feature is not done until the answers are recorded in its feature
directory. The implementer verifies everything it can (host tests, snapshots) before asking.

Test: the feature directory contains the checklist with recorded answers.

### VIII. Small, Private and Polite

- No cloud backend, accounts, telemetry, remote logging or third-party update checks (SEC-05).
- TLS certificate verification is on in every build, including development builds (SEC-01).
- Secrets (Wi-Fi passwords, access-point password, settings PIN, API credentials, tokens) never
  appear in logs, API responses or the repository (SEC-04, NET-08).
- External services are called rarely, one request at a time, only the hosts in
  `04-data-sources.md` §11, with the `User-Agent` naming the project and version (SEC-02, SEC-03).

Test: request-budget tests in the planner; secret review of logs and status interface.

### IX. Resource Discipline

- The firmware is a single application image without over-the-air update; the image stays at
  or below 2.9 MB, and its size is reported after every build.
- No blocking I/O on the UI thread (UI-04). No steady-state heap growth (SYS-04).
- Flash writes are rare (SYS-03) and atomic (DATA-07).

Test: the `check` command reports image size against the budget and fails over it; power-loss
tests run on the host with a failing fake file system.

### X. Simplicity and Scope

- One location in Türkiye, Turkish UI, and the features in `handover/docs/01-product-brief.md`;
  nothing more.
- Prefer the simplest design that satisfies the numbered requirements in
  `handover/docs/03-functional-spec.md`.
- Specs, tasks, tests and commit messages cite requirement ids (for example `ENG-03`, `DATA-05`).

Test: every task in `tasks.md` names at least one requirement id or spec section.

## Hardware and Product Constraints

- Target: CrowPanel Advance 7.0", DIS02170A, hardware revision V1.4, ESP32-S3-WROOM-1-N16R8
  (16 MB flash, 8 MB octal PSRAM). The basic model (DIS08070H) is not a build target; adding it
  back is a new slice.
- Partition layout (16 MB flash, `specs/001-silent-board-bringup/plan.md`): NVS 24 kB, PHY 4 kB,
  application 3.0 MB, LittleFS 4 MB; the rest is unallocated. Fonts budget 300 kB.
- Code structure and layering per `05-architecture.md` §4.
- Code, comments, commit messages and logs are in English; UI strings are in Turkish, kept in one
  place.

## Development Workflow and Quality Gates

- Work follows Spec Kit, one slice per feature branch, in the order of `handover/docs/06-roadmap.md`,
  using the prompts in `handover/prompts/`. A slice does not start before the previous one meets its
  "done when".
- Definition of done for every slice (`05-architecture.md` §11): `/speckit-analyze` has no open
  critical findings; host tests, silence check and firmware build pass within budget; affected UI
  snapshots are reviewed; the owner checklist is recorded; `/speckit-converge` reports converged;
  README and handover documents are corrected with anything learned.
- Commits are small and build. When a handover document turns out to be wrong, it is corrected in
  the same branch.

## Governance

- This constitution supersedes other practices in this repository. Every plan's Constitution Check
  verifies compliance with all ten principles; violations are justified in the plan's Complexity
  Tracking or the plan is changed.
- Changes to Principles I, II and III require the owner's explicit approval, recorded in the
  amending commit or the feature spec.
- Any principle change updates dependent templates (`.specify/templates/*`) and guidance
  (`CLAUDE.md`) in the same change, with a Sync Impact Report at the top of this file.
- Versioning follows semantic versioning: MAJOR for removing or redefining a principle, MINOR for
  adding a principle or materially expanding guidance, PATCH for clarifications.

**Version**: 1.1.0 | **Ratified**: 2026-11-20 | **Last Amended**: 2026-11-23
