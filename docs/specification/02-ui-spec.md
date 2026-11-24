# UI specification: Design A (“Şerit”)

This document defines every screen of the device at pixel level. It is written for an implementer who cannot see the hardware.

Three artefacts define the UI. If they disagree, this is the order of authority:

1. `docs/design/screens/*.png`: reference renders, exactly 800×480, one per state.
2. This document: positions, rules and text.
3. `docs/design/reference.html`: the interactive source the renders were made from. Open it in a browser to scrub through the day and switch states. `docs/design/tokens.json` holds the same numbers in machine-readable form.

The UI language is **Turkish**. Do not translate on-screen strings. Code, comments and logs are English.

## 1. Canvas and rendering rules

- 800×480 px, landscape, 16-bit colour (RGB565). Origin top-left, x right, y down. All numbers below are device pixels.
- Flat colour only: rectangles, rounded rectangles, straight lines, text. No gradients, shadows, blur or images (the only bitmap is the QR code on the setup and status screens).
- Opacity is used in exactly one place: the scrim behind the status overlay (§7.4).
- No animation. Values change in place once per second. The only time-based visual changes are the state changes in §5.
- Side margin is 24 px on every screen.
- The panel on the basic CrowPanel is TN, so contrast drops off-axis. Do not introduce low-contrast text beyond what is specified here.

## 2. Tokens

### 2.1 Colours

| Token | Hex | Use |
|---|---|---|
| `bg` | `#0F1B3D` | screen background, cut-outs, text on the yellow column |
| `ink` | `#F5F1E6` | primary text, QR background |
| `muted` | `#A3AFD6` | secondary text, hour labels |
| `dim` | `#6474A6` | passed prayer columns, placeholder dashes, hour ticks |
| `line` | `#3A4C8A` | leader lines, outlines |
| `now` | `#FFCF5C` | everything that means “now / next”: countdown, now marker, current column, progress fill |
| `alert` | `#FF8F66` | countdown under 15 min, warning status text |
| `track` | `#2A3B78` | progress track |
| `panel` | `#172552` | status overlay panel |
| strip `yatsi` | `#2A3B78` | night (00:00→İmsak and Yatsı→24:00) |
| strip `imsak` | `#7C6DC6` | İmsak→Güneş |
| strip `gunes` | `#7FC4E8` | Güneş→Öğle |
| strip `ogle` | `#D6EEFA` | Öğle→İkindi |
| strip `ikindi` | `#F0994A` | İkindi→Akşam |
| strip `aksam` | `#C2507A` | Akşam→Yatsı |

Yellow means “now” everywhere. Do not use it for anything else.

### 2.2 Type

Two families, both SIL OFL 1.1, both on Google Fonts:

- **Barlow** (text): Regular 400, Medium 500, SemiBold 600.
- **Barlow Condensed SemiBold 600** (all clock-like numbers).

Numbers that change every second must not jitter. In the reference they use tabular figures; LVGL has no OpenType features, so lay digits out yourself: one fixed cell per character, glyph centred in its cell.

- digit cell width = `round(0.475 × font_px)`
- colon cell width = `round(0.252 × font_px)`

| Role | Family / weight | Size px | Glyphs needed |
|---|---|---|---|
| Countdown | Barlow Condensed 600 | 114 | `0-9 : -` |
| Clock HH:MM | Barlow Condensed 600 | 96 | `0-9 : -` |
| Column times | Barlow Condensed 600 | 48 | `0-9 : -` |
| Clock seconds | Barlow Condensed 600 | 46 | `0-9 :` |
| Column times, calculated state | Barlow Condensed 600 | 44 | `0-9 :` |
| Setup SSID / password | Barlow Condensed 600 | 38 | ASCII letters, digits, `- _ . space` |
| Period bounds, IP, PIN | Barlow Condensed 600 | 25 | `0-9 : . space` |
| Period name, screen titles | Barlow 600 | 26 | Turkish set |
| Date | Barlow 500 | 21 | Turkish set + `≈` |
| “… kalan süre” label | Barlow 400 | 20 | Turkish set |
| Column names | Barlow 500 | 19 | Turkish set |
| City | Barlow 600 | 18 | Turkish set |
| Hijri date, setup body text | Barlow 400 | 17 | Turkish set |
| Column notes, overlay labels | Barlow 400 | 15 | Turkish set |
| Status text, legend | Barlow 400 | 14 | Turkish set |
| Hour labels | Barlow 400 | 13 | `0-9` |

