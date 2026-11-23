# Data Model: Silent Board Bring-up

Plain data owned by `lib/core/src/nvp/diag/`, unless a platform owner is named. Field names are indicative; types are C++17.

## BoardProfile (compile time, `src/platform/board.h`)

| Field | Type | Value / rule |
|---|---|---|
| model | `const char*` | `"DIS02170A"` |
| revision | `const char*` | `"V1.4"`, configured. The firmware cannot read the revision. Reported as "configured, not detected" (spec edge case). |
| pins, timing | constants | research R3–R5, each with its source comment |

## SelfTestResult

| Field | Type | Rule |
|---|---|---|
| name | enum `Silence, Display, Touch, Backlight, Storage, WiFi` | the order of the report |
| outcome | enum `Pending, Passed, Failed, Skipped` | `Skipped` always carries a reason |
| reason | `char[48]` | short, ASCII, never contains a password (report test) |

What counts as passed:

| Test | Passed when |
|---|---|
| Silence | 249 and 247 both acknowledged by 0x30 within 20 tries |
| Display | panel created and frame buffer allocated |
| Touch | GT911 answers at 0x5D and its product ID reads `911` |
| Backlight | byte 0 acknowledged |
| Storage | LittleFS mounted and a 16-byte write/read-back matches |
| WiFi | radio started; one scan completed (zero networks counts as completed) |

**State**: `Pending → Passed | Failed | Skipped`, set once at boot. Storage and WiFi are re-evaluated by stress cycles. A failure never stops the other tests (FR-027).

## SilenceState (platform-owned, `audio_off.cpp`)

| Field | Type | Rule |
|---|---|---|
| acknowledged | `bool` | true only if both 249 and 247 were acknowledged |
| attempts | `uint8_t` | 1…20 |
| ran_first | `bool` | set by `audio_off()`. The report states it (FR-026). |
| rejected_count, last_rejected | `uint8_t` | bytes the gate refused before `Serial.begin`; printed with the silence line |

When `acknowledged == false`, `StressController::start` and `WifiSetup::start` refuse with reason `silence step not acknowledged`, and stored Wi-Fi details are not used (FR-001a).

## AuxCommand gate (pure, `audio_off.h`)

`bool aux_byte_allowed(uint8_t b)` is true only for `b == 0` (backlight full), `b == 247`, `b == 249` and `b == 250`. It is false for everything else, notably 246, 248 and every dimming value 1…245 (OD1). Contract: [contracts/aux-controller.md](contracts/aux-controller.md).

## Backlight

There is no level to model: the backlight is switched fully on (byte 0) once after the silence step and stays on (OD1, FR-016…018). The self-test records whether byte 0 was acknowledged; the report shows `backlight: full (dimming disabled)`.

## CornerTouchResult

| Field | Type | Rule |
|---|---|---|
| corner | enum `TopLeft, TopRight, BottomLeft, BottomRight` | target centres (20,20), (779,20), (20,459), (779,459) |
| state | enum `NotTouched, Passed, Failed` | Passed when \|dx\| ≤ 10 and \|dy\| ≤ 10. The latest touch counts (FR-014a). |
| dx, dy | `int16_t` | signed error in px |

A touch is assigned to the nearest corner target only if it lies within 60 px of that target. Otherwise it only moves the marker.

## StressCounters

| Field | Type | Rule |
|---|---|---|
| running | `bool` | |
| started_ms, elapsed_s | `uint32_t` | |
| cycles | `uint32_t` | +1 after scan and write complete |
| scan.ok / scan.err | `uint32_t` | a zero-network scan counts as ok |
| write.ok / write.err | `uint32_t` | |
| https.ok / https.err / https.skip | `uint32_t` | skip only when a request was due (FR-011) |
| last_error | `char[48]` | activity + reason |
| paused | `bool` | true while the AP is up (spec edge case) |

**Schedule**: one cycle about every 10 s. An HTTPS request is due when ≥ 30 s have passed since the last attempt. Pure `StressSchedule::next(now_ms, connected, ap_up)` returns the activities to run.

## StoredWifi (NVS namespace `wifi`)

| Key | Type | Rule |
|---|---|---|
| `ssid` | string ≤ 32 bytes | may be shown |
| `pass` | string 0…63 bytes | write-only (NET-08). Never printed, served or logged. |

Written only after a successful connection (FR-022). Erased by "forget Wi-Fi" (confirmed on screen). Slice 4 reads the same keys (FR-025).

## HotspotIdentity

| Field | Source | Rule |
|---|---|---|
| ssid | `NamazVakti-` + last 4 hex digits of the station MAC, upper case | NET-02 |
| password | NVS `ap/pass`, 8 decimal digits from `esp_random()`, generated on first use | shown only on the device screen (FR-024) |

## WifiState (state machine, `wifi_link.cpp` with a pure transition table in `lib/core`)

```
          details stored & silence ok          connect ok
  Idle ───────────────────────────────► Joining ─────────► Connected
   ▲  ▲                                  │  ▲                   │ link lost
   │  │ forget (confirmed)               │  └──── backoff ◄─────┘ (5 s → 5 min)
   │  └──────────────────────────────────┤ fail
   │                                     ▼
   │  setup confirmed     submit       Backoff
   └── SetupAP ─────────► Trying (≤ 20 s) ── ok ──► store, AP closes, Connected
         ▲   cancel → previous state         │
         └──────────── fail (discard) ───────┘
```

## Report (FR-026)

This is a line-oriented text block. The format is in [contracts/serial-console.md](contracts/serial-console.md). It is built by the pure `report::format(const ReportInput&)`. `ReportInput` contains no password field, so a password can never be formatted. It carries `boot_to_diag_ms` (0 means no display) for SC-006, and `flash_bytes` and `psram_bytes` as detected (owner answer I2).

## OwnerChecklist

The file [owner-checklist.md](owner-checklist.md) has, per question: id, text, yes/no answer, date, notes. It is not stored on the device.
