#!/usr/bin/env python3
"""Extract authentic HGSS Pokédex list chrome for PokéGear Condition/Search.

Retail HGSS has no PokéNav Condition screen. This adapter deliberately reuses
the authentic HGSS Pokédex list/search presentation instead of drawing an
"HGSS-style" replacement.

Visible source pixels are taken only from the checksum-verified rendered
Pokédex member-057 source already used by the HGSS Pokédex pipeline:
  - 8x8 pale-blue list interior at (32, 8)
  - 8x8 orange/purple list rule at (32, 24)

The selection pointer is the exact member-057 lavender pointer already verified
by make_hgss_pokedex_start_cursor.py. No selected/disabled recolor is created.
"""

import argparse
from pathlib import Path
import struct

from make_hgss_pokedex_stats import PALETTE, decode_member
from make_hgss_pokedex_start_cursor import (
    build_palette as build_cursor_palette,
    build_tiles as build_cursor_tiles,
)

SOURCE_MEMBER = 57
FILL_X = 32
FILL_Y = 8
RULE_X = 32
RULE_Y = 24
TILE_SIZE = 8

FONT_ID4 = Path("graphics/gen4_ui/hgss_pokegear/verified/font_id4.bin")
FONT_HEADER = (16, 32592, 509, 16, 16, 2, 2)
FONT_GLYPH_SIZE = 64
LABEL_WIDTH = 88
LABEL_HEIGHT = 16
LABELS = ("Party", "Search", "Cancel", "Cool", "Beauty", "Cute", "Smart", "Tough")


def extract_tile(rows, x0, y0):
    tile = []
    for y in range(TILE_SIZE):
        for x in range(TILE_SIZE):
            value = rows[y0 + y][x0 + x]
            if value >= 16:
                raise SystemExit(
                    f"member {SOURCE_MEMBER:03d}: 4bpp source index {value} "
                    f"at {x0 + x},{y0 + y}"
                )
            tile.append(value)
    if len(tile) != 64:
        raise SystemExit("unexpected tile size")
    return tile


def encode_4bpp(tile):
    out = bytearray()
    for i in range(0, 64, 2):
        out.append((tile[i] & 0xF) | ((tile[i + 1] & 0xF) << 4))
    return bytes(out)


def build_chrome():
    # decode_member verifies the complete retail-derived member-057 raster
    # against SHA-256 before any pixels are exposed here.
    rows = decode_member(SOURCE_MEMBER)
    fill = extract_tile(rows, FILL_X, FILL_Y)
    rule = extract_tile(rows, RULE_X, RULE_Y)

    if fill == rule:
        raise SystemExit("HGSS list fill/rule source tiles unexpectedly match")

    # Tile 0 = retail list interior; tile 1 = retail divider/border rule.
    # Pixel indices are preserved exactly; no +1 transparency shift is used.
    return encode_4bpp(fill) + encode_4bpp(rule)


