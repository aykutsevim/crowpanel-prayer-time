# Namaz Vakti Panel

Firmware for a silent ESP32-S3 touch display (Elecrow CrowPanel 7.0", 800×480) that shows
Diyanet prayer times for one district in Türkiye.

Start with [`handover/README.md`](handover/README.md): product brief, UI specification,
functional specification, data sources, architecture, roadmap and the Spec Kit prompts for every
slice. Working rules for Claude Code are in [`CLAUDE.md`](CLAUDE.md); the binding principles are
in [`.specify/memory/constitution.md`](.specify/memory/constitution.md).

## Requirements

Git, Python 3.11+, `uv`, PlatformIO Core 6.2+, Spec Kit CLI (`specify`) 1.0.4, and a host C++
compiler (GCC/MinGW on Windows) for the native tests. If `pio` is not on `PATH`, it lives in
`~/.platformio/penv/Scripts/` (Windows) or `~/.platformio/penv/bin/` (Linux/macOS);
`tools/check.py` finds it there automatically.

## Commands

| Purpose | Command |
|---|---|
| **Everything (run before every commit)** | `python tools/check.py` |
| Host-only check (no firmware build) | `python tools/check.py --skip-firmware` |
| Host unit tests | `pio test -e native` |
| Build firmware | `pio run -e crowpanel7_basic` |
| Flash | `pio run -e crowpanel7_basic -t upload` |
| Serial monitor | `pio device monitor -b 115200` |
| Silence check | `python tools/check_silence.py` |
| Silence check self-test | `python -m unittest discover -s tools -p "test_*.py"` |

`check` runs the tool tests, the silence check, the host tests and the firmware build, then
prints the image size against the 2.9 MiB budget. The silence check also runs automatically before
every PlatformIO build and test run, and the device build fails if the image exceeds the budget.

## Layout

```
lib/core/        pure C++: engine, records, formatting, settings, merge rules
lib/providers/   pure C++: provider parsers, refresh planner
lib/ui/          LVGL screens (host harness and device)
src/platform/    board, drivers, storage, network, time
src/app/         mode state machine, wiring, dev serial commands
data/            bundled place list and web pages
assets/fonts/    converted fonts and their generator
tools/           check, silence check, generators, snapshot and soak tools
test/            host unit tests and fixtures
sim/             host UI harness
boards/          PlatformIO board definition for the CrowPanel 7.0" basic
partitions/      partition table (05-architecture.md §5)
docs/decisions/  recorded technical decisions
```

## Workflow

One Spec Kit feature per slice, in the order of `handover/docs/06-roadmap.md`. In this
installation the commands are spelled `/speckit-specify`, `/speckit-clarify`, `/speckit-plan`,
`/speckit-tasks`, `/speckit-analyze`, `/speckit-implement`, `/speckit-converge`.
