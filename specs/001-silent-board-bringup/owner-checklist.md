# Owner checklist: Silent Board Bring-up (DIS02170A V1.4)

Answer each question with **yes** or **no**, and add a note when something looks odd. The implementer records your answers here with the date. The answers decide the display stack for the rest of the project (FR-030, FR-031). The backlight is always fully on (OD1).

Listening checks: a quiet room, your ear close to the board, speaker connector and buzzer. For A1 to A6, plug a speaker into the 2-pin speaker connector if you have one; afterwards it can stay unplugged.

## Before flashing: board inspection (Stage 1)

| # | Question | Answer | Date | Note |
|---|---|---|---|---|
| I1 | Does the PCB silkscreen say **V1.4**? | yes | 2026-11-23 | |
| I2 | What is the full marking on the metal ESP32 module (expected `ESP32-S3-WROOM-1-N16R8`)? | ESP32-S3-WROOM-1-N16R8 equivalent: flash 16 MB, embedded PSRAM 8 MB | 2026-11-23 | Shield shows no memory suffix. esptool flash-id (2026-11-23): `Detected flash size: 16MB`, `Features: ... Embedded PSRAM 8MB (AP_3v3)`, chip ESP32-S3 (QFN56) revision v0.2. Stage 1 report: `memory: flash 16 MB, psram 8 MB (detected)`. |
| I3 | What are DIP switches S1 and S0 set to? (Recommended: both ON, "MIC&TF Card", which disconnects the speaker path.) After setting, write the final position here. | both on (1/1, "MIC&TF Card") | 2026-11-23 | as recommended |
| I4 | Is a speaker connected to the 2-pin speaker connector? | no | 2026-11-23 | owner: "I want no audio" |
| I5 | What is printed on the 8-pin chip next to the speaker connector (expected NS4168)? | unreadable | 2026-11-23 | Identity taken from the V1.4 schematic (research R3). Owner: "I want no audio". |
| I6 | What is printed on the 32-pin chip (expected STC8H1K28)? | STC8H1K28 | 2026-11-23 | as researched |
| I7 | With the factory firmware still on the board: any click, beep or tone at power-on or reset? (yes/no) | no | 2026-11-23 | First recorded as "yes"; corrected by the owner on 2026-11-23: no sound heard at power-on or reset with the factory firmware. |
| I7a | What did you hear: a short beep, a melody, a click or pop, or something else? | no sound heard | 2026-11-23 | not applicable (I7 = no) |
| I7b | When did it start: the moment power was applied (before the screen lit), or after the screen showed the factory demo? | no sound heard | 2026-11-23 | not applicable (I7 = no) |
| I8 | Is a buzzer (round black part, about 5 × 5 mm) fitted on the board? (yes/no) | yes | 2026-11-23 | the buzzer-off command (247) is needed |

## Owner decision

| # | Question | Answer | Date |
|---|---|---|---|
| OD1 | Backlight dimming is produced by the board's auxiliary controller (helper chip) at a frequency the vendor does not document, so the ≥ 20 kHz rule cannot be confirmed from paper. How do we decide whether dimming may be used? | **C: no dimming at all; backlight always fully on** | 2026-11-20 |

## A. Silence (Stages 1–3)

| # | Question | Answer | Date | Note |
|---|---|---|---|---|
| A1 | Power-on: was the device completely silent (no click, pop, hiss, chirp, tone or whine)? | yes | 2026-11-23 | Stage 1 firmware 0.1.0 (power-on (USB unplugged and plugged in again)). An earlier "silent" the same day was given while the factory firmware still ran and counts only for I7. |
| A2 | Reset button: was it completely silent during the restart? | yes | 2026-11-23 | Stage 1 firmware 0.1.0 (reset button). An earlier "silent" the same day was given while the factory firmware still ran and counts only for I7. |
| A3 | With the backlight on (it is always full): no whine from the board? | yes | 2026-11-23 | no whine with the backlight fully on |
| A4 | Ten minutes of stress mode: silent throughout? | | | |
| A5 | Wi-Fi setup from the phone, including connecting: silent throughout? | | | |
| A6 | Touching the screen and its buttons: no sound and no vibration? | yes | 2026-11-23 | no sound or vibration while touching the screen and buttons |

## B. Picture (Stage 2, B5 in Stage 3)

