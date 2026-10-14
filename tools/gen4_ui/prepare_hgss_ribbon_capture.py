#!/usr/bin/env python3
"""Temporarily boot a CI-only Emerald build directly into the HGSS ribbon viewer.

This modifies ONLY the workflow checkout's local linker script. Do not commit
the modified linker script or use it for normal / release ROM builds.
"""
from pathlib import Path

script = Path("ld_script_modern.ld")
original = "gInitialMainCB2 = CB2_InitCopyrightScreenAfterBootup;"
test_boot = "gInitialMainCB2 = CB2_HgssRibbonGridTest;"
text = script.read_text()
if text.count(original) != 1 or test_boot in text:
    raise SystemExit("Unexpected linker script; refusing to patch")
script.write_text(text.replace(original, test_boot))
print("Temporary CI ROM will boot into CB2_HgssRibbonGridTest")
