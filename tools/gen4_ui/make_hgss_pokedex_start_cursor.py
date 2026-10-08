#!/usr/bin/env python3
"""Build the authentic HGSS Pokédex START-menu cursor from member 057.

The cursor keeps the existing GBA 8x16 OBJ geometry so the live menu's
positions, 16-pixel row spacing, and horizontal bob animation remain unchanged.
Only the visible pointer pixels are replaced: they are copied 1:1 from the
lavender HGSS member-057 list pointer at source coordinates x=120..124,
y=12..19. The adjacent source rule at x=127 is intentionally excluded.

Tile layout:
  tile 0: upper half of the 8x16 cursor OBJ
  tile 1: lower half of the 8x16 cursor OBJ
"""

import argparse
import struct

from make_hgss_pokedex_stats import PALETTE, decode_member

SOURCE_X = 120
SOURCE_Y = 12
SOURCE_INDEX = 2
POINTER_WIDTHS = (2, 3, 4, 5, 5, 4, 3, 2)
OBJ_WIDTH = 8
OBJ_HEIGHT = 16
POINTER_Y = 4


def encode_tile_4bpp(pixels):
    if len(pixels) != 64:
        raise SystemExit(f"tile has {len(pixels)} pixels, expected 64")
    out = bytearray()
    for i in range(0, 64, 2):
        out.append((pixels[i] & 0xF) | ((pixels[i + 1] & 0xF) << 4))
    return bytes(out)


def rgb555(rgb):
    r, g, b = rgb
    return (
        min(31, (r + 4) // 8)
        | (min(31, (g + 4) // 8) << 5)
        | (min(31, (b + 4) // 8) << 10)
    )


def build_pixels():
    source = decode_member(57)
    out = [0] * (OBJ_WIDTH * OBJ_HEIGHT)

    for row, width in enumerate(POINTER_WIDTHS):
        source_y = SOURCE_Y + row
        dest_y = POINTER_Y + row
        for x in range(width):
            source_x = SOURCE_X + x
            source_index = source[source_y][source_x]
            if source_index != SOURCE_INDEX:
                raise SystemExit(
                    f"unexpected member-057 cursor color {source_index} "
                    f"at {source_x},{source_y}"
                )
            out[dest_y * OBJ_WIDTH + x] = 1

    return out


def build_tiles():
    pixels = build_pixels()
    top = pixels[:64]
    bottom = pixels[64:]
    return encode_tile_4bpp(top) + encode_tile_4bpp(bottom)


def build_palette():
    colors = [
        (0, 0, 0),      # transparent
        PALETTE[2],     # authentic member-057 lavender cursor
    ]
    colors += [(0, 0, 0)] * (16 - len(colors))
    return b"".join(struct.pack("<H", rgb555(color)) for color in colors)


def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--tiles", action="store_true")
    group.add_argument("--palette", action="store_true")
    parser.add_argument("output")
    args = parser.parse_args()

    if args.tiles:
        data = build_tiles()
        label = "2 OBJ tiles"
    else:
        data = build_palette()
        label = "16-color OBJ palette"

    with open(args.output, "wb") as f:
        f.write(data)

    print(f"{args.output}: authentic HGSS member-057 START cursor, {label}")


if __name__ == "__main__":
    main()
