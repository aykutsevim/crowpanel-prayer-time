# Slice 0: Bootstrap

Goal: a repository with Spec Kit, a constitution that locks in the hard constraints, and a project skeleton whose host tests and firmware both build. No product code yet.

Run the steps in order. Blocks marked **paste into Claude Code** are prompts; blocks marked **terminal** are shell commands the owner runs (or asks Claude Code to run).

## Step 1: Check the environment

**Paste into Claude Code**

```text
Read CLAUDE.md and handover/README.md.

Then check this machine for the tools the project needs and report versions in a table: git, python3, uv, PlatformIO Core (pio), and the Spec Kit CLI (specify). For anything missing, tell me the exact install command for this operating system and wait for my go-ahead before installing. Do not create or change any project files in this step.
```

## Step 2: Initialise Git and Spec Kit

**Terminal**, in the repository root (the folder that contains `CLAUDE.md`):

```bash
git init
uv tool install specify-cli --from git+https://github.com/github/spec-kit.git
specify init --here --integration claude
specify check
```

Notes:

- The folder is not empty, so `specify init` asks for confirmation. Confirm; it adds its own files and does not touch `handover/`. If it offers to overwrite `CLAUDE.md`, decline, or restore the file from the package afterwards and keep any section Spec Kit added.
- Older Spec Kit releases use `--ai claude` instead of `--integration claude`. `specify init --help` shows which one your version accepts, and `specify integration list` shows the exact key for Claude Code.
- Restart Claude Code after this step so it picks up the new commands. Typing `/speckit` should list them. If they appear as `/speckit-constitution` rather than `/speckit.constitution`, use that spelling in every prompt of this package.

## Step 3: Constitution

**Paste into Claude Code**

```text
/speckit.constitution Create the constitution for the Namaz Vakti Panel firmware. Read CLAUDE.md, handover/docs/01-product-brief.md and handover/docs/05-architecture.md first, and make the principles below binding. Keep each principle short, testable, and marked with whether it is non-negotiable.

1. Silence (NON-NEGOTIABLE). The device never emits sound. No audio peripheral, library, tone or buzzer code may exist in the firmware. The amplifier is shut down as the first action after reset. PWM stays at or above 20 kHz. An automated check fails the build on any violation. No feature may be specified, planned or implemented that implies sound.

2. Truthful display (NON-NEGOTIABLE). The screen never presents calculated, stale or unsynchronised information as official or current. Every degraded condition has a defined visible state.

3. The approved design is the contract (NON-NEGOTIABLE). handover/docs/02-ui-spec.md and the reference renders define the UI. Deviations require the owner's written approval recorded in the feature's spec.

4. Offline first. The screen is always drawn from local state. Network and storage failures degrade the display in defined steps and never block or crash it.

5. Verify, do not assume. Hardware facts come from vendor documentation or the owner's inspection of the board. API and library behaviour comes from current documentation and live calls recorded in research.md, with versions pinned. Nothing about the physical device is reported as verified without owner confirmation or log evidence.

6. Testable core. Time logic, parsing, merging and refresh planning live in libraries with no hardware dependencies and are developed test-first on the development machine. UI code renders identically in a host harness and on the device.

7. Owner-in-the-loop verification. Each feature defines a short checklist of yes/no checks for the owner wherever human eyes, ears or a phone are needed, and is not done until the answers are recorded.

8. Small, private and polite. No cloud backend, accounts, telemetry or third-party update checks. Certificate verification is always on. Secrets never appear in logs or in the repository. External services are called rarely, one request at a time, with a descriptive User-Agent.

9. Resource discipline. The firmware fits the 4 MB board without over-the-air update. No blocking I/O on the UI thread. No steady-state heap growth. Flash writes are rare and atomic.

10. Simplicity and scope. One location in Türkiye, Turkish UI, the features in handover/docs/01-product-brief.md and nothing more. Prefer the simplest design that satisfies the numbered requirements in handover/docs/03-functional-spec.md; cite requirement ids in specs, tasks and tests.

Also record the governance rule: changes to principles 1 to 3 require the owner's explicit approval; any principle change updates dependent templates.
```

Review the generated constitution. It must contain all ten principles. Commit it.

## Step 4: Project skeleton

**Paste into Claude Code**

```text
Create the project skeleton described in handover/docs/05-architecture.md, without any product code. This is plain setup work, not a Spec Kit feature.

Before writing files, read handover/docs/05-architecture.md §3 on which Arduino core lines PlatformIO offers for ESP32, confirm against current documentation that it still holds, look at the vendor's current PlatformIO example for my board to see which core line it uses, and tell me which platform and exact release you will pin and why. Do not rely on memory for this.

Deliver:
1. The directory layout from 05-architecture.md §4, with a short README.md in each top-level directory stating what belongs there and the layering rule that applies.
2. A PlatformIO project with two environments: `native` for host unit tests (one trivial passing test in lib/core) and `crowpanel7_basic` for the ESP32-S3-WROOM-1-N4R8 board (4 MB flash, 8 MB PSRAM) whose main() only prints a version line on the serial port at 115200 baud. Add a custom partition table matching 05-architecture.md §5. Do not add display, touch, Wi-Fi or any other library yet.
3. tools/check_silence.py implementing 05-architecture.md §9, wired to run before every firmware build and inside the host test run. Include its own test: a temporary file containing a banned token must make it fail.
4. A single command (script or Makefile target, your choice) named `check` that runs host tests, the silence check and the firmware build, and prints the firmware size against the 2.9 MB budget.
5. .gitignore, and a root README.md with the development commands and a pointer to handover/README.md.

Then run `check` and show me its output. If the firmware build cannot run on this machine, say exactly why and what I need to install. Commit with a message that mentions Slice 0.
```

## Done when

- [ ] `.specify/` exists and the `/speckit.*` commands are available in Claude Code.
- [ ] The constitution is committed and contains the ten principles.
- [ ] `check` passes: host test green, silence check green, firmware builds, size reported.
- [ ] The repository root `README.md` lists the working commands.

Next: `slice-01-board-bringup.md`, in a fresh Claude Code session.
