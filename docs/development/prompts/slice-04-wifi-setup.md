# Slice 4: Wi-Fi setup from the phone

Goal: someone who has never seen the device puts it on their home Wi-Fi and chooses their district using only a phone and the instructions on the device’s screen. See `docs/specification/06-roadmap.md`.

Slice 1 already built a minimal, engineering-only version of this (owner decision 2026-11-20; `specs/001-silent-board-bringup/contracts/setup-portal.md`, research R11). This slice extends it and does not replace it (FR-025 of spec 001):

- NVS namespace `wifi`, keys `ssid` and `pass`: stored only after a successful connection, the password write-only. Devices already set up in Slice 1 must keep working without being set up again.
- NVS namespace `ap`, key `pass`: the access-point password, eight decimal digits generated on first use and kept until factory reset. It is the `ap_password` of the settings table.
- Access point `NamazVakti-XXXX` (last four hex digits of the station MAC), WPA2-PSK, at most two clients, 192.168.4.1, AP+STA mode.
- Portal paths on port 80: `GET /`, `GET /api/scan`, `GET /api/status`, `POST /api/setup` (form-encoded `ssid` and `password`, answered `202` at once). This slice adds the rest of NET-04, the captive-portal DNS responder, the location fields and classified errors (NET-05).
- Platform code is plain ESP-IDF since Slice 1 (`docs/decisions/0002-display-stack.md`): `esp_wifi`/`esp_netif` for Wi-Fi, `esp_http_server` for the portal, `nvs` for storage. The custom portal was chosen over a Wi-Fi manager library in Slice 1.
- Rule from Slice 1: no flash write (NVS or files) while a Wi-Fi scan is in progress. Storing credentials after a connection must not overlap a scan.

## Step 1: Specify

**Paste into Claude Code**

```text
/speckit.specify Wi-Fi setup from a phone for the Namaz Vakti Panel, using the device's own access point.

Read first: docs/specification/03-functional-spec.md §1, §5, §7 and the settings table in §8; docs/specification/02-ui-spec.md §7, §8 and §10; reference renders 15, 16 and 17 in docs/design/screens/; docs/specification/04-data-sources.md §9.

Why: the device has no keyboard and must not need a cable, an app or an account. The owner's phone is the only tool. Setup also has to capture the location, and at that moment the phone has no internet because it is joined to the device, so everything the setup page needs must come from the device itself.

User stories, in priority order:

1. First setup. As a new owner, I power the device and its screen tells me what to do: it shows a network name, a password and a QR code. I scan the code with my phone's camera, my phone joins that network, and a page opens by itself. On the page I pick my home Wi-Fi from a list, type its password, choose my province and district, and press one button. The device's screen shows that it is connecting and then shows the main screen. Acceptance: the whole flow works on a current iPhone and a current Android phone without typing any address; from power-on to main screen takes under three minutes for a first-time user.

2. When it goes wrong. As the owner, if I type the wrong password, pick a network the device cannot reach, or something else fails, the device returns to the setup screen and tells me which of these happened, in the words given in the UI spec, and I can try again without restarting anything. Acceptance: each of the three failure messages can be provoked and is shown.

3. It remembers. As the owner, after a power cut the device reconnects to my Wi-Fi by itself and never asks me to set it up again. If my Wi-Fi is down it keeps retrying quietly and never opens its own network unasked.

4. Status at a tap. As the owner, I tap the main screen and see a panel with the location, the Wi-Fi network and signal quality, the device's address and the firmware version. Fields that later features will fill (time source, last update, stored range, settings PIN) are present and show a dash for now. Tapping again closes it.

5. Start over. As the owner, I hold my finger on the status panel for five seconds, confirm, and the device forgets the Wi-Fi network (but keeps my location) and shows the setup screen again, with my location preselected on the page.

6. Page without the pop-up. As the owner, if the page does not open by itself, typing the address shown on the device's screen into a browser opens it.

Requirements covered: MODE-02 to MODE-06, NET-01 to NET-06, NET-08, LOC-01, LOC-02, and 02-ui-spec.md §7, §8, §10. The access-point password and settings PIN are generated and stored as in the settings table.

Edge cases: a home network with a hidden name; an open network without a password; a network name with Turkish characters, spaces or emoji; more than twenty networks in range; two phones joining at once; the phone leaving before submitting; the page submitted twice; power lost while connecting; a 5 GHz-only network (not visible to the device: the page says only 2.4 GHz networks are supported); enterprise networks listed as unsupported.

Out of scope: using the location to fetch anything, the settings page on the home network, changing settings other than Wi-Fi and location, Bluetooth provisioning, a phone app.
```

## Step 2: Clarify

```text
/speckit.clarify Focus on the handover between phone and device when the connection attempt starts, and on what exactly is erased by "start over".
```

