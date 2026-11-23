# lib/providers

Pure C++17. One parser per data provider (bytes in, day records out, streaming and
field-filtered), and the refresh planner as a deterministic state machine that talks to an
abstract HTTP interface. See `docs/specification/04-data-sources.md`.

**Layering rule:** may depend on `lib/core` only. No platform headers, no sockets, no clocks.
Tested on the host against recorded responses in `test/fixtures/`.
