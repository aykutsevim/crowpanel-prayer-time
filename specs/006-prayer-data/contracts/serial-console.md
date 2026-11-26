# Contract: serial console additions (FR-016, FR-017, SYS-05, SYS-06)

Development-only commands (`NVP_DEV_TOOLS`); the report line and the log lines appear in every build.

| Command | Effect | Reply |
|---|---|---|
| `data refresh` | asks the worker for a refresh now (needs running mode, Wi-Fi and a valid clock) | `data: refresh requested` or `data: refused (<reason>)`; the result follows as a log line |
| `data show` | prints the store summary | `data: district 9206, 403 records 2026-11-24..2027-12-31, source imsakiyem, crc ok, last refresh 2026-11-24 15:52` or `data: none (<reason>)` |
| `data corrupt` | flips one byte in the middle of `store_<id>.bin` (the RAM copy is unchanged until the next start) | `data: store file corrupted; restart to see the recovery` |
| `data erase` | deletes the store of the current district and its RAM copy | `data: store erased` |

**As implemented (T032):** `data refresh` answers at once on the UI side (`data: refresh requested`, or `data: refused (no location | not connected | time not valid)`). The other three are carried out by the network worker, which prints its reply as a log line (`[   t] data: …`). `data show` prints the last refresh in UTC (`last refresh 2026-11-24 13:31 UTC`), because the worker has no time zone; the `data:` report line prints it in local time. The checksum state is that of the file as it is now (`crc ok`, `crc bad checksum`). `data corrupt` answers `data: corrupt refused (no store file)` when there is no file.

## Report line (after `location:`)

```text
data: 403 records 2026-11-24..2027-12-31, today yes, source imsakiyem, last refresh 15:52 (0 min ago)
data: none
```

## Log lines

As implemented (examples from the device, 2026-11-24):

```text
[     430] data: no store for district 9206
[     438] data: store_9206.bin loaded, 366 records 2026-11-24..2027-11-24
[    8624] data: imsakiyem v2 ok, 366 records stored (2026-11-24..2027-11-24), 2.1 s, heap min 22 kB
[     442] data: store_9206.bin bad checksum, ignored
[    7385] data: imsakiyem v2 failed (connect ESP_ERR_HTTP_CONNECT)
[    7396] data: imsakiyem v1 failed (connect ESP_ERR_HTTP_CONNECT), retry in 60 s
[   68773] data: imsakiyem v1 failed (connect ESP_ERR_HTTP_CONNECT), no retry until reconnection
[   14394] data: store erased
```

`heap min` is `heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)` after the request, the lowest free internal heap since start, so an upper bound for the fetch's own low point. A rejected batch logs `imsakiyem v2 rejected (<reason>), <n> records, store unchanged`. After two failed attempts no further automatic attempt is made until the Wi-Fi reconnects, the date changes or `data refresh` is typed (Slice 7 replaces this with its scheduler). Planned forms:

```text
[ 21345] data: imsakiyem v2 ok, 403 records stored (2026-11-24..2027-12-31), 2.3 s, heap min 28 kB
[ 21345] data: imsakiyem v2 rejected (wrong district 9205), store unchanged
[ 22346] data: imsakiyem v1 ok, 30 records stored (2026-11-24..2026-12-23), 1.9 s
[ 21345] data: imsakiyem failed (timeout), retry in 60 s
[   612] data: store_9206.bin bad checksum, ignored
[ 21400] data: store for district 9541 removed (location changed to 9206)
```
