#!/usr/bin/env python3
"""Build the interactive 4bpp HGSS Search overlay from authentic member 068.

BG3 already carries the exact 8bpp member-068 screen. BG1 duplicates those
pixels in 4bpp so the existing search highlight system can swap palette banks:
  bank 4: selected
  bank 5: normal (exact HGSS colors)
  bank 6: selected + disabled
  bank 7: disabled

Pixel index 0 is avoided by shifting the 12 authentic source colors to 1..12.
"""

import argparse
import struct

from make_hgss_pokedex_search import PALETTE, build_layout

OUT_WIDTH = 240
OUT_HEIGHT = 160
MAP_WIDTH = 32
MAP_HEIGHT = 32
BASE_TILE = 0
SELECTED_BANK = 4
NORMAL_BANK = 5
DISABLED_SELECTED_BANK = 6
DISABLED_BANK = 7


def build_overlay():
    source = build_layout()
    return [bytearray(pixel + 1 for pixel in row) for row in source]


def flip_h(tile):
    return tuple(tile[y * 8 + x] for y in range(8) for x in range(7, -1, -1))


def flip_v(tile):
    return tuple(tile[y * 8 + x] for y in range(7, -1, -1) for x in range(8))


def get_tile(rows, tx, ty):
    tile = []
    for y in range(8):
        py = ty * 8 + y
        for x in range(8):
            px = tx * 8 + x
            if py < OUT_HEIGHT and px < OUT_WIDTH:
                tile.append(rows[py][px])
            else:
                tile.append(0)
    return tuple(tile)


def pack(rows):
    blank = (0,) * 64
    unique = [blank]
    lookup = {blank: 0}
    tilemap = []

    for ty in range(MAP_HEIGHT):
        for tx in range(MAP_WIDTH):
            tile = get_tile(rows, tx, ty)
            variants = (
                (tile, 0),
                (flip_h(tile), 1 << 10),
                (flip_v(tile), 1 << 11),
                (flip_h(flip_v(tile)), (1 << 10) | (1 << 11)),
            )

            entry = None
            for candidate, flags in variants:
                index = lookup.get(candidate)
                if index is not None:
                    entry = (BASE_TILE + index) | flags | (NORMAL_BANK << 12)
                    break

            if entry is None:
                index = len(unique)
                if index >= 1024:
                    raise SystemExit("HGSS Search overlay exceeds text-BG tile limit")
                unique.append(tile)
                lookup[tile] = index
                entry = (BASE_TILE + index) | (NORMAL_BANK << 12)

            tilemap.append(entry)

    return unique, tilemap


def encode_4bpp(tile):
    out = bytearray()
    for i in range(0, 64, 2):
        out.append((tile[i] & 0xF) | ((tile[i + 1] & 0xF) << 4))
    return bytes(out)


def rgb555(rgb):
    r, g, b = rgb
    return (
        min(31, (r + 4) // 8)
        | (min(31, (g + 4) // 8) << 5)
        | (min(31, (b + 4) // 8) << 10)
    )


def selected(rgb):
    # Preserve the HGSS hue but raise contrast/brightness for cursor focus.
    return tuple(min(255, int(channel * 1.12) + 14) for channel in rgb)


def disabled(rgb):
    r, g, b = rgb
    gray = (r * 3 + g * 6 + b) // 10
    return tuple((channel + gray * 2) // 3 for channel in (r, g, b))


def build_bank(colors):
    shifted = [(0, 0, 0)] + colors
    shifted += [(0, 0, 0)] * (16 - len(shifted))
    return b"".join(struct.pack("<H", rgb555(color)) for color in shifted)


def build_palette():
    banks = [
        [selected(color) for color in PALETTE],
        list(PALETTE),
        [disabled(selected(color)) for color in PALETTE],
        [disabled(color) for color in PALETTE],
    ]
    return b"".join(build_bank(bank) for bank in banks)


def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--tiles", action="store_true")
    group.add_argument("--tilemap", action="store_true")
    group.add_argument("--palette", action="store_true")
    parser.add_argument("output")
    args = parser.parse_args()

    rows = build_overlay()
    tiles, tilemap = pack(rows)

    if args.tiles:
        data = b"".join(encode_4bpp(tile) for tile in tiles)
        label = "4bpp tiles"
    elif args.tilemap:
        data = b"".join(struct.pack("<H", entry) for entry in tilemap)
        label = "tilemap"
    else:
        data = build_palette()
        label = "4-bank palette"

    with open(args.output, "wb") as f:
        f.write(data)

    print(
        f"{args.output}: authentic HGSS Search overlay {label}, "
        f"{len(tiles)} unique tiles, banks {SELECTED_BANK}-{DISABLED_BANK}"
    )


if __name__ == "__main__":
    main()
