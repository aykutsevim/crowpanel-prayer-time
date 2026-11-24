# Feature Specification: Wi-Fi setup from the phone

**Feature Branch**: `feat/004-wifi-setup`

**Created**: 2026-11-24

**Status**: Draft

**Input**: User description: "Slice 4 on new feat branch" (Slice 4 of `docs/specification/06-roadmap.md`; brief in `docs/development/prompts/slice-04-wifi-setup.md`)

## Context

The device has no keyboard and must not need a cable, an app or an account. The owner's phone is the only tool. Setup also has to capture the location, and at that moment the phone has no internet because it is joined to the device, so everything the setup page needs must come from the device itself.

Slice 1 already built a minimal, engineering-only version of this flow (owner decision 2026-11-20; `specs/001-silent-board-bringup/contracts/setup-portal.md`). This feature extends it and does not replace it (FR-025 of spec 001): the same stored keys for the Wi-Fi details and the access-point password, the same access point and the same portal paths, now with the product screens, the captive-portal behaviour, the location, classified errors, the status overlay and the restart-setup flow.

Sources: `docs/specification/03-functional-spec.md` §1, §5, §7, §8; `docs/specification/02-ui-spec.md` §7, §8, §10; reference renders `15-setup-ap.png`, `16-connecting.png`, `17-status-overlay.png`; `docs/specification/04-data-sources.md` §9.

Requirements covered: MODE-02 to MODE-06, NET-01 to NET-06, NET-08, LOC-01, LOC-02, SEC-04, SIL-01, SIL-06, 02 §7.2 to §7.4, §8, §10.

## Owner decisions

- **OD4-1 (2026-11-24): a device set up in Slice 1 goes to setup.** It has stored Wi-Fi details but no location, so MODE-02 applies as written: the setup screen appears and the owner enters the Wi-Fi password again (it is write-only) and chooses a district. No special case for upgraded devices.
- **OD4-2 (2026-11-24): the overlay's settings-page QR code is hidden until the settings page exists.** A code that points to an address that does not answer would mislead (truthful display). The host render of fixture 17 still includes it for the comparison with the reference.

- **OD4-3 (2026-11-24, during planning): districts that Diyanet does not publish separately are listed under their centre's id.** The provider's list holds 869 Diyanet districts and 134 more (for example Kadıköy, Üsküdar, Çankaya) whose times are copied from the province centre and whose own ids (100000 and up) the second source does not know. They are listed by name; choosing one stores the centre's Diyanet id (Kadıköy → 9541) with the chosen district's name (research R6).

- **OD4-4 (2026-11-24, during planning): unsupported networks are marked `desteklenmiyor`.** 02 §10 has no wording for the mark NET-06 requires. The owner approved showing the network name greyed with `desteklenmiyor` after it (constitution III).

- **OD4-5 (2026-11-24, during convergence): the hidden-network field is labelled `Ağ adı`.** 02 §10 says that `Ağım listede yok` reveals a free-text field for the network name but gives it no label; the page labels it `Ağ adı`, the setup screen's own word for the network name (02 §7.2). The owner approved it (constitution III).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - First setup (Priority: P1)

As a new owner, I power the device and its screen tells me what to do: it shows a network name, a password and a QR code. I scan the code with my phone's camera, my phone joins that network, and a page opens by itself. On the page I pick my home Wi-Fi from a list, type its password, choose my province and district, and press one button. The device's screen shows that it is connecting and then shows the main screen.

**Why this priority**: Without it the device cannot be used by anyone but a developer.

**Independent Test**: Start a device with no stored Wi-Fi details and no location; complete the flow from an iPhone and from an Android phone without typing any address; time it.

**Acceptance Scenarios**:

