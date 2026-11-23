# Contract: auxiliary controller (STC8H1K28, I²C 0x30)

Sources: research R3, R5, R6. Bus: I²C0, SDA GPIO15, SCL GPIO16, shared with GT911 (0x5D) and the RTC.

## Transport

- Each command is one byte, written as a single-byte I²C write to address 0x30, with no register address.
- Success means the controller acknowledged the byte. There is no read-back.
- The only writer is `aux_send(uint8_t)` in `src/platform/audio_off.cpp`. It calls `aux_byte_allowed()` first. A rejected byte is not sent, and the call logs `aux: rejected byte N`.

## Allowed bytes

| Byte | Meaning | Caller |
|---|---|---|
| 0 | backlight full | backlight, once at boot |
| 247 | buzzer off | `audio_off()` |
| 249 | amplifier mute | `audio_off()` |
| 250 | touch reset sequence | touch start-up |

## Rejected bytes (never sent)

| Byte | Reason |
|---|---|
| 1…244 | dimming; never used (owner decision OD1). 2 and 3 also overlap microphone and audio-path commands (EX lesson-10). |
| 245 | backlight off; not needed |
| 246 | buzzer on |
| 248 | amplifier on |
| 251…255 | undocumented |

## Unit tests (`test/test_audio_off/`)

- Every value 0…255 is checked against the table above.
- 246 and 248 are rejected.
- The silence sequence constant is exactly `{249, 247}`, in that order.
- The retry policy is at most 20 attempts.
