"""Tests for tools/size_by_module.py (FR-032). Run: python -m unittest discover -s tools -p "test_*.py"."""
from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import size_by_module  # noqa: E402

MAP = """\
Discarded input sections

 .text.gone     0x00000000       0x99 .pio/build/x/src/app/bringup_app.cpp.o

Linker script and memory map

 .text.setup    0x42000010       0x20 .pio/build/x/src/app/bringup_app.cpp.o
 .literal._ZN3nvp4diag6formatEv
                0x42000030       0x10 .pio/build/x/lib1a2/libcore.a(report.cpp.o)
 .text._ZN3nvp4diag6formatEv
                0x42000040      0x100 .pio/build/x/lib1a2/libcore.a(report.cpp.o)
 .text.other    0x42000140       0x50 .pio/build/x/lib1a2/libnvp_core.a(other.cpp.o)
 .rodata.diag   0x3c000000       0x30 .pio/build/x/lib3b4/libui.a(diag_screen.cpp.o)
 .rodata.x.str1.1
                0x3c000100     0x8000 .pio/build/x/src/app/bringup_app.cpp.o
 .bss.counter   0x3fc80000        0x4 .pio/build/x/src/app/bringup_app.cpp.o
 .debug_info    0x00000000      0x500 .pio/build/x/src/app/bringup_app.cpp.o
"""


class SizeByModuleTest(unittest.TestCase):
    def test_sums_per_module(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "lib/core/src/nvp/diag").mkdir(parents=True)
            (root / "lib/core/src/nvp/diag/report.cpp").write_text("", encoding="utf-8")
            (root / "lib/ui/src/diag").mkdir(parents=True)
            (root / "lib/ui/src/diag/diag_screen.cpp").write_text("", encoding="utf-8")
            sizes = size_by_module.module_sizes(MAP, root)
        # discarded, .bss, debug and merged strings are not counted; other.cpp is not diag
        self.assertEqual(sizes, {"diag ui": 0x30, "diag core": 0x110, "app": 0x20})


if __name__ == "__main__":
    unittest.main()
