# Product brief: Namaz Vakti Panel

## What it is

A silent, always-on desk or wall display that shows today’s Islamic prayer times for one location in Türkiye, the current prayer period, and a countdown to the next one. It runs on an Elecrow CrowPanel 7.0" ESP32-S3 touch display (800×480).

It is a personal device for one household. There is no cloud backend, no account, no companion app and no telemetry.

## Who uses it and how

- **The owner** sets it up once from a phone: joins the device’s own Wi-Fi access point, enters the home Wi-Fi credentials, picks province and district. After that the owner rarely touches it.
- **Everyone in the room** glances at it from one to four metres away to answer three questions: what time is it, which prayer period are we in and when does it end, and how long until the next one.

## Goals

1. Show the six daily times published by Diyanet (Türkiye’s Presidency of Religious Affairs) for the chosen district: İmsak, Güneş, Öğle, İkindi, Akşam, Yatsı.
2. Make the current period, its start and end, and the time remaining obvious at a glance, using the approved layout (Design A, “Şerit”).
3. Keep working through Wi-Fi outages, API outages and power cuts: cache months of data, refresh it on its own, and fall back automatically through several sources.
4. Never mislead: if the times on screen are calculated rather than Diyanet-published, or the clock is not synchronised, the screen says so.
5. Be set up entirely from a phone, with no cables, apps or typing on the device.
6. Make no sound, ever.

## Non-goals for version 1

- Any audio: adhan, chime, beep, speech. This is a hard constraint, not a deferred feature.
- Multiple locations, multiple profiles, or travelling mode.
- Languages other than Turkish on the device and setup page.
- Qibla direction, monthly timetable view, Ramadan features, religious-day calendar, verse or hadith of the day, weather.
- Automatic time-zone detection outside Türkiye. The default zone is UTC+3 with no daylight saving; an advanced setting allows another POSIX TZ string.
- Battery operation and power saving. The device is mains powered.
- Remote access from outside the home network.

## Hard constraints

| Constraint | Detail |
|---|---|
| **Silence** | The firmware must not produce sound under any circumstance, including boot, errors, touch feedback and tests. The board has an audio amplifier and speaker connector (the Advance model also a buzzer and microphone); they must stay disabled. Backlight dimming must not create audible whine. |
| **Hardware** | Elecrow CrowPanel 7.0" ESP32-S3, 800×480, capacitive touch. Default target is the basic model (DIS08070H: 4 MB flash, 8 MB PSRAM, TN panel). The Advance model is a second build profile. See `05-architecture.md`. |
| **Design** | Design A as specified in `02-ui-spec.md`. The layout is approved; do not redesign it. |
| **Data authority** | Diyanet’s published times are the reference. Anything else is a fallback and is labelled as approximate. |
| **Setup** | Wi-Fi provisioning through the device’s own access point and a captive portal page on the phone. |
| **Process** | Development follows GitHub Spec Kit, slice by slice, as laid out in `06-roadmap.md` and `handover/prompts/`. |

## What success looks like

- From a cold start with stored settings, the main screen shows correct times within 20 seconds of Wi-Fi coming up, and immediately from cache if Wi-Fi never comes up but the clock can be set.
- Unplug the router for a week: the screen keeps showing correct Diyanet times and says it is offline.
- Block every Diyanet-derived source: within one refresh cycle the device shows calculated times marked as approximate, and returns to Diyanet times on its own when a source comes back.
- A person who has never seen the device can complete setup from the instructions on its screen in under three minutes.
- Thirty days of continuous operation without a restart, a frozen screen, or a wrong time.
- No sound is ever emitted.

## Glossary

Turkish terms are used as-is in code identifiers (ASCII-folded: `imsak`, `gunes`, `ogle`, `ikindi`, `aksam`, `yatsi`) and on screen.

| Term | Meaning |
|---|---|
| Vakit | A prayer time, and the period that starts at it. |
| İmsak | Dawn; start of the fasting day and of the morning prayer window. In Aladhan’s vocabulary this corresponds to `Fajr`, not to Aladhan’s `Imsak` field. |
| Güneş | Sunrise; end of the morning prayer window. The period Güneş→Öğle has no obligatory prayer and is called Kuşluk. |
| Öğle, İkindi, Akşam, Yatsı | Dhuhr, Asr, Maghrib, Isha. |
| Kerahat | Windows around sunrise, solar noon and sunset in which prayer is discouraged. Shown as a hatched layer on the day strip. |
| Hicri | The Hijri (Islamic lunar) calendar. |
| İl, İlçe | Province, district. Diyanet publishes times per district. |
| Diyanet | Diyanet İşleri Başkanlığı, the authority that publishes the official times. |
| Period | The interval from one vakit to the next. There are six per day; the last one (Yatsı) runs past midnight to the next day’s İmsak. |

## Open questions for the owner

These do not block Slice 1 to Slice 3. Each has a default that the firmware uses until the owner decides otherwise.

| # | Question | Default in use |
|---|---|---|
| 1 | Which board exactly: basic (label DIS08070H) or Advance (DIS02170A), and which hardware revision is printed on the PCB? | Basic, revision auto-detected where possible |
| 2 | Kerahat layer: keep it on, and are 45 minutes after sunrise, 45 before Öğle and 45 before Akşam the durations you want? | On, 45 / 45 / 45 |
| 3 | Do you have, or want to apply for, credentials for Diyanet’s official API? Without them the device uses community mirrors of the same data. | No credentials; optional later |
| 4 | Should the backlight dim at night? | Off; available as a setting in Slice 8 |
| 5 | Is a 6-digit PIN on the settings page acceptable, or should the home network be trusted without one? | PIN required |
