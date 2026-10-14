#!/usr/bin/env python3
"""Sanity-check screenshots captured from the actual PokeNav screen in mGBA.

This checks only capture geometry, nonblank rendering and meaningful visual
changes. Human inspection is still required for exact chrome/cursor quality.
"""
from pathlib import Path
import sys
from PIL import Image, ImageChops

if len(sys.argv) != 2:
    raise SystemExit("usage: verify_hgss_ribbon_live_screens.py SHOTS_DIR")
base = Path(sys.argv[1])
names = (
    "owner_list",
    "summary_page_01",
    "zoom_page_01",
    "zoom_normal_page_04",
    "zoom_gift_page_04",
    "zoom_gift_page_05",
    "summary_page_05",
)
images = {}
for name in names:
    img = Image.open(base / (name + ".png")).convert("RGB")
    if img.size != (240, 160):
        raise SystemExit(f"{name}: expected 240x160 GBA framebuffer, got {img.size}")
    colors = img.getcolors(240 * 160) or []
    if len(colors) < 16:
        raise SystemExit(f"{name}: suspiciously blank/low-color capture ({len(colors)} colors)")
    images[name] = img
    print(f"{name}: {len(colors)} framebuffer RGB colors")

for left, right in (
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
        raise SystemExit(f"Missing screen transition between {left} and {right}: {changed} pixels")
    print(f"{left} -> {right}: {changed} changed pixels")
print("Captured distinct live PokéNav states; manual screenshot review is still required.")
