# Contract: serial console additions (FR-024, SYS-05, SYS-06)

This extends `specs/001-silent-board-bringup/contracts/serial-console.md` and `specs/003-main-screen/contracts/serial-console.md`. No command, reply or log line contains the Wi-Fi password, the access-point password or the settings PIN (FR-023).

## Changed commands

The commands answer through the network worker's log lines (below); a refusal is logged as
`wifi: refused` (or `wifi: refused, silence step not acknowledged`).

| Command | Effect in Slice 4 | What the log shows |
|---|---|---|
| `wifi setup` | `DevSetup`: enter setup mode and show the setup screen. The stored details are kept. Refused if the silence step failed. | `mode: setup`, `scan: done, N networks listed`, `wifi: device network NamazVakti-83EC open at 192.168.4.1`; with silence failed: `refused: silence step not acknowledged` |
| `wifi cancel` | `DevCancel`: leave setup back to running, only if Wi-Fi details and location are stored | `wifi: device network closed`, `wifi: joining <ssid>`, `mode: run`; otherwise `wifi: refused` |
| `wifi forget` | `StartOver`: erase the Wi-Fi details (the location is kept), then enter setup, as `Baştan kur` does (MODE-06) | `wifi: stored Wi-Fi details erased; location kept (MODE-06)`, `mode: setup`, then as `wifi setup` |

## Report lines

Slice 1's `wifi:` line is unchanged (`wifi: off | not set up | joining <ssid> | connected <ssid> -54 dBm |
hotspot NamazVakti-83EC at 192.168.4.1`). Spec 004 adds two lines at the end of the report, after
`screen:`; names and classes only, never a password or PIN:

```text
screen: setup | connecting | main (state no_data[, fixture][, overlay]) | diagnostic | settings
mode: setup, phones 1, last none | connecting | connected | run, joining | run, connected | run, waiting to retry | run, no radio (silence)
location: ANKARA / ANKARA (9206) | location: none
```

## Fixtures (development builds)

| Command | Effect |
|---|---|
| `fixture 15` / `16` / `17` | shows the setup, connecting or overlay screen with the reference sample data (network `NamazVakti-7F3A`, password `48271935`, SSID `EvAgi_2.4G`, location `İstanbul, Sultanbeyli`, address `192.168.1.37`, version `1.0.0`). Fixture 17 draws the overlay QR code, so it can be compared with the reference. On the device in normal use the QR code stays hidden (OD4-2). |
| `fixture off` | back to the live screen of the current mode |

The sample password `48271935` is the reference render's value, not the device's.

## Log lines (one per mode change, SYS-05)

```text
[  1234] mode: setup (no location)
[ 40211] portal: phone joined (1)
[ 41000] portal: rescan (grouped)
[ 61002] mode: connecting "EvAgi_2.4G"
[ 66480] attempt: failed wrong_password (2 reasons, last 15)
[ 90120] attempt: ok 192.168.1.37; stored wifi + location 9549
[ 95130] wifi: device network closed
[ 95131] mode: run
[ 99999] wifi: not connected, retry in 5 s
[ 99999] lvgl memory: 58108 of 304788 B used (20 %), largest free 62076 B   (after start-up and each fixture, R16)
```
