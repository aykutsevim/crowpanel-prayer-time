# Slice 8: Settings page

Goal: everything adjustable can be adjusted from a browser on the home network, protected by a PIN shown on the device. See `docs/specification/06-roadmap.md`.

## Step 1: Specify

**Paste into Claude Code**

```text
/speckit.specify Settings page for the Namaz Vakti Panel, served on the home network.

Read first: docs/specification/03-functional-spec.md §8, §9 and §11; docs/specification/02-ui-spec.md §7.4 and §10; docs/specification/04-data-sources.md §6 and §12; the owner's answers to the open questions in docs/specification/01-product-brief.md.

Why: after setup the device should need no attention, but a few things are personal: the district may change, the kerahat layer is a matter of preference. These must be adjustable without re-running setup and without controls on the device itself, and only by someone who can see the device.

User stories, in priority order:

1. Open the page. As the owner, on a phone or computer on my home network I open the address shown in the device's status panel (or scan its QR code, or use the device's local name) and get a Turkish settings page in the same visual style as the setup page. It shows the current values, the data source, the last update and the stored range.

2. Prove I am at the device. As the owner, saving any change asks for the six-digit PIN shown in the status panel. A wrong PIN is refused; after five wrong attempts the page is locked for five minutes. Reading the page needs no PIN and shows no secret.

3. Change district. As the owner, I pick another province and district, save, and within a minute the device shows the new district's times. The old district's stored times are dropped only once the new ones have arrived; until then the screen keeps showing the old district's times with its old name.

4. Display options. As the owner, I can switch the kerahat layer on or off and set its three durations, and choose when the countdown turns to the alert colour. Each change shows on the device within a second or two. There are no brightness or night-dimming options: the backlight is always fully on (owner decision OD1, 2026-11-20; SET-04 dropped).

5. Refresh now. As the owner, I can ask for an immediate refresh and see its outcome on the page.

6. Advanced. As the owner, I can set latitude and longitude by hand, a time-zone string, and store credentials for Diyanet's official service for later use. Stored passwords are never shown again.

7. Start from scratch. As the owner, I can reset the device to factory state after an explicit confirmation; it erases everything and returns to the setup screen.

Requirements covered: SET-01 to SET-03, NET-07, NET-08, DATA-08, LOC-03, SEC-01 to SEC-05, UI-05 (as amended: no backlight setting). SET-04 is dropped (OD1).

Edge cases: two browsers editing at once; a save while a refresh is running; values out of range; a district change while offline (accepted and applied when the network returns, clearly reported on the page); the device's address changing after a router restart; opening the page from outside the home network (not supported, nothing to do).

Out of scope: user accounts, HTTPS on the local page, remote access, firmware update through the page, any sound option, any brightness or dimming option.
```

## Step 2: Clarify

```text
/speckit.clarify Focus on what is visible without the PIN, and on the district hand-over while the new data is not yet stored.
```

Suggested answers if asked: without the PIN the page shows location, display options, source, last update and stored range, never passwords or the PIN itself; the lock-out is per device, not per browser; during hand-over the main screen keeps the old district until the new store has today’s record.

## Step 3: Plan

```text
/speckit.plan Plan the settings page.

Constraints and research:
- Research the local-name announcement (mDNS) support of ESP-IDF 5.5.5 (the framework since Slice 1, docs/decisions/0002-display-stack.md) and how reliably current iOS, Android, Windows and macOS resolve such names; the page must always be reachable by numeric address as well.
- Reuse the setup page's components and build step from Slice 4. One self-contained document, no external requests, Turkish. Size budget 40 kB gzipped excluding the place list.
- HTTP interface: read endpoints without the PIN; every state-changing endpoint requires it, compared in constant time. Decide whether the attempt counter and lock-out survive a restart; restarting the device must not be a way around the lock-out. State your choice. State-changing requests must not be triggerable by another site in the owner's browser: require a custom header or content type and reject cross-origin requests. Add host tests for the request handlers with a fake transport.
- Settings model in lib/core: one struct with defaults, ranges and validation from the table in 03-functional-spec.md §8, serialised to non-volatile storage with a version. Unit-tested, including migration from "no settings".
- Applying changes: handlers enqueue a command; the app task validates, persists and publishes a new snapshot; the UI reacts. No restart for anything except factory reset.
- District change per DATA-08: implement the hand-over in the store layer with a host test that fails the network after the change and proves the old district keeps being shown.
- No backlight settings: Slice 1 recorded that dimming cannot be confirmed silent on this board, and the owner decided against it (OD1). The backlight module only switches the backlight fully on, and its command gate refuses every other level; do not add one.
- Settings writes go to NVS; keep Slice 1's rule that no flash write happens while a Wi-Fi scan is in progress.
- Manual refresh goes through the planner from Slice 7 with its rate limit.
- Status overlay: show the PIN and the address QR code per 02-ui-spec.md §7.4; regenerate the overlay snapshot in the host harness.
- Factory reset per SET-03.
- Security notes in the plan: what someone on the home network without the PIN can learn or do; what is stored in plain form on flash and why that is acceptable for this device.
```

## Steps 4 to 6

```text
/speckit.tasks
```

```text
/speckit.analyze
```

```text
/speckit.implement
```

For a staged run, split after the settings model and handlers with their host tests, then the page, then device integration.

## Step 7: Owner checklist

- [ ] The address and QR code in the status panel open the settings page from my phone; `http://namazvakti.local/` works on at least one of my devices.
- [ ] The page shows current values and nothing secret before I enter the PIN.
- [ ] A wrong PIN is refused; the fifth wrong attempt locks the page for five minutes.
- [ ] Changing the district shows the new district’s name and times on the device within a minute.
- [ ] Kerahat off removes the hatching and the legend; changing a duration changes the hatched width.
- [ ] Changing the alert threshold changes when the countdown turns orange (check with the simulated clock).
- [ ] The page has no brightness or night-dimming controls, and the backlight stays fully on.
- [ ] “Şimdi güncelle” reports a result.
- [ ] Factory reset returns the device to the setup screen with a new PIN.
- [ ] No sound.

## Step 8: Converge and merge

```text
/speckit.converge
```

## Done when

The checklist is complete and `check` passes.

Next: `slice-09-hardening-release.md`.