1. **Given** no stored Wi-Fi details or no stored location, **When** the device starts, **Then** it shows the setup screen of 02 §7.2 (reference 15) with its own network name, its password in two groups of four digits, a QR code that joins that network, and the footer `Telefonun bağlanması bekleniyor`.
2. **Given** the setup screen, **When** the owner scans the QR code with a current iPhone or a current Android phone, **Then** the phone joins the device's network and the setup page opens without the owner typing an address, and the footer changes to `Telefon bağlandı. Açılan sayfayı doldur.`
3. **Given** the setup page, **When** it loads, **Then** it lists the visible networks (at most 20, strongest first, duplicates merged, signal as `iyi`/`orta`/`zayıf`, a lock mark for secured ones) and offers province and district lists, all without internet access.
4. **Given** a network, its correct password and a district are chosen, **When** the owner presses `Kaydet ve bağlan`, **Then** the page shows `Cihaz Wi-Fi ağına bağlanıyor. Sonucu cihazın ekranından takip et.`, the device shows the connecting screen of 02 §7.3 (reference 16) with the network name, then `Bağlandı` / `Saat ve vakitler alınıyor`, then the main screen.
5. **Given** a first-time user with the device and a phone, **When** they start from power-on, **Then** they reach the main screen in under three minutes.

---

### User Story 2 - When it goes wrong (Priority: P1)

As the owner, if I type the wrong password, pick a network the device cannot reach, or something else fails, the device returns to the setup screen and tells me which of these happened, and I can try again without restarting anything.

**Why this priority**: A wrong password is the most common setup failure; without a clear message the owner is stuck.

**Independent Test**: Provoke each of the three failures and read the device's footer; then retry with correct details in the same session.

**Acceptance Scenarios**:

1. **Given** a secured network and a wrong password, **When** the attempt ends, **Then** the device returns to the setup screen with `Wi-Fi şifresi kabul edilmedi. Tekrar dene.` in the `alert` colour.
2. **Given** a network name that is not in range (typed by hand, or a 5 GHz-only network), **When** the attempt ends, **Then** the footer reads `Ağ bulunamadı. Yalnızca 2,4 GHz ağlar desteklenir.`
3. **Given** any other failure (for example no address from the router, or no answer in 20 s), **When** the attempt ends, **Then** the footer reads `Bağlanılamadı. Tekrar dene.`
4. **Given** any failed attempt, **Then** nothing submitted is stored, the access point stays up with the same name and password, and the owner can submit again from the same or a reopened page without restarting the device.

---

### User Story 3 - It remembers (Priority: P1)

As the owner, after a power cut the device reconnects to my Wi-Fi by itself and never asks me to set it up again. If my Wi-Fi is down it keeps retrying quietly and never opens its own network unasked.

**Why this priority**: A device that has to be set up again after every power cut fails its purpose.

**Independent Test**: After setup, cut the power and restore it; then switch the router off for a long period and watch the phone's list of networks and the device screen.

**Acceptance Scenarios**:

1. **Given** completed setup, **When** power is cut and restored, **Then** the device shows the main screen and reconnects without any owner action.
2. **Given** the home network is unavailable, **When** the device is running, **Then** it retries indefinitely with growing pauses from 5 s up to 5 min, the screen stays on the main screen, and the device's own network never appears.
3. **Given** the home network comes back, **When** the next retry runs, **Then** the device reconnects without owner action.

---

### User Story 4 - Status at a tap (Priority: P2)

As the owner, I tap the main screen and see a panel with the location, the Wi-Fi network and signal quality, the device's address and the firmware version. Fields that later features will fill (time source, last update, stored range, settings PIN) are present and show a dash for now. Tapping again closes it.

**Why this priority**: It is the only way for the owner to see what the device is configured with, and the entry to "start over".

**Independent Test**: Render fixture 17 on the development machine and compare with reference 17; on the device, tap, read, tap again, and wait 20 s with it open.

**Acceptance Scenarios**:

