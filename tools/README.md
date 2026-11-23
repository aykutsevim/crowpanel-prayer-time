# tools

Development-machine scripts (Python 3.11+). Every check has its own tests (`test_*.py`), run by
`python -m unittest discover -s tools -p "test_*.py"` and by `check.py`.

| File | Purpose |
|---|---|
| `check.py` | the one `check` command: tool tests, silence check, TLS check, host tests, firmware build, size report, diagnostic-mode size, UI harness |
| `check_silence.py` | silence check (SIL-05, `05-architecture.md` §9) |
| `check_tls.py` | TLS source check: no `setInsecure`, no one-argument https `begin` (SEC-01) |
| `size_by_module.py` | size of the diagnostic mode in the firmware image, from the linker map (FR-032) |
| `pio_silence_check.py` | PlatformIO pre-build hook running the silence check |
| `pio_size_budget.py` | PlatformIO post-build hook: image size against the 2.9 MiB budget, then the diagnostic-mode size |
| `test_repository.py` | repository rules: required `.gitignore` entries, audio libraries kept out of the device build |

Later: place-list generator, snapshot tooling, soak-log tools.

**Rule:** tools never contain secrets and never disable TLS verification.
