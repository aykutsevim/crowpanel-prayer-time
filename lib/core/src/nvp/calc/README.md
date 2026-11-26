# Prayer-time calculation (spec 007 research R3)

A C++17 port of the astronomical core of **adhan-js** (MIT, Batoul Apps; notice in `LICENSE-adhan.txt`):

- Source: https://github.com/batoulapps/adhan-js
- Tag `v4.4.6`, commit `b6e04cd9da28fffc66773404a0a2e9c739370baf` (read 2026-11-26)
- Files ported: `src/Astronomical.ts`, `SolarCoordinates.ts`, `SolarTime.ts`, `PrayerTimes.ts`, `CalculationParameters.ts`, `CalculationMethod.ts` (Turkey only), `Madhab.ts`, `HighLatitudeRule.ts`, `MathUtils.ts`, `DateUtils.ts`, `TimeComponents.ts`, `Rounding.ts`
- Not ported: Qibla, Sunnah times, Shafaq, polar-circle resolution
- Turkey method (`CalculationMethod.ts` lines 88–96): Fajr 18°, Isha 17°, adjustments sunrise −7, Dhuhr +5, Asr +4, Maghrib +7
- Test oracle: `Shared/Times/*.json` of the same commit, copied to `test/fixtures/calc/adhan/`

No heap, no exceptions; results in minutes after local midnight. The port arrives with spec 007 T025 (Stage 2).
