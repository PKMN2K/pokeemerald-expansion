#!/usr/bin/env python3
"""Non-destructive, 8x8 tile-aligned authentic HGSS Ribbons layout previews.

Requires original a/1/6/2 NARC and Pillow; no original artwork is bundled.
Diagnostic output only, not approved GBA tilemaps or live game resources.
"""
import argparse
import json
from pathlib import Path
from render_hgss_ribbon_backgrounds import narc_members, palette_banks, render
from audit_hgss_ribbon_gba_layout import REGIONS

TRIALS = {
    "header_footer": ((0, 0, 240, 24), (0, 128, 240, 32)),
    "left_case": ((0, 0, 80, 32), (0, 56, 80, 40), (0, 128, 240, 32)),
    "header_only": ((0, 0, 240, 24),),
}

def main():
    from PIL import Image, ImageDraw
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("narc", type=Path)
    p.add_argument("out", type=Path)
    p.add_argument("--viewport-x", type=int, default=0)
    p.add_argument("--viewport-y", type=int, default=0)
    p.add_argument("--bg3-nscr", type=int, choices=(10, 11), default=10,
                   help="Retail BG3 screen-map member to preview")
    p.add_argument("--bg6-nscr", type=int, choices=(9, 78), default=9,
                   help="Retail BG6 screen-map member to preview")
    p.add_argument("--all-source-pairs", action="store_true",
                   help="Compare all four verified BG3/BG6 NSCR pairings")
    args = p.parse_args()
    if not (0 <= args.viewport_x <= 16 and 0 <= args.viewport_y <= 96):
        p.error("240x160 viewport must fit the 256x256 source map")
    if args.viewport_x % 8 or args.viewport_y % 8:
        p.error("Viewport origin must be 8x8 tile aligned for source-faithful BG composition")
    # Reject unverified source archives before generating composition previews.
    # Import here to avoid a top-level dependency cycle with the exporter.
    from export_hgss_ribbon_gba_bg import verify_source_narc
    archive = args.narc.read_bytes()
    verify_source_narc(archive)
    parts = narc_members(archive)
    if len(parts) != 79:
        raise ValueError("Expected 79 original NARC members")
    palettes = palette_banks(parts[0])
    pairings = ((a, b) for a in (10, 11) for b in (9, 78)) if args.all_source_pairs else ((args.bg3_nscr, args.bg6_nscr),)
    bounds = (args.viewport_x, args.viewport_y,
              args.viewport_x + 240, args.viewport_y + 160)
    protected = [r for r in REGIONS if "frame" not in r.name]
    args.out.mkdir(parents=True, exist_ok=True)
    # Collect previews in deterministic order for side-by-side retail review.
    contact = []
    # Distinct names prevent candidate combinations overwriting one another.
    for bg3_nscr, bg6_nscr in pairings:
        bg3 = render(parts[2], parts[bg3_nscr], palettes, 256)
        bg6 = render(parts[1], parts[bg6_nscr], palettes, 512)
        base, case = bg3.crop(bounds), bg6.crop(bounds)
        source_label = f"bg3_{bg3_nscr}_bg6_{bg6_nscr}_x{args.viewport_x}_y{args.viewport_y}"
        manifest = []
        for label, regions in TRIALS.items():
            result = base.copy()
            n = 0
            protected_skips = 0
            transparent_skips = 0
            visible_bg6_pixels = 0
            for x, y, w, h in regions:
                if any(v % 8 for v in (x, y, w, h)):
                    raise AssertionError("Selection must be 8x8 tile aligned")
                for ty in range(y, y+h, 8):
                    for tx in range(x, x+w, 8):
                        if any(tx < r.right and tx+8 > r.x and
                               ty < r.bottom and ty+8 > r.y for r in protected):
                            protected_skips += 1
                            continue
                        tile = case.crop((tx, ty, tx+8, ty+8))
                        visible = tile.getchannel("A").getbbox()
                        if visible is None:
                            transparent_skips += 1
                            continue
                        visible_bg6_pixels += sum(1 for alpha in tile.getchannel("A").getdata() if alpha)
                        result.paste(tile, (tx, ty), tile)
                        n += 1
            result.save(args.out / f"{source_label}_{label}.png")
            contact.append((f"{source_label}_{label}", result.copy()))
            guide = result.copy()
            draw = ImageDraw.Draw(guide)
            for r in REGIONS:
                draw.rectangle((r.x, r.y, r.right-1, r.bottom-1),
                               outline=(255, 0, 255, 255))
            draw.line((36,104,44,104), fill=(0,255,0,255))
            draw.line((40,100,40,108), fill=(0,255,0,255))
            guide.save(args.out / f"{source_label}_{label}_guide.png")
            manifest.append({"candidate": f"{source_label}_{label}",
                             "bg3_nscr": bg3_nscr, "bg6_nscr": bg6_nscr,
                             "viewport_x": args.viewport_x, "viewport_y": args.viewport_y,
                             "trial": label, "bg6_tiles_applied": n,
                             "protected_tiles_skipped": protected_skips,
                             "fully_transparent_bg6_tiles_skipped": transparent_skips,
                             "visible_bg6_pixels_applied": visible_bg6_pixels})
            print(f"{source_label}_{label}: {n} visible BG6 tiles, "
                  f"{visible_bg6_pixels} opaque pixels composited")
        (args.out / f"{source_label}_manifest.json").write_text(
            json.dumps({"purpose": "source-only preview, not verified retail layer ordering",
                        "candidates": manifest}, indent=2) + "\n")
    # Place all generated alternatives in one image, without modifying pixels.
    # Labels occupy a separate margin, not the original BG artwork.
    columns = len(TRIALS)
    rows = (len(contact) + columns - 1) // columns
    sheet = Image.new("RGBA", (columns * 240, rows * 184), (0, 0, 0, 255))
    label_draw = ImageDraw.Draw(sheet)
    for index, (name, preview) in enumerate(contact):
        x, y = (index % columns) * 240, (index // columns) * 184
        sheet.paste(preview, (x, y))
        label_draw.text((x + 3, y + 164), name, fill=(255, 255, 255, 255))
    sheet.save(args.out / "candidate_contact_sheet.png")
    print("Review-only: DS scroll, priority, and GBA palette allocation unverified.")

if __name__ == "__main__":
    main()
