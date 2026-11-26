# Documentation

Everything that defines the Namaz Vakti Panel: what it is, how it looks, what it must do, where its data comes from, how it is built and in which order. Code and tests live outside this folder; Spec Kit feature records live in [`../specs/`](../specs/).

## Contents

```
docs/
  README.md                      this index
  specification/
    01-product-brief.md          what the product is: goals, non-goals, hard constraints, glossary, open questions
    02-ui-spec.md                the approved design (Design A, “Şerit”) at pixel level: every screen, state and string
    03-functional-spec.md        numbered requirements: modes, time, engine, Wi-Fi, data, settings, silence, reliability
    04-data-sources.md           the APIs, field mappings, cache, refresh policy and automatic fallback
    05-architecture.md           board facts and pins, stack, code structure, testing, silence enforcement
    06-roadmap.md                the ten slices plus Slice 11 (both boards), what each delivers and when it is done
  design/
    reference.html               interactive reference of every screen (open in a browser)
    tokens.json                  colours and geometry, machine-readable
    screens/*.png                17 reference renders, exactly 800×480
  decisions/
    README.md                    index of recorded technical decisions
    0001-platform.md             PlatformIO platform and Arduino core
  development/
    workflow.md                  how a slice is built with Spec Kit and Claude Code
    prompts/
      00-bootstrap.md            repository, Spec Kit and constitution (done in Slice 0)
      slice-01-…md … slice-11-…md  the prompts for every Spec Kit step of every slice
```

Requirement ids (for example `SIL-02`, `DATA-05`) are defined in `specification/03-functional-spec.md` and cited everywhere else. The binding principles are in the constitution, [`../.specify/memory/constitution.md`](../.specify/memory/constitution.md); working rules for Claude Code are in [`../CLAUDE.md`](../CLAUDE.md).

## Reading order for a new contributor

1. [`specification/01-product-brief.md`](specification/01-product-brief.md)
2. [`design/screens/01-main-ogle-1432.png`](design/screens/01-main-ogle-1432.png), then open [`design/reference.html`](design/reference.html) in a browser
3. [`specification/06-roadmap.md`](specification/06-roadmap.md)
4. [`development/workflow.md`](development/workflow.md)

Read the other documents when a slice calls for them.

## Decisions already made

| Topic | Decision |
|---|---|
| Design | Design A, “Şerit”: dark, one 24-hour colour strip, countdown as the hero. Bell icons from the earlier mock-up were removed because the device is silent. |
| Sound | None, ever. Enforced in hardware set-up, in a build check and in the constitution. |
| Backlight | Always fully on; no dimming (owner decision OD1, 2026-11-20, spec 001). |
| Data | Diyanet-published times through two community mirrors (İmsakiyem first, emushaf second), optionally Diyanet’s own API with the owner’s credentials; then Aladhan’s Diyanet method; then on-device calculation. Calculated times are labelled approximate. |
| Cache | Day records on flash, normally through the end of the calendar year; weekly refresh at night; atomic writes. |
| Setup | The device opens its own WPA2 access point, shows a QR code and password, and serves a captive page where the owner picks the home Wi-Fi and the province and district. The place list is bundled, so setup needs no internet. |
| Time | SNTP, with an HTTP `Date` header fallback. UTC+3 by default. |
| Scope | One location in Türkiye, Turkish UI. |
| Board | CrowPanel Advance 7.0", DIS02170A rev V1.4, 16 MB flash, 8 MB PSRAM (owner decision 2026-11-20, constitution 1.1.0). The CrowPanel 7.0" basic, DIS08070H, is added as a second target in Slice 11 (specified 2026-11-26, constitution 1.2.0, `specs/011-dual-board-support/`). |
| Tooling | PlatformIO, host-testable core, a host UI harness that renders snapshots so the agent can check its own UI work. |

Slice 1 corrected the documents that described the basic board (DIS08070H) as the default and included brightness settings (spec 001, FR-033). Since 2026-11-26 the basic board is planned as a second target (Slice 11); where a document still speaks of a single target board, the constitution 1.2.0 and `specs/011-dual-board-support/` take precedence.

## Keeping the documents true

When something here turns out to be wrong (an API changed, a pin differs on the board), it is corrected in the same branch that found out, and the change is noted below.

## Document history

**23 November 2026**

- Repository restructure: the documents moved from `handover/` to `docs/` (`docs/specification/`, `docs/design/`, `docs/development/prompts/`). Contents unchanged.

**20 November 2026**

- `04-data-sources.md`: the İmsakiyem API description was read in full. It is now version 1.1.0 and adds version 2 endpoints: a times endpoint that returns up to 366 days in one request, and a district-detail endpoint with coordinates and time zone. The refresh plan, the coordinate lookup and the place-list generator now prefer these, with the version 1 calls as fallback. Added the known rough edges (paging metadata differs from the description; empty result for unknown ids). Aladhan’s `timezonestring` parameter and its date-range endpoint confirmed from client libraries. Reported emushaf rate limit added.
- `05-architecture.md`: the Advance model is now described from the vendor page and reviews: 16 MB flash, IPS panel, on-board battery-backed clock, and a buzzer in addition to the amplifier. Consequences for silence, time-keeping and flash are spelled out. The Arduino-core situation in PlatformIO is stated (official platform: 2.x core; 3.x through the community `pioarduino` platform).
- `03-functional-spec.md`, `01-product-brief.md`, prompts for Slice 0, 1, 6 and 10: adjusted to match.

**17 November 2026**

- First version of the documents.