**Turkish set** = space, `0-9`, `A-Z`, `a-z`, `Ç Ğ İ Ö Ş Ü ç ğ ı ö ş ü`, and `. , : ; - ’ ' ( ) / % ! ? + …`. The apostrophe used in strings is U+2019 (’); the ellipsis is U+2026.

Allowed consolidation to save flash (the basic board has 4 MB): hour labels may use the 14 px font; the note in the current column may use Regular 15 instead of SemiBold 15; IP and PIN may use the 25 px number font. Do not consolidate the number sizes.

Font conversion is part of Slice 3. Keep the converted fonts in the repository together with the script that produced them, so they can be regenerated.

## 3. Time-to-pixel mapping

Every horizontal position on the timeline derives from one function:

```
X(t) = 24 + t / 86400 × 752        t = seconds since local midnight, 0…86400
```

Column `k` (0 = İmsak … 5 = Yatsı) has centre `Cx(k) = 24 + (k + 0.5) × 752 / 6` → 86.67, 212.00, 337.33, 462.67, 588.00, 713.33.

## 4. Main screen

Reference: `01-main-ogle-1432.png`. The screen has four bands: status row, hero (clock left, countdown right), timeline, prayer columns.

```
y=0   ┌──────────────────────────────────────────────────────────────┐
      │ ⌖ İstanbul                               ⌔ Son güncelleme 03:00 │  status row
y=44  │ 14:32:07               Öğle vakti        12:54 ▬▬▬▬▭▭ 15:27  │
      │ 17 Kasım 2026 Salı     İkindiye kalan süre                    │  hero
      │ 7 Cemaziyelahir 1448   00:54:53                               │
      │ ▨ Kerahat vakti                                               │
y=240 │ 00    03    06    09    12    15    18    21    24            │
y=264 │ ████▓▓▒▒░░░░░░░░░░░░░░░░░▼░░░▒▒▒▓▓████████████████            │  timeline
y=294 │        \   \        |        |      /     /                   │  leader lines
y=336 │  İmsak  Güneş  [ Öğle ]  İkindi  Akşam  Yatsı                 │
      │  06:16  07:46  [12:54 ]  15:27   17:51  19:14                 │  columns
y=464 │                [şu an ]  54 dk…  3 sa…  4 sa…                 │
y=480 └──────────────────────────────────────────────────────────────┘
```

### 4.1 Elements

Text positions are given as **left or right edge + baseline y**, measured in the reference render. Match baselines; do not rely on a font’s line box.

