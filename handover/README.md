# Namaz Vakti Panel: handover package

Everything needed to build the firmware with Claude Code and GitHub Spec Kit: the product brief, the approved design at pixel level, the full specification, verified data sources, the architecture, a ten-slice roadmap, and copy-paste prompts for every Spec Kit step of every slice.

Prepared on 17 November 2026, revised on 20 November 2026. Facts about third-party services and the board were checked against their public documentation on those dates; each document says what still needs a live check. Changes in the revision are listed at the end of this file.

## What is in here

```
CLAUDE.md                         working rules Claude Code reads on every session
handover/
  README.md                       this file
  docs/
    01-product-brief.md           what the product is, goals, non-goals, hard constraints, glossary, open questions
    02-ui-spec.md                 Design A at pixel level: every screen, state and string
    03-functional-spec.md         numbered requirements: modes, time, engine, Wi-Fi, data, settings, silence, reliability
    04-data-sources.md            the APIs, field mappings, cache, refresh policy, automatic fallback
    05-architecture.md            board facts and pins, recommended stack, code structure, testing, silence enforcement
    06-roadmap.md                 the ten slices, what each delivers and when it is done
  prompts/
    00-bootstrap.md               repository, Spec Kit and constitution
    slice-01-…md  to  slice-10-…md    one file per slice
  design/
    reference.html                interactive reference of every screen (open in a browser)
    tokens.json                   colours and geometry, machine-readable
    screens/*.png                 17 reference renders, exactly 800×480
```

## Decisions already made

| Topic | Decision |
|---|---|
| Design | Design A, “Şerit”: dark, one 24-hour colour strip, countdown as the hero. Bell icons from the earlier mock-up were removed because the device is silent. |
| Sound | None, ever. Enforced in hardware set-up, in a build check and in the constitution. |
| Data | Diyanet-published times through two community mirrors (İmsakiyem first, emushaf second), optionally Diyanet’s own API with the owner’s credentials; then Aladhan’s Diyanet method; then on-device calculation. Calculated times are labelled approximate. |
| Cache | Day records on flash, normally through the end of the calendar year; weekly refresh at night; atomic writes. |
| Setup | The device opens its own WPA2 access point, shows a QR code and password, and serves a captive page where the owner picks the home Wi-Fi and the province and district. The place list is bundled, so setup needs no internet. |
| Time | SNTP, with an HTTP `Date` header fallback. UTC+3 by default. |
| Scope | One location in Türkiye, Turkish UI. |
| Board | Default target CrowPanel 7.0" basic (DIS08070H, 4 MB flash). The Advance model is a second build profile. |
| Tooling | PlatformIO, host-testable core, a host UI harness that renders snapshots so the agent can check its own UI work. |

## Before starting

The owner needs:

1. The exact board model and revision. Look at the back label and the PCB: `DIS08070H` (basic) or `DIS02170A` (Advance), and a revision such as `V2.2` or `V3.0`. Slice 1 asks for this.
2. A development machine with Git, Python 3.11 or newer, `uv`, PlatformIO Core, and Claude Code installed and signed in (`https://docs.claude.com/en/docs/claude-code/overview`).
3. A USB-C data cable, a 2.4 GHz Wi-Fi network, and a phone.
4. Answers, when convenient, to the open questions at the end of `docs/01-product-brief.md`. Defaults are in place for all of them.

## Getting started

1. Unzip the package. The folder `namaz-vakti-panel/` becomes the repository root; it contains `CLAUDE.md` and `handover/`.
2. Open a terminal in that folder and start Claude Code.
3. Open `handover/prompts/00-bootstrap.md` and follow it. It initialises Git and Spec Kit, writes the constitution and creates the project skeleton.
4. Work through `slice-01` to `slice-09` in order. `slice-10` is optional.

## The loop for every slice

Each slice file contains the same numbered steps. A step that starts with a slash command is pasted into Claude Code as it is.

| Step | Command | What happens |
|---|---|---|
| 1 | `/speckit.specify …` | creates the feature branch and `spec.md` from the prompt |
| 2 | `/speckit.clarify …` | Claude asks up to five questions; answer them or accept the defaults suggested in the slice file |
| 3 | `/speckit.plan …` | research, technical plan, data model, contracts |
| 4 | `/speckit.tasks` | ordered task list |
| 5 | `/speckit.analyze` | consistency check; fix findings at their source and repeat until clean |
| 6 | `/speckit.implement …` | builds the slice, in the stages the slice file suggests |
| 7 | owner checklist | the owner checks the device and answers the yes/no questions |
| 8 | `/speckit.converge` | confirms nothing was missed; repeat step 6 if it appends tasks |
| 9 | merge | merge the feature branch, tag if the slice says so |

Spec Kit command spelling depends on its version and mode. If Claude Code lists the commands as `/speckit-specify` instead of `/speckit.specify`, use that spelling throughout.

Three habits that make this work:

- **Start each slice in a fresh Claude Code session.** The specification, plan and `CLAUDE.md` carry the context; a long session does not need to.
- **Let Claude verify what it can before asking you.** Host tests and rendered snapshots come first. You are asked only for things that need eyes, ears or a phone.
- **When something in these documents turns out to be wrong** (an API changed, a pin differs on your revision), have Claude correct the document in the same branch. The handover should stay true.

## Reading order for a new contributor

`01-product-brief.md`, then look at `design/screens/01-main-ogle-1432.png` and open `design/reference.html`, then `06-roadmap.md`. Read the other documents when a slice calls for them.

## Revision history

**20 November 2026**

- `04-data-sources.md`: the İmsakiyem API description was read in full. It is now version 1.1.0 and adds version 2 endpoints: a times endpoint that returns up to 366 days in one request, and a district-detail endpoint with coordinates and time zone. The refresh plan, the coordinate lookup and the place-list generator now prefer these, with the version 1 calls as fallback. Added the known rough edges (paging metadata differs from the description; empty result for unknown ids). Aladhan’s `timezonestring` parameter and its date-range endpoint confirmed from client libraries. Reported emushaf rate limit added.
- `05-architecture.md`: the Advance model is now described from the vendor page and reviews: 16 MB flash, IPS panel, on-board battery-backed clock, and a buzzer in addition to the amplifier. Consequences for silence, time-keeping and flash are spelled out. The Arduino-core situation in PlatformIO is stated (official platform: 2.x core; 3.x through the community `pioarduino` platform).
- `03-functional-spec.md`, `01-product-brief.md`, prompts for Slice 0, 1, 6 and 10: adjusted to match.