1. **Given** the main screen, **When** the owner taps once with one finger, **Then** the status overlay of 02 §7.4 opens over the dimmed main screen.
2. **Given** the overlay, **Then** it shows `Konum` as `<province>, <district>`, `Wi-Fi` as `<network>, iyi|orta|zayıf` (or `bağlı değil`), `Ayar adresi` as the device's address on the home network, `Sürüm` as the firmware version, and a dash for `Vakit kaynağı`, `Son güncelleme`, `Kayıtlı son gün` and `Ayar PIN’i`.
3. **Given** the overlay is open, **When** the owner taps again or 20 s pass without touch, **Then** it closes.
4. **Given** the main screen, **When** the owner holds two fingers for 5 s, **Then** the hidden diagnostic screen of Slice 3 opens and the overlay does not.

---

### User Story 5 - Start over (Priority: P2)

As the owner, I hold my finger on the status panel for five seconds, confirm, and the device forgets the Wi-Fi network (but keeps my location) and shows the setup screen again, with my location preselected on the page.

**Why this priority**: Needed when the owner changes router or password; without it the only way out is a developer.

**Independent Test**: From the main screen, open the overlay, hold 5 s, try `Vazgeç`, then repeat and confirm `Baştan kur`; open the page and check the preselected province and district.

**Acceptance Scenarios**:

1. **Given** the overlay is open, **When** the owner holds one finger on it for 5 s, **Then** the panel shows `Kurulum baştan yapılsın mı?`, `Kayıtlı Wi-Fi ayarı silinir. Vakitler ve konum korunur.` and the buttons `Vazgeç` and `Baştan kur`.
2. **Given** the confirmation, **When** the owner taps `Vazgeç` or does nothing for 15 s, **Then** nothing is erased and the device returns to the main screen.
3. **Given** the confirmation, **When** the owner taps `Baştan kur`, **Then** the stored Wi-Fi details are erased, the location, settings, access-point password and any stored times are kept, and the setup screen appears.
4. **Given** setup after "start over", **When** the owner opens the page, **Then** the stored province and district are preselected.

---

### User Story 6 - Page without the pop-up (Priority: P3)

As the owner, if the page does not open by itself, typing the address shown on the device's screen into a browser opens it.

**Why this priority**: A fallback for phones that dismiss or never show the sign-in sheet.

**Independent Test**: Join the device's network, dismiss the sign-in sheet, open a browser and type `192.168.4.1`.

**Acceptance Scenarios**:

1. **Given** a phone joined to the device's network, **When** the owner types `192.168.4.1` into a browser, **Then** the setup page opens.
2. **Given** the same phone, **When** the owner types any other web address, **Then** the setup page opens as well.

---

### Edge Cases

