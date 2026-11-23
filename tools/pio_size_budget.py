# PlatformIO post-build hook: reports the application image size and fails the build if it
# exceeds the budget (handover/docs/05-architecture.md §5, constitution Principle IX).
# Wired through `extra_scripts = post:tools/pio_size_budget.py` for device environments.
import os
import subprocess
import sys

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

BUDGET_BYTES = int(2.9 * 1024 * 1024)   # image budget, 2.9 MiB
PARTITION_BYTES = 0x300000              # factory app partition, partitions/crowpanel7_advance.csv


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
    # The diagnostic mode ships with the product; its share is reported separately (FR-032).
    project = env.subst("$PROJECT_DIR")
    map_file = os.path.join(env.subst("$BUILD_DIR"), "firmware.map")
    subprocess.run([sys.executable, os.path.join(project, "tools", "size_by_module.py"), map_file,
                    "--root", project])


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", _report)  # noqa: F821
