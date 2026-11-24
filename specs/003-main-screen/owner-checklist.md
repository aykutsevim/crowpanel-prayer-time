# Owner checklist: main screen (spec 003)

Constitution VII. Answer each question yes or no; add a note when something looks off. The implementer has already compared all fourteen renders with the references on the development machine (research R15); these questions are for what only eyes on the panel can judge.

**Setup**: the board is flashed with this slice's firmware and connected to the serial monitor (115200). Commands are in [contracts/serial-console.md](contracts/serial-console.md). Keep `docs/design/screens/` open on a computer to compare.

## A. Silence

| # | Question | Answer | Date | Notes |
|---|---|---|---|---|
| A1 | During everything below, did the device stay completely silent (no beep, click, whine or vibration)? | yes | 2026-11-24 | Owner: \"all tests are ok\" |

## P. Picture

| # | Question | Answer | Date | Notes |
|---|---|---|---|---|
| P1 | Send `fixture 1`. Does the panel look like `01-main-ogle-1432.png`: same layout, colours and text? | yes | 2026-11-24 | Owner: \"all tests are ok\" |
| P2 | Still on fixture 1: from about three metres, can you read the clock, the countdown and the six times? | yes | 2026-11-24 | Owner: \"all tests are ok\" |
| P3 | Can you tell the six colours of the day strip apart, also when you look from the side and from below? | yes | 2026-11-24 | Owner: \"all tests are ok\" |
| P4 | Send `fixture 2` to `fixture 9` one by one. Does each match its reference (02 to 09)? Note any that differs. | yes | 2026-11-24 | Owner: \"all tests are ok\" |
| P5 | Send `fixture 10` to `fixture 14`. Does each match its reference (10 to 14)? | yes | 2026-11-24 | Owner: \"all tests are ok\" |
| P6 | Send `fixture 1`, `clock sim 2026-11-17 00:00:00`, `clock speed 3600`. A whole day passes in 24 seconds. Did you see no flicker, no leftover pixels, and no sideways jumping of the ticking digits? Then send `fixture off`. | yes | 2026-11-24 | Owner: \"all tests are ok\" |

## K. Clock (board's real-time clock, OD3-1)

| # | Question | Answer | Date | Notes |
|---|---|---|---|---|
| K1 | After `fixture off`, does the clock on the main screen show the right local time (compare with your phone, within 2 seconds)? (The implementer set the board's clock on 2026-11-24 from the computer's clock.) | yes | 2026-11-24 | Owner: \"all tests are ok\" |
| K2 | Unplug the board for a few minutes, then plug it in again. Is the time on the screen still right within 2 seconds? | yes | 2026-11-24 | Owner: \"all tests are ok\" |
| K3 | After that restart, does the screen show the main screen (not the diagnostic screen), with `Vakitler alınamadı` at the top right, because no prayer times are loaded yet (reference 14; the city is empty because no location is set up yet)? | yes | 2026-11-24 | Owner: \"all tests are ok\" |

## G. Hidden diagnostic mode

| # | Question | Answer | Date | Notes |
|---|---|---|---|---|
| G1 | On the main screen, hold two fingers on the screen for 5 seconds. Does the diagnostic screen open? | yes | 2026-11-24 | Owner: \"all tests are ok\" |
| G2 | Back on the main screen (button `Main screen` on the diagnostic screen), tap once with one finger, and hold one finger for 5 seconds. Does nothing change? | yes | 2026-11-24 | Owner: \"all tests are ok\" |
| G3 | Did the `Main screen` button return to the main screen? | yes | 2026-11-24 | Owner: \"all tests are ok\" |