- **Hidden home network**: `Ağım listede yok` reveals a field for the network name; the attempt proceeds as for a listed network (NET-06).
- **Open network**: the password field is hidden and not required; the attempt connects without a password (NET-06).
- **Enterprise network**: listed and marked as not supported; it cannot be selected (NET-06).
- **Network name with Turkish characters, spaces or emoji**: the page lists and submits it unchanged; the connecting screen shows characters missing from the font as `?`; the stored name is exactly the submitted bytes.
- **More than twenty networks**: only the twenty strongest are listed; a hidden or weak network can still be typed by hand.
- **5 GHz-only network**: not visible to the device; if typed by hand the attempt fails as "network not found" and the message says only 2.4 GHz networks are supported.
- **Two phones join at once**: both are admitted (at most two clients); a third is refused. Only one attempt runs at a time.
- **The page is submitted twice, or from two phones**: a submission while an attempt is running is refused without changing the running attempt.
- **The phone leaves before submitting**: nothing changes; the device keeps waiting, the footer returns to the waiting text when no phone is joined.
- **The phone drops off when the attempt starts** (the device's network may change channel): the device screen is the source of truth; the page's fixed message tells the owner to watch the device.
- **Power lost while connecting**: nothing was stored, so the next start behaves as before the attempt.
- **Wi-Fi details stored but the network is gone for good**: the device stays in RUN and keeps retrying; the owner uses "start over" (MODE-05).
- **Network name longer than 32 bytes or password longer than 63**: refused with an error by the device; the page prevents it first.
- **Secured network, password shorter than 8 characters**: the page shows `Şifre en az 8 karakter olmalı` and does not submit.
- **Tap on the main screen while a two-finger hold is starting**: a touch with two fingers is never a tap; the overlay opens only on a one-finger tap.
- **One finger held on the main screen** (not the overlay): nothing happens.
- **Factory reset**: belongs to the settings page (Slice 8, SET-03) and is not part of this feature.
- **The silence step failed** (spec 001 FR-001a): the device starts no radio, so it neither joins a network nor opens setup, whatever is stored; it shows the main screen and the diagnostic screen says why (analysis U1, mode `RunNoRadio` in data-model.md).
- **Values a later slice fills** (`Vakit kaynağı`, `Son güncelleme`, `Kayıtlı son gün`, `Ayar PIN’i`) show `—` (U+2014) in the value font in the `dim` colour, the token of the main screen's placeholder dashes (analysis U3).

## Requirements *(mandatory)*

### Functional Requirements

**Modes**

- **FR-001**: At start, the device MUST enter setup when stored Wi-Fi details or a stored location are missing, and the normal running mode otherwise (MODE-02). A device set up in Slice 1 (Wi-Fi details stored, no location) therefore enters setup; its stored Wi-Fi details are kept until a new attempt succeeds (OD4-1).
- **FR-002**: Setup MUST have no timeout (MODE-03). The device's own network MUST be up only in setup and during a connection attempt, and MUST never be started by the running mode on its own (MODE-05).
- **FR-003**: The behaviour of the modes (setup, connecting, running) and every transition, time limit and failure class MUST be verified on the development machine with simulated Wi-Fi events, without hardware.

**Device screens**

- **FR-004**: The setup screen MUST match 02 §7.2 and reference 15: title, subtitle, three numbered steps, the network name and the password (shown as two groups of four digits; the real password has no space), the QR code with the Wi-Fi join payload for this network and password (error correction M), its caption, and the address `192.168.4.1`. All strings byte for byte.
- **FR-005**: The setup screen footer MUST show, with the colour of 02 §7.2: `Telefonun bağlanması bekleniyor` while no phone is joined; `Telefon bağlandı. Açılan sayfayı doldur.` while a phone is joined; after a failed attempt, the message of its class (FR-016) until the next attempt starts.
- **FR-006**: The connecting screen MUST match 02 §7.3 and reference 16 with the submitted network name; on success it MUST show `Bağlandı` and `Saat ve vakitler alınıyor` for at most 10 s and then the main screen.
- **FR-007**: The status overlay MUST match 02 §7.4 and reference 17 and show the values of User Story 4. `Vakit kaynağı`, `Son güncelleme`, `Kayıtlı son gün` and `Ayar PIN’i` MUST show a dash until the features that fill them exist. The Wi-Fi row MUST use `iyi` (signal ≥ −60 dBm), `orta` (≥ −75 dBm), `zayıf` (below), or `bağlı değil`. On the device the overlay's QR code and its caption `Ayar sayfası` MUST stay hidden until the settings page exists (Slice 8); fixture 17 on the development machine renders them, so the layout is still checked against the reference (OD4-2).
- **FR-008**: Touch MUST follow 02 §8: a one-finger tap on the main screen opens the overlay; a tap on the overlay or 20 s without touch closes it; one finger held 5 s on the overlay opens the confirmation; on the confirmation `Vazgeç`, `Baştan kur`, or 15 s without response (counts as `Vazgeç`). The two-finger hidden gesture of Slice 3 MUST keep working and MUST NOT open the overlay.
- **FR-009**: The setup, connecting and overlay screens MUST be rendered on the development machine from fixtures 15, 16 and 17 and compared with their references within the tolerances of 02 §12.

**Access point and captive portal**

- **FR-010**: The device's network MUST be named `NamazVakti-` plus the last four hexadecimal digits of the device's station MAC address in upper case, secured with WPA2 and an eight-digit password generated from the hardware random source on first use and kept until factory reset, at address 192.168.4.1 with automatic addressing for at most two phones (NET-02). A password already generated in Slice 1 MUST be kept.
- **FR-011**: While the device's network is up, every name lookup from a joined phone MUST be answered with the device's address, and the probe addresses phones use to detect a sign-in page, and every unknown address or path, MUST lead to the setup page, so that current iPhones and Android phones open the page by themselves (NET-03).
- **FR-012**: The portal MUST offer the interface of NET-04, extending the paths and formats of Slice 1: the page, the network list, the place list, a status (mode, result class of the last attempt, stored location) and the submission (network name, password, district), answered at once as accepted. Requests with an empty or over-32-byte network name, an over-63-byte password, or an unknown district MUST be refused. A submission while an attempt runs MUST be refused.

**Setup page**

- **FR-013**: The page MUST follow 02 §10: one self-contained document with no external requests, under 25 kB compressed excluding the place list, usable at 320 px width, Turkish, with the product palette and the exact strings, headings, sections, buttons and validation messages of §10. It MUST be built so that its components can be reused by the settings page of Slice 8.
- **FR-014**: The network section MUST list at most 20 networks, strongest first, duplicates merged, with signal class and a lock for secured ones; enterprise networks MUST be marked as not supported and not be selectable; `Yenile` MUST rescan; `Ağım listede yok` MUST reveal a field for a hidden network name. The password field MUST have a `Göster` toggle and be hidden for open networks; a secured network MUST require at least 8 characters.
- **FR-015**: The location section MUST offer the 81 provinces in Turkish alphabetical order and then the districts of the chosen province, province centre first, with names in Turkish title case (`SULTANBEYLİ` → `Sultanbeyli`, using Turkish rules for `I/ı` and `İ/i`). A stored location MUST be preselected. After submission the page MUST show the fixed message of §10 and stop asking the device for status after 60 s.

**Connection attempt**

- **FR-016**: A submission MUST start an attempt of at most 20 s while the device's network stays up (MODE-04). The result MUST be classified as wrong password, network not found or other (NET-05), and shown with the matching footer of FR-005.
- **FR-017**: Only after a successful attempt MUST the device store the Wi-Fi details and the location (district id, province name, district name; LOC-01) in non-volatile storage, keep its own network up for a 5 s grace period, then close it and enter the running mode. On failure the submitted details MUST be discarded and the device MUST return to setup.
- **FR-018**: A write to storage MUST NOT overlap a Wi-Fi scan (rule of Slice 1).

**Running mode**

- **FR-019**: With stored details the device MUST join the network in the background at every start, without delaying the main screen, and after any loss of connection retry indefinitely with pauses growing from 5 s up to 5 min (NET-01).
- **FR-020**: `Baştan kur` MUST erase the stored Wi-Fi details only and keep the location, settings, access-point password and stored times; the device then enters setup (MODE-06).

**Place list**

- **FR-021**: The provinces and districts of Türkiye with Diyanet district ids MUST be bundled with the firmware (LOC-02). They MUST be produced by a generator kept in the repository from the provider named in 04 §9, which records the generation date and a checksum, and fails unless the result has exactly 81 provinces and every province has at least one district. The generated list MUST be reviewed (province count, plausible district counts, Turkish characters intact, a handful of ids confirmed against a second source) and committed.

**Secrets, silence, development tools**

- **FR-022**: A six-digit settings PIN MUST be generated from the hardware random source on first use and stored until factory reset (settings table, 03 §8).
- **FR-023**: The Wi-Fi password, the access-point password and the settings PIN MUST never appear in a log line, serial output, report or portal response (NET-08, SEC-04). The access-point password appears only on the device screen and in its QR code.
- **FR-024**: Development serial commands MUST extend those of Slice 1 rather than add parallel ones: show the setup screen (`wifi setup`), forget the Wi-Fi details (`wifi forget`), and report the Wi-Fi state including the mode and the last result class (`report`). They MUST NOT print a password or PIN.
- **FR-025**: This feature MUST NOT make a sound or imply one; touch gives no audible or haptic feedback (SIL-01, SIL-06).
- **FR-026**: The plan MUST include a security review: what someone within radio range can and cannot do while the device's network is up, and after setup.

### Key Entities

- **Wi-Fi details**: network name (1–32 bytes, any characters) and password (empty for open networks, 8–63 characters otherwise). Stored only after a successful attempt; the password is write-only. Kept where Slice 1 stored them.
- **Location**: Diyanet district id with province and district names (LOC-01); for a district without its own Diyanet times, the id is its centre's (OD4-3). Stored only after a successful attempt; kept by "start over".
- **Access-point password**: eight decimal digits, generated once, kept until factory reset; shown only on the device screen.
- **Settings PIN**: six decimal digits, generated once, kept until factory reset; not shown in this feature.
- **Connection attempt**: the submitted details, a start time, and a result: success, wrong password, network not found, or other.
- **Place list**: 81 provinces, each with its districts (id, name), plus generation date and checksum.
- **Device mode**: setup, connecting or running, with the transitions of 03 §1.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A first-time user completes setup from power-on to the main screen in under three minutes, on a current iPhone and on a current Android phone.
- **SC-002**: On both phones, scanning the QR code joins the network and opens the setup page with no address typed (2 of 2 platforms).
- **SC-003**: Each of the three failure messages is provoked and shown on the device, and a following correct attempt succeeds without a restart (3 of 3).
- **SC-004**: After a power cut with the router on, the device is back on the home network within one minute with no owner action.
- **SC-005**: With the router off for at least 30 minutes, the device's own network never appears in the phone's list and the main screen stays up.
- **SC-006**: The setup page is under 25 kB compressed (excluding the place list) and fits a 320 px wide screen without sideways scrolling.
- **SC-007**: The setup, connecting and overlay renders match references 15, 16 and 17 within the tolerances of 02 §12.
- **SC-008**: A serial log of a full session (setup, a failed attempt, a successful attempt, start over) contains no Wi-Fi password, access-point password or settings PIN.
- **SC-009**: The place list holds exactly 81 provinces, every province has at least one district, and the spot-checked district ids agree with the second source.
- **SC-010**: The device makes no sound at any point (owner confirmation).

## Assumptions

- The Slice 1 storage locations (`wifi/ssid`, `wifi/pass`, `ap/pass`) are kept, and the location and settings PIN are stored beside them; Slice 1's portal paths and form-encoded submission are kept and extended.
- The Slice 1 engineering setup page (English) is replaced by the product page of 02 §10; the Slice 1 diagnostic screen's Wi-Fi action and `wifi setup` now enter the same product setup flow.
- The `Saat ve vakitler alınıyor` step only waits: network time and prayer data arrive in Slices 5 and 6. The main screen after setup therefore shows the truthful `no_data` state of Slice 3 (with the board clock's time, or `no_time` if it is not valid).
- The settings PIN is generated and stored now but shown as a dash until the settings page that uses it exists (Slice 8), as the owner's brief states.
- "Current iPhone" and "current Android phone" mean the phones the owner has at hand, on their current system versions.
- Factory reset (SET-03) belongs to Slice 8; the access-point password and PIN are not changed by anything in this feature.

## Out of Scope

- Using the location to fetch anything (Slices 5 and 6); coordinates (LOC-03).
- The settings page on the home network and `namazvakti.local` (NET-07, Slice 8); changing anything other than Wi-Fi and location.
- Factory reset; Bluetooth provisioning; a phone app.
