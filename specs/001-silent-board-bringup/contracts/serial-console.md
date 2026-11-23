# Contract: serial console

UART0 through the CH340K, 115200 8N1. Each command is one line ending in `\n`, with `\r` ignored. Matching is case-insensitive and extra spaces are ignored. Byte I/O is in `src/platform/console_io`; parsing is done by the pure `nvp::diag::parse_command`, and dispatch by `src/app/serial_console`.

## Commands (FR-028)

| Command | Effect | Refused when |
|---|---|---|
| `help` | prints the command list | — |
| `report` | prints the report (below) | — |
| `stress on` | starts stress mode | silence not acknowledged (FR-001a) |
| `stress off` | stops stress mode and prints final counters | — |
| `wifi setup` | starts the hotspot (no confirmation on serial) | silence not acknowledged |
| `wifi cancel` | closes the hotspot and keeps the stored details | — |
| `wifi forget` | erases the stored details and disconnects | — |
| `diag` | in this slice prints `already in diagnostic mode` and changes nothing; from Slice 3 on it leaves the product screen for the diagnostic screen (FR-007a) | — |

Unknown input prints `unknown command, type help` and changes nothing. There is deliberately no backlight command (OD1).

## Report format (FR-026)

Fixed keys, one per line, between markers, so the owner can paste the block:

```
=== NVP REPORT ===
firmware: 0.1.0
board: DIS02170A V1.4 (configured; not detected)
memory: flash 16 MB, psram 8 MB (detected)
audio-off: acknowledged first (attempts 1)
selftest silence: passed
selftest display: passed
selftest touch: failed (GT911 not found at 0x5D)
selftest backlight: passed
selftest storage: passed
selftest wifi: skipped (no Wi-Fi set up)
wifi: not set up | joining <ssid> | connected <ssid> -54 dBm | hotspot NamazVakti-1A2B at 192.168.4.1
backlight: full (dimming disabled)
touch corners: TL passed (+2,-1) TR not touched BL failed (+14,+3) BR passed (0,0)
heap internal: 182340 free, 160000 largest
heap psram: 7340032 free
stress: off | on 312 s cycles 31 scan 31/0 write 31/0 https 10/0/0 last-error none
boot-to-diag: 2840 ms
uptime: 42 s
=== END ===
```

- `audio-off` covers the whole silence sequence (249, then 247) and names no individual part (FR-026, SIL-05).
- `memory` gives the flash chip size and the PSRAM size as the chip reports them, so the report backs up owner answer I2.
- `boot-to-diag` is the time from reset to the diagnostic screen (SC-006); it reads `n/a (no display)` when there is no display, as in Stage 1.
- The hotspot line gives the name and address, never the hotspot password (FR-024).
- The report contains no password of any kind. This is tested by `test_report_never_contains_secret`, which feeds a password-like SSID and checks the output.
- Each stress event is also logged with a millisecond timestamp, for matching against the owner's observations:

  ```
  [  123456] stress: scan start
  [  123987] stress: scan done 7 networks
  [  124001] stress: write ok 4096 B
  [  130002] stress: https ok 200 812 B 1.4 s
  ```