def rgb555(rgb):
    r, g, b = rgb
    return (
        min(31, (r + 4) // 8)
        | (min(31, (g + 4) // 8) << 5)
        | (min(31, (b + 4) // 8) << 10)
    )


def build_chrome_palette():
    if len(PALETTE) != 15:
        raise SystemExit(f"expected 15 authentic HGSS colors, got {len(PALETTE)}")

    # Source indices 0..14 keep their original ordering. Entry 15 is unused.
    colors = list(PALETTE) + [(0, 0, 0)]
    return b"".join(struct.pack("<H", rgb555(color)) for color in colors)


def decode_font_id4():
    data = FONT_ID4.read_bytes()
    if len(data) != 33101:
        raise SystemExit(f"unexpected font-ID-4 size: {len(data)}")

    header_size, width_data_start, num_glyphs = struct.unpack_from("<III", data, 0)
    fixed_width, fixed_height, glyph_width, glyph_height = data[12:16]
    observed = (
        header_size,
        width_data_start,
        num_glyphs,
        fixed_width,
        fixed_height,
        glyph_width,
        glyph_height,
    )
    if observed != FONT_HEADER:
        raise SystemExit(f"unexpected font-ID-4 header: {observed!r}")

    widths = data[width_data_start:width_data_start + num_glyphs]
    if len(widths) != num_glyphs:
        raise SystemExit("truncated font-ID-4 width table")
    return data, widths


def hgss_charcode(ch):
    if "A" <= ch <= "Z":
        return 299 + ord(ch) - ord("A")
    if "a" <= ch <= "z":
        return 325 + ord(ch) - ord("a")
    if ch == " ":
        return 478
    raise SystemExit(f"unsupported HGSS label character {ch!r}")


def decode_font_tile(tile):
    if len(tile) != 16:
        raise SystemExit("font tile is not 16 bytes")

    rows = []
    for y in range(8):
        lo = tile[y * 2]
        hi = tile[y * 2 + 1]
        row = []
        for value in (hi, lo):
            row.extend(
                (
                    (value >> 6) & 3,
                    (value >> 4) & 3,
                    (value >> 2) & 3,
                    value & 3,
                )
            )
        rows.append(row)
    return rows


def decode_glyph(data, widths, charcode):
    glyph = charcode - 1
    if glyph < 0 or glyph >= FONT_HEADER[2]:
        raise SystemExit(f"font glyph {charcode} out of range")

    start = FONT_HEADER[0] + glyph * FONT_GLYPH_SIZE
    raw = data[start:start + FONT_GLYPH_SIZE]
    if len(raw) != FONT_GLYPH_SIZE:
        raise SystemExit(f"truncated glyph {charcode}")

    out = [[0] * 16 for _ in range(16)]
    for tile_index in range(4):
        tile = decode_font_tile(raw[tile_index * 16:(tile_index + 1) * 16])
        ox = (tile_index & 1) * 8
        oy = (tile_index >> 1) * 8
        for y in range(8):
            for x in range(8):
                source_class = tile[y][x]
                out[oy + y][ox + x] = source_class if source_class in (1, 2) else 0

    return out, widths[glyph]


def pack_bitmap_4bpp(rows):
    height = len(rows)
    width = len(rows[0])
    if width % 8 or height % 8:
        raise SystemExit(f"4bpp bitmap must be tile-aligned, got {width}x{height}")

    out = bytearray()
    for ty in range(0, height, 8):
        for tx in range(0, width, 8):
            for y in range(8):
                for x in range(0, 8, 2):
                    lo = rows[ty + y][tx + x] & 0xF
                    hi = rows[ty + y][tx + x + 1] & 0xF
                    out.append(lo | (hi << 4))
    return bytes(out)


def build_labels():
    data, widths = decode_font_id4()
    atlas = bytearray()

    for label in LABELS:
        canvas = [[0] * LABEL_WIDTH for _ in range(LABEL_HEIGHT)]
        x = 0
        for ch in label:
            glyph, advance = decode_glyph(data, widths, hgss_charcode(ch))
            for y in range(LABEL_HEIGHT):
                for gx in range(16):
                    if x + gx >= LABEL_WIDTH:
                        continue
                    value = glyph[y][gx]
                    if value:
                        canvas[y][x + gx] = value
            x += advance

        if x > LABEL_WIDTH:
            raise SystemExit(f"HGSS label {label!r} is {x}px wide, exceeds {LABEL_WIDTH}px")
        atlas.extend(pack_bitmap_4bpp(canvas))

    return bytes(atlas)


def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--chrome", action="store_true")
    group.add_argument("--palette", action="store_true")
    group.add_argument("--cursor", action="store_true")
    group.add_argument("--cursor-palette", action="store_true")
    group.add_argument("--labels", action="store_true")
    parser.add_argument("output")
    args = parser.parse_args()

    if args.chrome:
        data = build_chrome()
        label = "two exact 8x8 member-057 chrome tiles"
    elif args.palette:
        data = build_chrome_palette()
        label = "exact member-057 source-color palette"
    elif args.cursor:
        data = build_cursor_tiles()
        label = "exact member-057 lavender pointer pixels"
    elif args.cursor_palette:
        data = build_cursor_palette()
        label = "authentic member-057 lavender pointer palette"
    else:
        data = build_labels()
        label = "exact retail HGSS font-ID-4 glyph label atlas"

    with open(args.output, "wb") as f:
        f.write(data)

    print(f"{args.output}: authentic HGSS Condition/Search source, {label}")


if __name__ == "__main__":
    main()