| ID | Element | Position | Type / colour | Content |
|---|---|---|---|---|
| M1 | Location icon | 16×16 at (24, 18) | 1.6 px stroke, `ink` | map pin |
| M2 | City | left 46, baseline 32.5, at most 300 px wide | Barlow 600 18, `ink` | district name in Turkish title case, or province name if the district is the province centre; truncated with `…` if wider than 300 px |
| M3 | Status text | right edge 776, baseline 31.5 | Barlow 400 14, `muted` or `alert` | see §6 |
| M4 | Status icon | 16×16, 8 px left of M3 | stroke, same colour as M3 | see §6 |
| M5 | Clock HH:MM | left 24, baseline 130 | Barlow Cond. 600 96, `ink` | local time, 24 h |
| M6 | Clock seconds | 3 px right of M5, baseline 130 | Barlow Cond. 600 46, `muted` | `:SS` |
| M7 | Date | left 24, baseline 172 | Barlow 500 21, `ink` | `17 Kasım 2026 Salı` |
| M8 | Hijri date | left 24, baseline 196 | Barlow 400 17, `muted` | `7 Cemaziyelahir 1448` |
| M9 | Kerahat legend | swatch 22×10 at (24, 218), text left 53, baseline 228 | Barlow 400 14, `muted` | `Kerahat vakti`; hidden when the kerahat layer is off |
| M10 | Period name | left 340, baseline 76.5 | Barlow 600 26, `ink` | §9.2 |
| M11 | Period end | right edge 776, baseline 75.3 | Barlow Cond. 600 25, `ink` | `HH:MM` |
| M12 | Progress track | 132×8 at (580, 62), radius 4 | `track`; fill `now` from the left | fill width = elapsed fraction of the current period |
| M13 | Period start | right edge 570, baseline 75.3 | Barlow Cond. 600 25, `ink` | `HH:MM` |
| M14 | Countdown label | left 340, baseline 116 | Barlow 400 20, `muted` | `İkindiye kalan süre` (§9.2) |
| M15 | Countdown | left 337, baseline 221 | Barlow Cond. 600 114, `now` | `HH:MM:SS` until the next prayer time |
| M16 | Hour labels | centred on X(h·3600), baseline 248, h = 0, 3, …, 24 | Barlow 400 13, `muted` | `00 03 06 … 24` |
| M17 | Hour ticks | at every hour: x = X(h·3600) | `tick` (`#6474A6`) | every 3rd hour: y 254→260, 1.5 px; others: y 257→260, 1 px |
| M18 | Day strip | x 24…776, y 264…294, corner radius 7 | strip colours | §4.2 |
| M19 | Now marker | x = X(now) | `now` with `bg` outline | §4.3 |
| M20 | Leader lines | one per prayer time | `line` 1.5 px; current: `now` 2.5 px | §4.4 |
| M21 | Columns | six, each 125.33 wide, y 336…464 | | §4.5 |

Measured sizes for orientation: the clock `14:32` spans x 24…231, seconds to 289; the countdown spans x 337…719 with digit tops at y≈139.

### 4.2 Day strip (M18)

Seven segments, left to right, each filled with its strip colour and clipped to the rounded rectangle:

| From | To | Colour |
|---|---|---|
| 00:00 | İmsak | `yatsi` |
| İmsak | Güneş | `imsak` |
| Güneş | Öğle | `gunes` |
| Öğle | İkindi | `ogle` |
| İkindi | Akşam | `ikindi` |
| Akşam | Yatsı | `aksam` |
| Yatsı | 24:00 | `yatsi` |

At each of the six prayer times draw a 2 px vertical line in `bg` over the full strip height (y 264…294), centred on X(t). These gaps separate the segments.

**Kerahat layer** (optional, on by default): three intervals are hatched over the strip with 45° stripes in `bg`, 3 px stripe, 6 px period.

- Güneş → Güneş + `kerahat_sunrise_min`
- Öğle − `kerahat_noon_min` → Öğle
- Akşam − `kerahat_sunset_min` → Akşam

Defaults are 45 minutes each and are placeholders for the owner to confirm (see `01-product-brief.md`, open questions). A duration of 0 hides that interval.

### 4.3 Now marker (M19)

At x = X(now), drawn above everything else in the timeline band:

1. A 9 px wide vertical line in `bg`, y 259…299 (creates a gap in the strip so the marker is visible on any segment colour).
2. A 3 px wide vertical line in `now`, y 259…299, centred in that gap.
3. A downward triangle in `now` with a 2 px `bg` outline: points (x−7, 251), (x+7, 251), (x, 261).

Updates once per minute are sufficient (1 px ≈ 115 s).

### 4.4 Leader lines (M20)

For prayer `k` at time `t`: a 4-point polyline `(X(t), 294) → (X(t), 304) → (Cx(k), 325) → (Cx(k), 334)`. Stroke `line` 1.5 px. The leader of the current column is `now`, 2.5 px. Lines never cross because order is preserved.

### 4.5 Columns (M21)

Each column is a 125.33×128 box at x = 24 + k×125.33, y = 336. Content is horizontally centred on Cx(k).

| Row | Baseline y | Type | Content |
|---|---|---|---|
| Name | 363 | Barlow 500 19 | `İmsak`, `Güneş`, `Öğle`, `İkindi`, `Akşam`, `Yatsı` |
| Time | 413 | Barlow Cond. 600 48 | `HH:MM` |
| Note | 436.5 | Barlow 400 15 | depends on state, below |

