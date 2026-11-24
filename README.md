# Namaz Vakti Panel

Firmware for a silent touch display that shows the Diyanet prayer times for one district in Türkiye. It runs on an Elecrow CrowPanel Advance 7.0" (DIS02170A, rev V1.4: ESP32-S3, 16 MB flash, 8 MB PSRAM, 800×480 RGB panel, GT911 touch).

Three rules shape everything:

- **The device never makes a sound.** No audio code exists. The amplifier and buzzer are switched off as the first action after reset. Build checks enforce it on the sources and on the linked image.
- **The screen tells the truth.** Calculated or stale times are marked as such, and an unsynchronised clock is shown as unsynchronised.
- **The approved design is the contract.** The UI is implemented exactly as specified, in Turkish.

## Status

| Slice | Content | State |
|---|---|---|
| 0 | Repository, Spec Kit, constitution, check command | done |
| 1 | Silent board bring-up ([spec 001](specs/001-silent-board-bringup/spec.md)) | all three stages verified on the device: silence, picture, touch, Wi-Fi setup, 10-minute stress test; close-out in progress |
| 2–10 | Prayer engine, main screen, Wi-Fi setup, time, data, refresh, settings, release | planned ([roadmap](docs/specification/06-roadmap.md)) |

## Repository layout

```
docs/               product and technical documentation (start with docs/README.md)
  specification/    product brief, UI spec, functional spec, data sources, architecture, roadmap
  design/           reference renders (800×480), interactive reference, design tokens
  decisions/        recorded technical decisions
  development/      workflow and the Spec Kit prompts for every slice
specs/              Spec Kit feature records, one folder per slice
src/                firmware
  main.cpp          entry point (app_main): silence first, then the UI task
  platform/         the only code that touches the board and the SDK
  app/              wiring, self-tests, serial console
lib/                host-testable libraries
  core/             pure C++: engine, records, formatting, diagnostics
  providers/        pure C++: data-provider parsers, refresh planner
  ui/               LVGL screens, shared by the device and the host harness
test/               host unit tests (Unity)
sim/                host UI harness that renders screens to PNG
tools/              check command, silence, image and TLS checks, build hooks
include/            LVGL configuration
boards/             PlatformIO board definition (CrowPanel Advance 7.0")
partitions/         partition table (16 MB)
spikes/             throwaway experiments kept as evidence (l4-idf: the display under Wi-Fi load)
sdkconfig.defaults  ESP-IDF build options (the display depends on them; decision 0002)
data/, assets/      bundled data and fonts (later slices)
.specify/           Spec Kit templates, scripts and the constitution
```

The binding principles are in the constitution, [`.specify/memory/constitution.md`](.specify/memory/constitution.md). Working rules for Claude Code are in [`CLAUDE.md`](CLAUDE.md).

## Framework

The firmware uses plain **ESP-IDF 5.5.5** through PlatformIO (`espressif32@8.0.0`, `framework-espidf@4.50505.0`), with LVGL 9.6.0. It moved from Arduino-ESP32 during Slice 1 because the RGB panel only stays steady under Wi-Fi load with build options the precompiled Arduino libraries cannot have. The reasons and evidence are in [decision 0002](docs/decisions/0002-display-stack.md). The first device build compiles the whole framework and takes about 10 minutes; later builds take about a minute.

## Requirements

Git, Python 3.11+, `uv`, PlatformIO Core 6.2+, the Spec Kit CLI (`specify`) 1.0.4, and a host C++ compiler (GCC or MinGW on Windows) for the native tests and the UI harness. If `pio` is not on `PATH`, it lives in `~/.platformio/penv/Scripts/` (Windows) or `~/.platformio/penv/bin/` (Linux/macOS); `tools/check.py` finds it there automatically.

## Commands

| Purpose | Command |
|---|---|
| **Everything (run before every commit)** | `python tools/check.py` |
| Host-only check (no firmware build) | `python tools/check.py --skip-firmware` |
| Host unit tests | `pio test -e native` |
| Build firmware | `pio run -e crowpanel7_advance` |
| Flash | `pio run -e crowpanel7_advance -t upload` |
| Serial monitor | `pio device monitor -b 115200` |
| Render the engineering screens | `pio run -e sim -t exec` |

`check` runs the tool tests, the silence check, the TLS check, the host tests and the firmware build. It then prints the image size against the 2.9 MiB budget, the size of the diagnostic mode, checks the linked image for audio drivers, and runs the host UI harness. The silence check also runs before every PlatformIO build and test run.

If the PlatformIO IDE rewrites `.gitignore` with its own template, restore it with `git checkout -- .gitignore`; `tools/test_repository.py` fails until you do.

## On the device

- **Serial console**: 115200 baud through the USB-C port (CH340). Commands:

  | Command | Effect |
  |---|---|
  | `help` | list the commands |
  | `report` | print the report: silence, self-tests, Wi-Fi, memory, stress counters, reset reason |
  | `stress on`, `stress off` | start or stop stress mode (Wi-Fi scan and 4 kB write every 10 s, HTTPS every 30 s) |
  | `wifi setup`, `wifi cancel`, `wifi forget` | open the setup hotspot, close it, or erase the stored Wi-Fi details |
  | `diag` | show the diagnostic screen |
  | `test write`, `test scan`, `test https`, `test mix` | one diagnostic operation, logged with its duration |

  There is deliberately no backlight command. Passwords are never printed.
- **Wi-Fi setup**: on the device, open settings and start the setup; join the `NamazVakti-XXXX` hotspot from a phone with the 8-digit password shown on the screen, and open `192.168.4.1`.
- **DIP switches S1/S0**: both ON ("MIC&TF Card"), which disconnects the ESP32 from the amplifier.
- **Factory firmware backup**: keep it in `backup/`, which git ignores. The command is in [spec 001 quickstart](specs/001-silent-board-bringup/quickstart.md).

## Contributing

Development follows Spec Kit, one slice per branch, in roadmap order. The workflow is in [`docs/development/workflow.md`](docs/development/workflow.md). Commits are small, build, and cite requirement ids (for example `SIL-02`).
