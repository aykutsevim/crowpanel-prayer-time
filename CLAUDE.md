# Namaz Vakti Panel: working rules

A silent ESP32-S3 touch display (Elecrow CrowPanel 7.0", 800×480) that shows Diyanet prayer times for one district in Türkiye. The full brief, specification, design and slice prompts are in `handover/`. Start with `handover/README.md`.

## Non-negotiable

1. **The device never makes a sound.** Do not initialise I²S, a DAC, a buzzer or any tone or audio API. Do not add an audio library. The amplifier is put in shutdown as the first action after reset. Backlight PWM stays at or above 20 kHz. No feature, setting, icon or text may imply sound. If a task seems to need sound, stop and ask.
2. **The design is fixed.** `handover/docs/02-ui-spec.md` and the reference renders in `handover/design/screens/` define the UI. Implement them; do not restyle, rearrange or re-word. On-screen text is Turkish and is copied exactly.
3. **Tell the truth on screen.** Times that are not Diyanet-published are marked approximate. A clock that is not synchronised is shown as such. Never present cached, calculated or guessed values as fresh official ones.
4. **Do not guess hardware.** Pins, timings and board revisions come from the vendor’s documentation and schematic, cited in the plan. If something is not documented, ask the owner to check the board.
5. **Do not guess external services or libraries.** Before coding against an API, SDK or library, read its current documentation, record what you found in the feature’s `research.md`, and pin versions. `handover/docs/04-data-sources.md` lists what was verified on 17 and 20 November 2026 and what still needs a live check.
6. **TLS certificate checks stay on**, in every build.

## How work is organised

- Development follows Spec Kit, one slice at a time, in the order of `handover/docs/06-roadmap.md`. The prompts for each step are in `handover/prompts/`. Do not start a slice before the previous one meets its “done when”.
- Requirements have ids (for example `ENG-03`, `DATA-05`). Cite them in specs, tasks, tests and commit messages.
- Code structure and layering rules are in `handover/docs/05-architecture.md` §4. `lib/core` and `lib/providers` must build and pass tests on the development machine with no hardware.
- Write tests for `lib/core` before the code.

## You cannot see the panel

- Use the host UI harness to render snapshots and compare them with the reference renders yourself. Do this before asking the owner to look at the device.
- When a check needs human eyes or ears, give the owner a short numbered checklist of yes/no questions and wait for the answers. Record them in the feature directory.
- Never report a hardware behaviour as verified unless the owner confirmed it or a log line proves it.

## Conventions

- Code, comments, commit messages and logs in English. UI strings in Turkish, kept in one place, with the U+2019 apostrophe where the spec uses it.
- Identifiers for the six times: `imsak`, `gunes`, `ogle`, `ikindi`, `aksam`, `yatsi`.
- No secrets in the repository or in logs: Wi-Fi passwords, the access-point password, the settings PIN, API credentials.
- Keep the firmware within the flash budget; report image size after each build.
- Small commits that build. One slice per branch, as Spec Kit creates them.