Column states:

| State | When | Background | Name + time colour | Note text / colour |
|---|---|---|---|---|
| `past` | its time ≤ now and it is not the current period | none | `dim` | empty |
| `current` | now is inside its period, and that period started today | `now`, radius 12, full box | `bg` | `vakit girdi` for the first 5 min, then `şu an`; `bg`, SemiBold |
| `next` | the first upcoming time | none | `ink` | `54 dk sonra` in `now` |
| `later` | all other upcoming times | none | `ink` | `3 sa 18 dk sonra` in `muted` |

Two special cases:

- **After Yatsı** (Yatsı ≤ now < 24:00): Yatsı is `current`. The İmsak column shows **tomorrow’s** İmsak time, state `next`, note `yarın`. Other columns are `past`. Reference: `09-main-yatsi-2130.png`.
- **Before İmsak** (00:00 ≤ now < İmsak): the running period is yesterday’s Yatsı, but no column is `current`, because all six times shown are today’s and still ahead. İmsak is `next`, the rest `later`. The hero still says `Yatsı vakti`. Reference: `02-main-gece-0330.png`.

## 5. Time-driven states of the hero

| State | Condition | Change |
|---|---|---|
| Normal | otherwise | countdown `now` |
| Soon | remaining ≤ `soon_threshold_min` (default 15) | countdown colour `alert`. Reference: `07-main-az-kaldi-1742.png` |
| Entered | first 5 min of a period | current column note reads `vakit girdi`. Reference: `05-main-vakit-girdi-1255.png` |

The period bounds (M11–M13) for the night period show yesterday’s Yatsı and today’s İmsak before midnight-to-İmsak, and today’s Yatsı and tomorrow’s İmsak after Yatsı.

This device is silent. State changes are visual only; there is no chime, beep or call to prayer, and no UI element may suggest one.

## 6. System states

Exactly one system state is active. Evaluate top to bottom; the first match wins.

| # | State | Condition | Status icon | Status text (M3) | Colour | Other changes | Reference |
|---|---|---|---|---|---|---|---|
| 1 | `no_time` | wall-clock time is not valid | Wi-Fi off | `Saat alınamadı, Wi-Fi bekleniyor` | `alert` | clock shows `--:--`, no seconds; M7 reads `Saat bekleniyor`; M8 empty; M10 `Saat eşitleniyor`; M11–M13 hidden; M14 `Wi-Fi bağlantısı bekleniyor`; countdown `--:--:--` in `dim`; no now marker; if cached times exist, columns show them as `later` with empty notes | `13-state-time-not-set.png` |
| 2 | `no_data` | no record for today from any source | Wi-Fi off | `Vakitler alınamadı` | `alert` | clock and date normal; M10 `Vakitler alınamadı`; M11–M13 hidden; M14 `Wi-Fi ve konum ayarını kontrol edin`; countdown `--:--:--` in `dim`; strip is one `yatsi` bar with no separators, no leaders, no hatch, legend hidden; columns `--:--` in `dim` | `14-state-no-data.png` |
| 3 | `calculated` | today’s record is not Diyanet-derived (source rank below official, see `04-data-sources.md`) | ≈ | `Yaklaşık vakitler, hesaplama ile` | `alert` | every column time is prefixed with `≈` (Barlow 500 21, raised 8 px, 3 px gap) and drawn at 44 px instead of 48 | `12-state-calculated.png` |
| 4 | `cache_ending` | last refresh failed or Wi-Fi is down, and stored days after today ≤ 7 | Wi-Fi off | `Kayıtlı vakitler N gün sonra bitiyor` (`… bugün bitiyor` when N = 0) | `alert` | none | `11-state-cache-ending.png` |
| 5 | `offline` | Wi-Fi has been down for more than 10 min | Wi-Fi off | `Çevrimdışı, kayıtlı vakitler kullanılıyor` | `muted` | none | `10-state-offline.png` |
| 6 | `ok` | otherwise | Wi-Fi | `Son güncelleme HH:MM` if the last successful refresh was today, else `Son güncelleme DD.MM` | `muted` | none | `01-main-ogle-1432.png` |

