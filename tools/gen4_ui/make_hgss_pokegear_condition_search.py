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


def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--chrome", action="store_true")
    group.add_argument("--palette", action="store_true")
    group.add_argument("--cursor", action="store_true")
    group.add_argument("--cursor-palette", action="store_true")
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
    else:
        data = build_cursor_palette()
        label = "authentic member-057 lavender pointer palette"

    with open(args.output, "wb") as f:
        f.write(data)

    print(f"{args.output}: authentic HGSS Condition/Search source, {label}")


if __name__ == "__main__":
    main()
