# PlatformIO post-build hook: reports the application image size and fails the build if it
# exceeds the budget (handover/docs/05-architecture.md §5, constitution Principle IX).
# Wired through `extra_scripts = post:tools/pio_size_budget.py` for device environments.
import os
import sys

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

BUDGET_BYTES = int(2.9 * 1024 * 1024)   # image budget, 2.9 MiB
PARTITION_BYTES = 0x300000              # factory app partition, partitions/crowpanel7_basic.csv


def _report(target, source, env):
    path = str(target[0])
    size = os.path.getsize(path)
    pct = 100.0 * size / BUDGET_BYTES
    print(
        f"Firmware size: {size} bytes ({size / 1048576:.3f} MiB) = {pct:.1f}% of the "
        f"{BUDGET_BYTES} byte (2.9 MiB) budget; app partition {PARTITION_BYTES} bytes."
    )
    if size > BUDGET_BYTES:
        sys.stderr.write("Firmware image is over the 2.9 MiB budget.\n")
        env.Exit(1)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", _report)  # noqa: F821