Icons are 16×16, 1.6 px round-capped strokes. Paths are in the `<defs>` block of `reference.html`: `i-pin`, `i-wifi`, `i-wifioff` (Wi-Fi with a diagonal slash), `i-approx` (two waves).

## 7. Other screens

All use `bg` background and the same type scale.

### 7.1 Boot

Shown from power-on until the first screen is ready. Centred: `Namaz Vakti` (Barlow 600 26, `ink`, baseline 236), below it the firmware version (Barlow 400 15, `muted`, baseline 262). Nothing else. Maximum 3 s unless hardware init fails, in which case show the failing step as one line of `alert` text at baseline 300.

### 7.2 Setup, access point mode

Reference: `15-setup-ap.png`.

| Element | Position | Type | Content |
|---|---|---|---|
| Title | left 24, top 18 | Barlow 600 26, `ink` | `Kurulum` |
| Subtitle | left 24, below title | Barlow 400 17, `muted` | `Wi-Fi ayarını telefonundan yap` |
| Step list | left 24, top 92, width 450; rows 20 px apart | step number in a 36 px circle, 2 px `line` outline, Barlow 600 19; text Barlow 400 17 | see below |
| QR code | 268×268 at right 24, top 86; `ink` rounded square (radius 14), 14 px inner padding, modules in `bg` | | Wi-Fi join payload `WIFI:T:WPA;S:<ssid>;P:<password>;;`, error correction M |
| QR caption | centred under the QR, top 364 | Barlow 400 15, `muted` | `Bu ağa bağlanmak için okut` |
| Footer | left 24, bottom 18 | Barlow 400 17, `muted` or `alert` | status line, below |

Steps (the numbering is a real sequence):

1. `Telefonun kamerasıyla QR kodu okut ya da Wi-Fi ayarlarından bu ağa bağlan:` then two rows, label (Barlow 400 15, `muted`, 76 px column) + value (Barlow Cond. 600 38, `now`): `Ağ adı` → `NamazVakti-7F3A`; `Şifre` → `4827 1935` (password shown in two groups of four digits; the real password has no space).
2. `Açılan sayfada evdeki Wi-Fi ağını seç, şifresini yaz, ilini ve ilçeni seç.`
3. `Sayfa kendiliğinden açılmazsa tarayıcıya 192.168.4.1 yaz.` with the address in Barlow Cond. 600, `now`.

Footer text by situation:

| Situation | Text | Colour |
|---|---|---|
| waiting | `Telefonun bağlanması bekleniyor` | `muted` |
| a phone joined the access point | `Telefon bağlandı. Açılan sayfayı doldur.` | `ink` |
| last attempt failed: wrong password | `Wi-Fi şifresi kabul edilmedi. Tekrar dene.` | `alert` |
| last attempt failed: network not found | `Ağ bulunamadı. Yalnızca 2,4 GHz ağlar desteklenir.` | `alert` |
| last attempt failed: other | `Bağlanılamadı. Tekrar dene.` | `alert` |

### 7.3 Connecting

Reference: `16-connecting.png`. Three centred lines starting at y 150, 14 px apart: `Wi-Fi ağına bağlanılıyor` (Barlow 600 26), the SSID (Barlow Cond. 600 38, `now`; characters missing from the font render as `?`), `Bu işlem yarım dakika kadar sürebilir.` (Barlow 400 17, `muted`).

On success the first line becomes `Bağlandı` and the third `Saat ve vakitler alınıyor`, for at most 10 s, then the main screen appears. On failure return to the setup screen with the matching footer.

### 7.4 Status overlay

Reference: `17-status-overlay.png`. Opened by a tap anywhere on the main screen; closes on the next tap or after 20 s.

- Scrim: the main screen stays visible at 28 % opacity (equivalently, a full-screen `bg` rectangle at 72 % opacity on top of it).
- Panel: 600×376 at (100, 52), radius 16, fill `panel`, 1 px `line` border, padding 22 px top, 26 px sides.
- Title `Cihaz durumu` (Barlow 600 26).
- Two-column list, label column 150 px (Barlow 400 15, `muted`), value (Barlow 400 18, `ink`), rows 9 px apart:

