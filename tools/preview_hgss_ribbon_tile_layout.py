#!/usr/bin/env python3
"""Non-destructive, 8x8 tile-aligned authentic HGSS Ribbons layout previews.

Requires original a/1/6/2 NARC and Pillow; no original artwork is bundled.
Diagnostic output only, not approved GBA tilemaps or live game resources.
"""
import argparse
from pathlib import Path
from PIL import ImageDraw
from render_hgss_ribbon_backgrounds import narc_members, palette_banks, render
from audit_hgss_ribbon_gba_layout import REGIONS

TRIALS = {
    "header_footer": ((0, 0, 240, 24), (0, 128, 240, 32)),
    "left_case": ((0, 0, 80, 32), (0, 56, 80, 40), (0, 128, 240, 32)),
    "header_only": ((0, 0, 240, 24),),
}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("narc", type=Path)
    p.add_argument("out", type=Path)
    p.add_argument("--viewport-x", type=int, default=0)
    p.add_argument("--viewport-y", type=int, default=0)
    args = p.parse_args()
    if not (0 <= args.viewport_x <= 16 and 0 <= args.viewport_y <= 96):
        p.error("240x160 viewport must fit the 256x256 source map")
    parts = narc_members(args.narc.read_bytes())
    if len(parts) != 79:
        raise ValueError("Expected 79 original NARC members")
    palettes = palette_banks(parts[0])
    bg3 = render(parts[2], parts[10], palettes, 256)
    bg6 = render(parts[1], parts[9], palettes, 512)
    bounds = (args.viewport_x, args.viewport_y,
              args.viewport_x + 240, args.viewport_y + 160)
    base, case = bg3.crop(bounds), bg6.crop(bounds)
    protected = [r for r in REGIONS if "frame" not in r.name]
    args.out.mkdir(parents=True, exist_ok=True)
    for label, regions in TRIALS.items():
        result = base.copy()
        n = 0
        for x, y, w, h in regions:
            if any(v % 8 for v in (x, y, w, h)):
                raise AssertionError("Selection must be 8x8 tile aligned")
            for ty in range(y, y+h, 8):
                for tx in range(x, x+w, 8):
                    if any(tx < r.right and tx+8 > r.x and
                           ty < r.bottom and ty+8 > r.y for r in protected):
                        continue
                    tile = case.crop((tx, ty, tx+8, ty+8))
                    result.paste(tile, (tx, ty), tile)
                    n += 1
        result.save(args.out / f"{label}.png")
        guide = result.copy()
        draw = ImageDraw.Draw(guide)
        for r in REGIONS:
            draw.rectangle((r.x, r.y, r.right-1, r.bottom-1),
                           outline=(255, 0, 255, 255))
        draw.line((36,104,44,104), fill=(0,255,0,255))
        draw.line((40,100,40,108), fill=(0,255,0,255))
        guide.save(args.out / f"{label}_guide.png")
        print(f"{label}: {n} original BG6 tile positions composited")
    print("Review-only: DS scroll, priority, and GBA palette allocation unverified.")

if __name__ == "__main__":
    main()
