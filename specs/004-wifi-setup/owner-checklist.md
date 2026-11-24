# Owner checklist: Wi-Fi setup from the phone (spec 004)

Constitution VII. Answers are yes/no unless a question asks for something else; the implementer
records them here. The board runs the Stage 2 build (flashed 2026-11-24 through COM4).

## First half (Stage 2, T047): device network, page, lists

Before you start: the board shows the setup screen now, because no location is stored yet (OD4-1).
Your phone may still remember the device's network from Slice 1; that is fine.

| # | Question | Answer |
|---|---|---|
| A1 | The setup screen looks like `docs/design/screens/15-setup-ap.png`: title, three numbered steps, network name, password, QR code, the address 192.168.4.1. | yes |
| A2 | The network name on the screen (`NamazVakti-…`) is the one your phone lists. | yes |
| A3 | iPhone: scanning the QR code with the camera joins the network. | yes |
| A4 | iPhone: after joining, the setup page opened by itself. (If not, answer A6 and say whether you opened Settings → Wi-Fi.) | yes |
| A5 | Android: scanning the QR code joins the network and the page opens by itself. | yes |
| A6 | If the page did not open by itself, typing `192.168.4.1` in the browser opened it. | yes |
| A7 | When a phone joins, the screen's bottom line changes to `Telefon bağlandı. Açılan sayfayı doldur.` | yes |
| A8 | The page lists your home network with `iyi`/`orta`/`zayıf` and a lock mark; `Yenile` refreshes the list, and the phone stays on the device's network while it does. | yes |
| A9 | Province and district lists: 81 provinces, your district is there, names are spelled correctly with Turkish letters (try one with İ/ı, for example Iğdır or Kadıköy). | yes |
| A10 | No sound at any point. | yes |

Owner, 2026-11-24: "All yes". The owner then pressed `Kaydet ve bağlan` with the home network and
a district. Log evidence (serial, COM4): the device was then connected to the home network at
-42 dBm with its own network closed; after a reset (EN pulse) it logged `mode: run` at 0.4 s and
`wifi: connected` at 7.2 s without entering setup, so the Wi-Fi details and the location were stored
(FR-017, FR-019). The home network's name is not recorded here.

## Second half (Stage 3, T070): attempt, errors, reconnect, overlay, start over

The board runs the Stage 3 build (flashed 2026-11-24 through COM4) and is set up (location stored).
The order matters: B1-B4 first, then B5 erases the Wi-Fi details, then B6-B10 set it up again.

| # | Question | Answer |
|---|---|---|
| B1 | Tap the main screen once: the status panel opens and shows your district, your Wi-Fi name with `iyi`/`orta`/`zayıf`, the address, `0.1.0` as the version, grey dashes for the four later fields, and no QR code. | yes |
| B2 | Tap again: it closes. Open it and wait 20 s without touching: it closes by itself. | yes |
| B3 | Two fingers held 5 s on the main screen still open the diagnostic screen (not the panel); `Main screen` brings you back. | yes |
| B4 | Unplug the power, plug it in again: the main screen appears and nothing asks for setup. | yes |
| B5 | Open the panel, hold one finger on it 5 s: the question `Kurulum baştan yapılsın mı?` appears. `Vazgeç` closes it and nothing changes. Hold again and press `Baştan kur`: the setup screen appears. | yes |
| B6 | On the page (after joining the device's network again), your province and district are already selected. | yes |
| B7 | Wrong password for your home network: the connecting screen appears, then setup again with `Wi-Fi şifresi kabul edilmedi. Tekrar dene.` | yes |
| B8 | `Ağım listede yok`, type `YokBoyleAg`, any password: setup again with `Ağ bulunamadı. Yalnızca 2,4 GHz ağlar desteklenir.` | yes |
| B9 | The correct password: the screen shows `Bağlandı` / `Saat ve vakitler alınıyor`, about 5 s later the main screen, and the device's network disappears from the phone's list. | yes |
| B10 | Router off for 30 minutes (or as long as you can): the main screen stays, the device's network never appears on the phone; after the router is back, the panel reconnects by itself (B1 shows the Wi-Fi name again). | yes |
| B11 | Did the Android phone stay on the device's network long enough for the page to load and submit? | yes |
| B12 | From power-on to the main screen, a first setup takes under three minutes (your estimate). | yes |
| B13 | While joined to the device's network (during B6), typing `example.com` in the browser also opens the setup page. | yes |
| B14 | No sound at any point. | yes |

Optional: a hidden network typed by hand is accepted (only if you have one). Not tested; see C2.

Owner, 2026-11-24: B1-B14 all yes (marked in this file by the owner). The build the owner tested
includes the LVGL memory fix of research R16, flashed before the second half.

## Convergence (T073, T077)

| # | Question | Answer |
|---|---|---|
| C1 | Log scan (SC-008): after the session below, `python tools/scan_log_secrets.py <log>` reports 0 occurrences of both passwords and no suspicious digit runs. | skip |
| C2 | A hidden network typed by hand is accepted, or: no hidden network is available (say which). | skip |

Owner, 2026-11-24: C1 and C2 skipped. SC-008 therefore rests on the code review of T067 (research
R14) and on the serial logs captured during development, which showed only network names and
addresses; no full-session log was scanned with the real passwords. Joining a hidden network
remains untested.
