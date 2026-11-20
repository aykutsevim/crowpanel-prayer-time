# test

Host unit tests (Unity, `pio test -e native`), one folder per suite (`test_*/`), and
`fixtures/` with recorded provider responses (trimmed only by cutting whole records).

**Rule:** cite requirement ids (e.g. `ENG-07`) in test names or comments. Tests for `lib/core`
are written before the code.
