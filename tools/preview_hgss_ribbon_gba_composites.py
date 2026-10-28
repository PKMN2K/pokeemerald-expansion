#!/usr/bin/env python3
"""Exploratory HGSS ribbon background layer composites with GBA occupancy guides.

Original pixels are reconstructed exclusively from the retail a/1/6/2 NARC.
Neither ordering represents verified retail DS priority/transparency behavior.
The guide rectangles are diagnostic annotations, never exported game artwork.
"""
import argparse
from pathlib import Path

from PIL import Image, ImageDraw

from render_hgss_ribbon_backgrounds import narc_members, palette_banks, render
from export_hgss_ribbon_gba_bg import verify_source_narc
from audit_hgss_ribbon_gba_layout import REGIONS

LAYERS = {"bg3_10": (2, 10), "bg3_11": (2, 11),
          "bg6_9": (1, 9), "bg6_78": (1, 78)}
CASES = (("ribbons_normal", "bg3_10", "bg6_9"),
         ("ribbons_alternate", "bg3_11", "bg6_9"),
         ("ribbons_fallback", "bg3_10", "bg6_78"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("narc", type=Path, help="Authentic a/1/6/2 NARC")
    parser.add_argument("out", type=Path, help="Output directory")
    parser.add_argument("--viewport-x", type=int, default=0)
    parser.add_argument("--viewport-y", type=int, default=0)
    args = parser.parse_args()
    if not (0 <= args.viewport_x <= 16 and 0 <= args.viewport_y <= 96):
        parser.error("GBA viewport must fit the 256x256 source tilemaps")
    archive = args.narc.read_bytes()
    verify_source_narc(archive)
    members = narc_members(archive)
    if len(members) != 79:
        raise ValueError("Expected 79 original NARC members")
    palettes = palette_banks(members[0])
    images = {name: render(members[char_id], members[map_id], palettes,
                           {1: 512, 2: 256}[char_id])
              for name, (char_id, map_id) in LAYERS.items()}
    args.out.mkdir(parents=True, exist_ok=True)
    for case, bg3, bg6 in CASES:
        for order in ((bg6, bg3), (bg3, bg6)):
            # Transparent pixel convention is a diagnostic approximation:
            # actual DS BG mixing, scroll, priority and window masks differ.
            composite = Image.alpha_composite(images[order[0]], images[order[1]])
            crop = composite.crop((args.viewport_x, args.viewport_y,
                                   args.viewport_x + 240, args.viewport_y + 160))
            stem = f"{case}_{order[0]}_under_{order[1]}"
            crop.save(args.out / f"{stem}.png")
            guide = crop.copy()
            pen = ImageDraw.Draw(guide)
            for rect in REGIONS:
                pen.rectangle((rect.x, rect.y, rect.right - 1, rect.bottom - 1),
                              outline=(255, 0, 255, 255), width=1)
            # The cross marks only the anchor, not the species-dependent sprite bounds.
            pen.line((36, 104, 44, 104), fill=(0, 255, 0, 255))
            pen.line((40, 100, 40, 108), fill=(0, 255, 0, 255))
            guide.save(args.out / f"{stem}_occupancy.png")
            print(f"Review only: {stem}.png and {stem}_occupancy.png")
    print("These diagnostics do NOT establish verified retail layer ordering,")
    print("DS background transparency, scroll positions or approval for live wiring.")


if __name__ == "__main__":
    main()