| Label | Value example | Source |
|---|---|---|
| `Konum` | `İstanbul, Sultanbeyli` | configured province, district |
| `Vakit kaynağı` | `Diyanet (imsakiyem)` | provider of today’s record, §9.5 |
| `Son güncelleme` | `17.11.2026 03:00` | last successful refresh |
| `Kayıtlı son gün` | `31.12.2026` | last date present in the cache |
| `Wi-Fi` | `EvAgi_2.4G, iyi` | SSID + `iyi` / `orta` / `zayıf` (RSSI ≥ −60 / ≥ −75 / below), or `bağlı değil` |
| `Ayar adresi` | `192.168.1.37` | station IP |
| `Ayar PIN’i` | `482 913` | settings PIN, Barlow Cond. 600, `now` |
| `Sürüm` | `1.0.0` | firmware version |

- QR code 132×132 at panel right 24, top 70 (same style as §7.2, 8 px padding, radius 10) encoding `http://<station-ip>/`, caption `Ayar sayfası`. Hidden when Wi-Fi is down.
- Hint at panel bottom 18: `Kapatmak için dokun. Kurulumu baştan yapmak için 5 saniye basılı tut.` (Barlow 400 15, `muted`).

Holding a finger on the open overlay for 5 s replaces the panel content with a confirmation: title `Kurulum baştan yapılsın mı?`, text `Kayıtlı Wi-Fi ayarı silinir. Vakitler ve konum korunur.`, two buttons 200×56, radius 12: `Vazgeç` (outline `line`, text `ink`) and `Baştan kur` (fill `alert`, text `bg`). No response for 15 s counts as `Vazgeç`.

## 8. Touch

| Gesture | Where | Result |
|---|---|---|
| Tap | main screen | open status overlay |
| Tap | overlay | close overlay |
| Hold 5 s | overlay | restart-setup confirmation |
| Tap | confirmation buttons | as labelled |

Nothing else is interactive. Touch is never required for normal use.

## 9. Text

### 9.1 Formats

- Time of day: `HH:MM`, 24 h, zero-padded. Countdown: `HH:MM:SS`.
- Date: `D MMMM YYYY dddd`, e.g. `17 Kasım 2026 Salı`. Months: Ocak, Şubat, Mart, Nisan, Mayıs, Haziran, Temmuz, Ağustos, Eylül, Ekim, Kasım, Aralık. Days: Pazartesi, Salı, Çarşamba, Perşembe, Cuma, Cumartesi, Pazar.
- Hijri date: `D MMMM YYYY`. Months: Muharrem, Safer, Rebiülevvel, Rebiülahir, Cemaziyelevvel, Cemaziyelahir, Recep, Şaban, Ramazan, Şevval, Zilkade, Zilhicce. Prefer the string delivered with the prayer data; compute only as a fallback and never show a computed Hijri date as if it came from Diyanet without the `calculated` state.
- Duration (column notes), from whole minutes `m = floor(seconds / 60)`:
  - `m < 1` → `1 dk’dan az`
  - `m < 60` → `M dk`
  - otherwise `H sa M dk`, or `H sa` when M = 0
  - followed by ` sonra`.

### 9.2 Period and countdown label

| Period (from → to) | Period name (M10) | Countdown label (M14) |
|---|---|---|
| İmsak → Güneş | `İmsak vakti` | `Güneşe kalan süre` |
| Güneş → Öğle | `Kuşluk vakti` | `Öğleye kalan süre` |
| Öğle → İkindi | `Öğle vakti` | `İkindiye kalan süre` |
| İkindi → Akşam | `İkindi vakti` | `Akşama kalan süre` |
| Akşam → Yatsı | `Akşam vakti` | `Yatsıya kalan süre` |
| Yatsı → İmsak | `Yatsı vakti` | `İmsaka kalan süre` |

### 9.3 Column notes

`vakit girdi`, `şu an`, `yarın`, `<duration> sonra`.

### 9.4 Status and system strings

All strings in §6 and §7 are final copy. Keep them byte-for-byte, including the U+2019 apostrophe.

### 9.5 Source names for the overlay

