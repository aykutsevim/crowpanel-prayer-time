# tools

Development-machine scripts (Python 3.11+).

| File | Purpose |
|---|---|
| `check.py` | the one `check` command: tool tests, silence check, host tests, firmware build, size report |
| `check_silence.py` | silence check (SIL-05, `05-architecture.md` §9) |
| `test_check_silence.py` | its tests |
| `pio_silence_check.py` | PlatformIO pre-build hook running the silence check |
| `pio_size_budget.py` | PlatformIO post-build hook: image size against the 2.9 MiB budget |

Later: place-list generator, snapshot tooling, soak-log tools.

**Rule:** tools never contain secrets and never disable TLS verification.
