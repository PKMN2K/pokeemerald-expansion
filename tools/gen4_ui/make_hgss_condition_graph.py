#!/usr/bin/env python3
"""Adapt verified HGSS panels/font to the existing PokéNav condition chart.

Chrome and labels retain retail pixels. The reference pentagon/axes are
mathematical chart guides, not claimed to be HGSS artwork. The game still
draws the live data polygon with its existing scanline renderer.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from PIL import Image
from make_hgss_ribbon_summary import ASSETS, validate_assets
from make_hgss_pokegear_condition_search import (
    FONT_ID4, decode_font_id4, decode_glyph, hgss_charcode, pack_bitmap_4bpp,
)

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "graphics/gen4_ui/hgss_condition"
STORAGE = ROOT / "graphics/gen4_ui/hgss_storage/verified"
MAX_POINTS = ((155, 56), (189, 80), (176, 119), (135, 119), (122, 80))
CENTER = (155, 91)
LABELS = (("Cool", 142, 40), ("Beauty", 192, 64), ("Cute", 192, 109),
          ("Smart", 87, 109), ("Tough", 84, 64))


def digest(data):
    return hashlib.sha256(data).hexdigest()


def original_png(name, record, key="export"):
    path = STORAGE / name
    manifest = json.loads((STORAGE / record).read_text())
    assert digest(path.read_bytes()) == manifest[key]["sha256"], name
    image = Image.open(path)
    assert image.mode == "P", name
    return image


def build():
    validate_assets()
    tiles = (ASSETS / "summary_bg.4bpp").read_bytes()
    source = struct.unpack("<1024H", (ASSETS / "summary_bg.bin").read_bytes())
    pixels = [[1] * 240 for _ in range(160)]
    banks = [[1] * 30 for _ in range(20)]

    def cell(x, y, sx, sy, flip=0):
        attr = source[sy * 32 + sx] ^ flip
        banks[y][x] = attr >> 12
        for yy in range(8):
            for xx in range(8):
                px = 7 - xx if attr & 0x400 else xx
                py = 7 - yy if attr & 0x800 else yy
                value = tiles[(attr & 1023) * 32 + py * 4 + px // 2]
                pixels[y * 8 + yy][x * 8 + xx] = (value >> (4 * (px % 2))) & 15

    for y in range(3):
        for x in range(30):
            cell(x, y, x, y)

    def panel(x, y, width, height):
        # Source-adapted summary portrait card corners and edge/interior.
        for yy in range(height):
            for xx in range(width):
                cell(x + xx, y + yy, 0 if xx == 0 else 9 if xx == width - 1 else 2,
                     4 if yy == 0 else 7 if yy == height - 1 else 5)

    panel(0, 3, 30, 2)   # Name, gender/level and location, y=24..39.
    panel(0, 5, 10, 11)  # Portrait; a 64px mon centered at (40,80).
    panel(10, 5, 20, 11) # Native chart coordinates, fully above help bar.

    def line(a, b, color):
        # Bresenham guides use the same max-condition vertices as the game.
        x, y = a
        dx, dy = abs(b[0] - x), -abs(b[1] - y)
        sx, sy = (1 if x < b[0] else -1), (1 if y < b[1] else -1)
        err = dx + dy
        while True:
            pixels[y][x] = color
            if (x, y) == b:
                break
            e2 = err * 2
            if e2 >= dy:
                err += dy
                x += sx
            if e2 <= dx:
                err += dx
                y += sy

    for point in MAX_POINTS:
        line(CENTER, point, 14)
    for a, b in zip(MAX_POINTS, MAX_POINTS[1:] + MAX_POINTS[:1]):
        line(a, b, 3)

    data, widths = decode_font_id4()
    assert hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest() == "a4b7623e8061d2a2867510bd5d8b4a7f5db11bf2"
    for label, x, y in LABELS:
        for char in label:
            glyph, advance = decode_glyph(data, widths, hgss_charcode(char))
            for yy in range(16):
                for xx in range(16):
                    if glyph[yy][xx]:
                        assert banks[(y + yy) // 8][(x + xx) // 8] == 1
                        pixels[y + yy][x + xx] = 5 if glyph[yy][xx] == 1 else 2
            x += advance

    unique = [bytes(32)]
    tile_ids = {unique[0]: 0}
    screen = [0] * 1024
    for y in range(20):
        for x in range(30):
            tile = pack_bitmap_4bpp([row[x * 8:x * 8 + 8] for row in pixels[y * 8:y * 8 + 8]])
            if tile not in tile_ids:
                tile_ids[tile] = len(unique)
                unique.append(tile)
            screen[y * 32 + x] = tile_ids[tile] | banks[y][x] << 12
    assert len(unique) <= 512

    # Existing sheen animation uses seven 16x16 frames. Center the original
    # 8x8 HGSS Summary star without scaling; blank endpoints blink it.
    star = original_png("shiny_star.png", "shiny_indicator.json")
    star_rows = []
    for frame in range(7):
        canvas = [[0] * 16 for _ in range(16)]
        if frame not in (0, 6):
            for y in range(8):
                for x in range(8):
                    canvas[y + 4][x + 4] = star.getpixel((x, y))
        star_rows.extend(canvas)

    marks = original_png("markings_menu.png", "markings_menu.json")
    # The same complete source rows used by Storage: four saved marking
    # bits, followed by the original two action bars. The omitted third
    # symbol row contains star/diamond, which this engine does not persist.
    popup = [[marks.getpixel((x, y)) for x in range(88)]
             for y in list(range(56)) + list(range(80, 144))]
    for label, y in (("OK", 64), ("Cancel", 96)):
        glyphs = [decode_glyph(data, widths, hgss_charcode(c)) for c in label]
        x = (88 - sum(advance for _, advance in glyphs)) // 2
        for glyph, advance in glyphs:
            for yy in range(16):
                for xx in range(16):
                    if glyph[yy][xx]:
                        popup[y + yy][x + xx] = 8 if glyph[yy][xx] == 1 else 7
            x += advance
    outputs = {
        "graph_bg.4bpp": b"".join(unique),
        "graph_bg.bin": struct.pack("<1024H", *screen),
        "sheen_stars.4bpp": pack_bitmap_4bpp(star_rows),
        "markings_panel.4bpp": pack_bitmap_4bpp(popup),
    }
    manifest = {
        "panels": "../hgss_ribbons/summary_bg.source.json",
        "panel_manifest_sha256": digest((ASSETS / "summary_bg.source.json").read_bytes()),
        "font": "../hgss_pokegear/verified/font_id4.bin",
        "font_sha256": digest(data),
        "labels": LABELS,
        "guides": {"kind": "mathematical graph reference, not retail artwork", "center": CENTER,
                   "max_condition_vertices": MAX_POINTS, "palette_indices": [3, 14]},
        "sheen": "../hgss_storage/verified/shiny_indicator.json",
        "markings": "../hgss_storage/verified/markings_menu.json",
        "chrome_rule": "Original summary cells and font glyphs at native size; no repainting or recoloring",
        "palette_banks": [1, 7],
        "outputs": {name: digest(value) for name, value in outputs.items()},
    }
    outputs["source.json"] = (json.dumps(manifest, indent=2) + "\n").encode()
    return outputs


def validate():
    for name, data in build().items():
        assert (OUT / name).read_bytes() == data, f"Stale HGSS Condition asset: {name}"
    print("Verified original HGSS Condition panels/font/star and native chart guides")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.check:
        validate()
    else:
        OUT.mkdir(parents=True, exist_ok=True)
        for name, data in build().items():
            (OUT / name).write_bytes(data)
        print(f"Wrote verified Condition screen assets to {OUT}")