| Provider | Shown as |
|---|---|
| Diyanet official API | `Diyanet (resmî servis)` |
| İmsakiyem | `Diyanet (imsakiyem)` |
| EzanVakti / emushaf | `Diyanet (emushaf)` |
| Aladhan | `Hesaplama (Aladhan)` |
| On-device calculation | `Hesaplama (cihaz)` |

## 10. Phone setup page (captive portal)

Served by the device in access-point mode at `http://192.168.4.1/`. It is a web page on the phone, not an LVGL screen, but it belongs to the same product and uses the same palette.

Constraints: one self-contained HTML document with inline CSS and JS, no external requests of any kind (the phone has no internet while joined to the device), under 25 kB gzipped excluding the place list, usable at 320 px width, Turkish, no framework.

Layout, top to bottom, single column, 16 px side padding, `bg` background, `ink` text:

1. Heading `Namaz Vakti kurulumu`.
2. Section `Wi-Fi ağı`: a list of scanned networks (name, signal as `iyi/orta/zayıf`, a lock mark for secured ones), strongest first, duplicates merged; a `Yenile` button; a row `Ağım listede yok` that reveals a free-text SSID field. Selecting a network fills the SSID.
3. `Wi-Fi şifresi` field, type password, with a `Göster` toggle. Hidden for open networks.
4. Section `Konum`: `İl` select (81 provinces, alphabetical by Turkish collation) then `İlçe` select (districts of the chosen province, province centre first). Both come from the place list bundled in the firmware; no network is needed.
5. Primary button `Kaydet ve bağlan` (fill `now`, text `bg`, min height 48 px). Disabled until SSID, password (if secured) and district are set.
6. After submit: the page shows `Cihaz Wi-Fi ağına bağlanıyor. Sonucu cihazın ekranından takip et.` and stops polling after 60 s. The phone will usually drop off the access point at this moment; the device screen is the source of truth for the result.

Validation messages (inline, `alert` colour): `Bir ağ seç`, `Şifre en az 8 karakter olmalı`, `İl ve ilçe seç`.

Networks the device cannot join (enterprise/802.1X, WEP, WAPI, OWE, DPP; NET-06) stay in the list, greyed, with `desteklenmiyor` in place of the signal word, and cannot be chosen (owner decision OD4-4, spec 004). The free-text field that `Ağım listede yok` reveals is labelled `Ağ adı` (OD4-5).

The same page layout is reused for the LAN settings page in Slice 8, with extra sections defined in `03-functional-spec.md` §8.

## 11. Reference renders

| File | State |
|---|---|
| `01-main-ogle-1432.png` | normal, afternoon |
| `02-main-gece-0330.png` | after midnight, before İmsak |
| `03-main-imsak-0650.png` | İmsak period |
| `04-main-kusluk-1000.png` | Güneş→Öğle |
| `05-main-vakit-girdi-1255.png` | first minutes of a period |
| `06-main-ikindi-1620.png` | İkindi period |
| `07-main-az-kaldi-1742.png` | under 15 min, countdown in alert colour |
| `08-main-aksam-1820.png` | Akşam period |
| `09-main-yatsi-2130.png` | after Yatsı, İmsak shows tomorrow |
| `10-state-offline.png` … `14-state-no-data.png` | system states, §6 |
| `15-setup-ap.png`, `16-connecting.png`, `17-status-overlay.png` | §7 |

The sample data in every render: İmsak 06:16, Güneş 07:46, Öğle 12:54, İkindi 15:27, Akşam 17:51, Yatsı 19:14; yesterday’s Yatsı 19:15; tomorrow’s İmsak 06:17; date 17 Kasım 2026 Salı; Hijri 7 Cemaziyelahir 1448. These values are illustrative test fixtures, not authoritative times.

## 12. Acceptance tolerances

- Colours: exact after RGB565 quantisation.
- Geometry of non-text elements (strip, separators, marker, leaders, column box, track): ±1 px.
- Text baselines: ±2 px. Text left/right anchors: ±2 px. Glyph shapes will differ slightly from the browser render; that is expected.
- Strings: exact.
- A snapshot produced by the host UI harness (see `05-architecture.md` §7) for each fixture in §11 must be reviewed side by side with the reference render before a UI task is called done.
