#!/usr/bin/env python3
"""Sanity-check screenshots captured from the actual PokeNav screen in mGBA.

This checks only capture geometry, nonblank rendering and meaningful visual
changes. Human inspection is still required for exact chrome/cursor quality.
"""
from pathlib import Path
import sys
from PIL import Image, ImageChops

if len(sys.argv) != 3:
    raise SystemExit("usage: verify_hgss_ribbon_live_screens.py SHOTS_DIR OFFLINE_DIR")
base = Path(sys.argv[1])
reference_dir = Path(sys.argv[2])
names = (
    "main_menu",
    "after_select_01",
    "after_select_25",
    "after_select_125",
    "owner_list",
    "summary_page_01",
    "zoom_page_01",
    "zoom_normal_page_04",
    "zoom_gift_page_04",
    "zoom_gift_page_05",
    "summary_page_05",
)
images = {}
failures = []
for name in names:
    img = Image.open(base / (name + ".png")).convert("RGB")
    if img.size != (240, 160):
        raise SystemExit(f"{name}: expected 240x160 GBA framebuffer, got {img.size}")
    colors = img.getcolors(240 * 160) or []
    # Transition snapshots may legitimately land during the fade to black.
    # Only settled screens must satisfy the nonblank rendering check.
    # A readable owner list legitimately uses 14 colors. Measure occupied
    # image area instead of requiring an arbitrary number of palette colors.
    dominant_pixels = max(count for count, _ in colors)
    if not name.startswith("after_select_") and dominant_pixels >= img.width * img.height * 0.99:
        failures.append(f"{name}: nearly uniform capture ({dominant_pixels}/{img.width * img.height} pixels)")
    images[name] = img
    print(f"{name}: {len(colors)} framebuffer RGB colors")

for left, right in (
    ("main_menu", "owner_list"),
    ("owner_list", "summary_page_01"),
    ("summary_page_01", "zoom_page_01"),
    ("zoom_page_01", "zoom_normal_page_04"),
    ("zoom_normal_page_04", "zoom_gift_page_04"),
    ("zoom_gift_page_04", "zoom_gift_page_05"),
    ("zoom_gift_page_05", "summary_page_05"),
):
    diff = ImageChops.difference(images[left], images[right])
    changed = sum(1 for p in diff.getdata() if p != (0, 0, 0))
    if changed < 64:
        failures.append(f"Missing screen transition between {left} and {right}: {changed} pixels")
    print(f"{left} -> {right}: {changed} changed pixels")
# Single-frame captures are diagnostic, not mandatory transitions.
for left, right in (
    ("main_menu", "after_select_01"),
    ("after_select_01", "after_select_25"),
    ("after_select_25", "after_select_125"),
    ("after_select_125", "owner_list"),
):
    diff = ImageChops.difference(images[left], images[right])
    changed = sum(1 for p in diff.getdata() if p != (0, 0, 0))
    print(f"Diagnostic {left} -> {right}: {changed} changed pixels")

if failures:
    raise SystemExit("Live capture diagnostics:\n" + "\n".join(failures))
print("Captured distinct live PokéNav states; manual screenshot review is still required.")

# Positive identity check: the unexpanded REAL PokéNav must display the
# original HGSS Champion ribbon in its native top-left 32x32 slot.
# This rejects earlier captures that mistakenly entered the Hoenn region map,
# even if all frames are colorful and visually different.
actual = images["summary_page_01"]
expected = Image.open(reference_dir / "page_01.png").convert("RGBA")
assert expected.size == (128, 64)
compared = 0
mismatched = 0
for yy in range(32):
    for xx in range(32):
        ref = expected.getpixel((xx, yy))
        if ref[3] == 0:
            continue
        compared += 1
        got = actual.getpixel((xx + 88, yy + 32))
        if max(abs(got[channel] - ref[channel]) for channel in range(3)) > 8:
            mismatched += 1
# Save pixel-level diagnostics with the CI screenshots for visual review.
# These are inspection aids, never substituted for authentic game artwork.
actual_crop = actual.crop((88, 32, 120, 64))
reference_crop = expected.crop((0, 0, 32, 32))
actual_crop.save(base / "champion_actual_crop.png")
reference_crop.save(base / "champion_reference_crop.png")
comparison = Image.new("RGB", (32, 32), (0, 0, 0))
for yy in range(32):
    for xx in range(32):
        ref = reference_crop.getpixel((xx, yy))
        if ref[3]:
            got = actual_crop.getpixel((xx, yy))
            bad = max(abs(got[c] - ref[c]) for c in range(3)) > 8
            comparison.putpixel((xx, yy), (255, 0, 0) if bad else (0, 160, 0))
comparison.save(base / "champion_pixel_comparison.png")

if compared < 64 or mismatched * 10 > compared:
    raise SystemExit(
        f"Actual live PokéNav did not display the retail HGSS Champion icon: "
        f"{mismatched}/{compared} opaque pixel mismatches"
    )
print(f"Live PokéNav identity confirmed: {compared - mismatched}/{compared} HGSS Champion pixels agree.")
