#!/usr/bin/env python3
"""Pixel-verify five *real mGBA framebuffer screenshots* of the isolated HGSS
ribbon viewer against unscaled retail NCGR + NCLR-derived sprite previews.

This does NOT capture the normal live PokeNav screen, cursor, or zoom animation.
"""
import sys
from pathlib import Path
from PIL import Image

if len(sys.argv) != 3:
    raise SystemExit("usage: verify_hgss_ribbon_emulator_screens.py SHOTS_DIR OFFLINE_DIR")

shots, offline = (Path(x) for x in sys.argv[1:3])
total_checked = 0
for page in range(1, 6):
    name = f"page_{page:02}.png"
    screenshot = Image.open(shots / name).convert("RGB")
    reference = Image.open(offline / name).convert("RGBA")
    if screenshot.size != (240, 160) or reference.size != (128, 64):
        raise SystemExit(f"{name}: unexpected screenshot/reference geometry {screenshot.size}/{reference.size}")
    actual = screenshot.crop((88, 32, 216, 96))
    count = 0
    errors = []
    for y in range(64):
        for x in range(128):
            expected = reference.getpixel((x, y))
            if expected[3] == 0:
                continue  # DS palette index zero is transparent; BG backdrop varies.
            count += 1
            got = actual.getpixel((x, y))
            # GBA color expansion can differ by a few 8-bit steps.
            if any(abs(got[c] - expected[c]) > 8 for c in range(3)):
                if len(errors) < 8:
                    errors.append((x, y, got, expected[:3]))
    if count < 20:
        raise SystemExit(f"{name}: reference has no substantial opaque HGSS artwork")
    if errors:
        raise SystemExit(f"{name}: framebuffer differs from retail icon pixels at {errors}")
    total_checked += count
    print(f"{name}: verified {count} opaque original HGSS pixels against actual mGBA framebuffer")
print(f"SUCCESS: {total_checked} opaque native HGSS pixels match across all five real emulator screenshots.")
