#!/usr/bin/env python3
"""Overlay verified Emerald Ribbons UI footprints on original HGSS preview PNGs.

Diagnostic only: source artwork is unchanged; translucent rectangles are
measurements, not game assets. Do not import outputs into the ROM.
"""
import argparse
from pathlib import Path
from PIL import Image, ImageDraw

# Coordinates from src/pokenav_ribbons_summary.c on feature/gen4-ui-renderer.
ZONES = (
    ("Ribbon grid icons (4x2)", (88, 32, 216, 96), (0, 225, 225, 72)),
    ("Grid window", (80, 24, 240, 104), (240, 190, 35, 45)),
    ("Name window", (112, 8, 216, 24), (70, 130, 250, 72)),
    ("Count window", (96, 96, 224, 128), (240, 70, 95, 72)),
    ("List index", (8, 40, 64, 56), (130, 230, 100, 72)),
)
MON_ANCHOR = (40, 104)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("source", type=Path, help="Original standalone BG 256x256 PNG")
    p.add_argument("output", type=Path, help="240x160 diagnostic PNG")
    p.add_argument("--x", type=int, default=0)
    p.add_argument("--y", type=int, default=0)
    a = p.parse_args()
    if not (0 <= a.x <= 16 and 0 <= a.y <= 96):
        p.error("Crop must stay inside 256x256 original tilemap")
    image = Image.open(a.source).convert("RGBA")
    if image.size != (256, 256):
        p.error("Expected original 256x256 tilemap preview")
    image = image.crop((a.x, a.y, a.x + 240, a.y + 160))
    overlay = Image.new("RGBA", image.size)
    dr = ImageDraw.Draw(overlay, "RGBA")
    # Draw window before icons so their overlapping footprints remain explicit.
    for label, rect, color in ZONES:
        dr.rectangle(rect, fill=color, outline=color[:3] + (255,), width=1)
    x, y = MON_ANCHOR
    dr.line((x - 6, y, x + 6, y), fill=(255, 255, 255, 255), width=1)
    dr.line((x, y - 6, x, y + 6), fill=(255, 255, 255, 255), width=1)
    a.output.parent.mkdir(parents=True, exist_ok=True)
    Image.alpha_composite(image, overlay).save(a.output)
    print("Review zones (not authentic graphic assets):")
    for label, rect, _ in ZONES:
        print(f"  {label}: {rect}")
    print(f"  Pokémon sprite anchor: {MON_ANCHOR}")


if __name__ == "__main__":
    main()