Suggested answers if asked: the device screen is the source of truth for the result; the page shows a fixed message after submit and stops polling after 60 s; “start over” erases Wi-Fi credentials only; the access-point password is the same every time until factory reset.

## Step 3: Plan

```text
/speckit.plan Plan Wi-Fi setup through the device's own access point.

Constraints and research:
- Research first, with sources, in research.md: (a) how current iOS and Android detect captive portals (probe hosts and paths, expected responses, what makes the sign-in sheet appear and stay open) and how ESP-IDF's access point (esp_wifi, esp_netif), a DNS responder and esp_http_server satisfy them; (b) behaviour of simultaneous access-point and station operation on the ESP32-S3, in particular channel changes when the station connects and what that does to the joined phone; (c) how ESP-IDF 5.5.5 reports connection failures (esp_wifi events and disconnect reasons) so they can be classified as wrong password, network not found or other; (d) Slice 1 chose a small custom portal; confirm it still fits the page and flow in 02-ui-spec.md §10, and extend Slice 1's portal code rather than adding a Wi-Fi manager library.
- Access point per NET-02; DNS and probe handling per NET-03; HTTP interface per NET-04. The server runs in its own context and only posts commands to the app task (05-architecture.md §6).
- The setup page is one self-contained document, no external requests, under 25 kB gzipped excluding the place list, usable at 320 px width, Turkish, matching the palette. Put its source in data/ with a build step that minifies and compresses it into the firmware. Write it so the same components are reused by the settings page in Slice 8.
- Place list: write tools/gen_places.py per 04-data-sources.md §9, run it, review its output (81 provinces, plausible district counts, Turkish characters intact), and commit the generated file with its generation date and checksum. Inspect the provider's location responses live before coding the script; do not assume their shape. The page loads the list from the device and applies Turkish title casing.
- Mode state machine per 03-functional-spec.md §1 in src/app, as a table-driven state machine that is unit-tested on the host with fake Wi-Fi events: every transition, timeout and failure class.
- Credentials and location are stored in non-volatile storage only after a successful connection, in Slice 1's NVS keys (`wifi/ssid`, `wifi/pass`; `ap/pass` for the access-point password); new keys such as the location go beside them. The Wi-Fi password is never logged or returned by any endpoint (NET-08).
- Screens: setup, connecting and status overlay from 02-ui-spec.md §7, in lib/ui, with fixtures 15 to 17 added to the host UI harness. QR codes are generated on the device; pick a small, maintained generator and pin it. The overlay's hold gesture and confirmation dialog are part of this slice.
- Reconnect policy per NET-01. The access point is started only in SETUP mode (MODE-05).
- Development serial commands: show setup screen, forget Wi-Fi, report Wi-Fi state. None of them prints a password. Slice 1 already has `wifi setup`, `wifi cancel`, `wifi forget` and the `wifi:` line of `report` (specs/001-silent-board-bringup/contracts/serial-console.md); extend those rather than adding parallel commands.
- Security review as part of the plan: what an attacker within radio range can and cannot do while the access point is up, and after setup.
```

## Steps 4 and 5

```text
/speckit.tasks
```

```text
/speckit.analyze
```

## Step 6: Implement

```text
/speckit.implement Stage 1: the place-list generator and its output, the mode state machine with host tests, and the three screens in the host harness (fixtures 15 to 17 compared with their references).
```

```text
/speckit.implement Stage 2: access point, DNS, HTTP server and the setup page, up to the point where a phone can join, the page opens and the network list and province/district lists work. Stop there and give me the first half of the owner checklist.
```

```text
/speckit.implement Stage 3: connection attempt, failure classification, storing credentials and location, reconnect policy, status overlay with tap and hold gestures, start-over flow. Give me the rest of the checklist.
```

## Step 7: Owner checklist

- [ ] Setup screen matches `15-setup-ap.png`; the network name on screen is the one my phone sees.
- [ ] iPhone: scanning the QR code joins the network and the page opens by itself.
- [ ] Android: same.
- [ ] If I dismiss the page, opening a browser and typing `192.168.4.1` brings it back.
- [ ] My home network is in the list; province and district lists are complete and correctly spelled.
- [ ] Correct details: the device shows the connecting screen, then the main screen.
- [ ] Wrong password: the device returns to setup with the password message.
- [ ] A made-up network name: the device returns to setup with the not-found message.
- [ ] After unplugging and replugging, the device reconnects without asking.
- [ ] With the router switched off, the device stays on the main screen and does not open its own network.
- [ ] Tap opens the status panel with my location, Wi-Fi name and address; tap closes it.
- [ ] Holding five seconds and confirming returns to setup, and my province and district are preselected.
- [ ] No sound at any point.

## Step 8: Converge and merge

```text
/speckit.converge
```

## Done when

The checklist is complete on both phone platforms and `check` passes.

Next: `slice-05-time-sync.md`.