| # | Question | Answer | Date | Note |
|---|---|---|---|---|
| B1 | Does each colour bar match its label (white, yellow, cyan, green, magenta, red, blue, black)? | yes | 2026-11-23 |  |
| B2 | Is the text the right way up, and is the USB-C connector on the side the arrow says? | yes | 2026-11-23 | USB-C on the left, as the arrow says |
| B3 | Is the thin white border visible on **all four** edges, touching the edge of the glass? | yes | 2026-11-23 |  |
| B4 | Idle for one minute: are the fine vertical lines steady (even grey from arm's length, no colour tint, no crawling, shifting, tearing or flicker)? | yes | 2026-11-23 | First look at 16 MHz: steady between the shifts reported under D1. With L1 (14 MHz): lines steady, no shift (owner, 2026-11-23). |
| B5 | During ten minutes of stress mode: do the lines and border stay exactly as in B3 and B4? If not, note when it happened and paste the serial log lines from that moment. | | | |

## C. Touch (Stage 2)

| # | Question | Answer | Date | Note |
|---|---|---|---|---|
| C1 | Does the marker follow your finger, and disappear (`no touch`) when you lift it? | yes | 2026-11-23 |  |
| C2 | After touching the centre of each corner target: are **all four** targets green? | yes | 2026-11-23 | All four green after retries (first touches red). Report: TL passed (-4,-9) TR passed (+9,-7) BL passed (-10,+2) BR passed (+5,+10): all errors point outward, two at the 10 px limit; touch scale to be checked (GT911 resolution registers 0x8146..0x8149). |

## D. Backlight (Stage 2)

| # | Question | Answer | Date | Note |
|---|---|---|---|---|
| D1 | Is the backlight on at full brightness and steady, with no flicker, and does the screen say `Backlight: full (dimming disabled)`? | yes (with L1, 14 MHz) | 2026-11-23 | 2026-11-23, firmware 0.1.0 Stage 2: flicker about every 3–4 s, picture sometimes moves left. Cause found: flash ID read every 100 ms (research, stack decision log). Fixed the same day; asked again. Second look after the fix (firmware with the flash read removed): backlight steady, blackout gone, picture still moves left and returns. Ladder step L1 (14 MHz) applied; asked again. Third look with L1 (PCLK 14 MHz): fully stable, backlight steady (owner, 2026-11-23). |

## E. Wi-Fi setup (Stage 3)

| # | Question | Answer | Date | Note |
|---|---|---|---|---|
| E1 | From the phone, could you join the hotspot shown on screen, open the page, choose your network and connect? Under three minutes? | | | |
| E2 | After pressing reset, did the device reconnect without setup? | | | |
| E3 | Did "Forget Wi-Fi" ask for confirmation, and after `Yes` show `Not set up`? | | | |

## F. Report (Stages 1 and 3)

| # | Question | Answer | Date | Note |
|---|---|---|---|---|
| F1 | Does the report show `DIS02170A V1.4` and `audio-off: acknowledged first`? (Paste the report block below.) | yes (log line) | 2026-11-23 | Captured by the implementer over COM4 after flashing Stage 1; block pasted below. Silence acknowledged on attempt 4 (about 150 ms after start-up). |
| F2 | Looking at the pasted report and log: does any Wi-Fi or hotspot password appear anywhere? (expected: no) | | | |

## Pasted evidence

```
2026-11-23, factory firmware, boot log after a reset through the USB bridge (before Stage 1 was flashed):
The microcontroller is detected: address 0x30
The microcontroller is detected: address 0x5D
命令 发送成功        (vendor text: "command sent successfully"; printed twice)
Setup done

2026-11-23, factory flash image saved before flashing: backup/factory-DIS02170A-V1.4.bin
(16 777 216 bytes, SHA-256 f2728ffe5fca3576f72e755263b5a72eb26f4caf089e965cc70871d48683a475; git-ignored)

2026-11-23, Stage 1 firmware 0.1.0, boot log after a reset through the USB bridge:
E (104) esp_core_dump_flash: No core dump partition found!
E (104) esp_core_dump_flash: No core dump partition found!
audio-off: acknowledged first (attempts 4)
=== NVP REPORT ===
firmware: 0.1.0
board: DIS02170A V1.4 (configured; not detected)
memory: flash 16 MB, psram 8 MB (detected)
audio-off: acknowledged first (attempts 4)
selftest silence: passed
selftest display: skipped (not in this stage)
selftest touch: skipped (not in this stage)
selftest backlight: passed
selftest storage: passed
selftest wifi: skipped (not in this stage)
wifi: off
backlight: full (dimming disabled)
heap internal: 337480 free, 278516 largest
heap psram: 8383012 free
stress: off
boot-to-diag: n/a (no display)
uptime: 0 s
=== END ===
(`report` sent 11 s later gave the same block with `uptime: 11 s`.)
```

## Decisions derived from the answers

| Decision | Value | Based on |
|---|---|---|
| Backlight | fixed fully on | OD1 (2026-11-20), A3, D1 |
| Display stack | see research.md "Stack decision log" | B4, B5 |
