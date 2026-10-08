#!/usr/bin/env python3
"""Prepare authentic HGSS OBJ replacements for the shared PokéNav list.

Phase 1 only: no runtime binding or legacy deletion. Keep the existing six-tile
layout: 8x16 right pointer, 16x8 down arrow, 16x8 up arrow. Pointer pixels come
from verified Pokédex member 057; scroll pixels come from verified member 000.
Only palette-index remapping and vertical reflection are performed.
"""

import argparse
import struct

from make_hgss_pokedex_start_cursor import build_tiles as pointer_tiles
from make_hgss_pokedex_start_cursor import PALETTE as POINTER_PALETTE
from make_hgss_pokedex_scroll_controls import build_tiles as scroll_tiles
from make_hgss_pokedex_scroll_controls import PALETTE as SCROLL_PALETTE
from make_hgss_pokedex_scroll_controls import rgb555


def remap_tiles(data, mapping):
    out = bytearray()
    for value in data:
        lo, hi = value & 15, value >> 4
        if lo not in mapping or hi not in mapping:
            raise SystemExit("unexpected source OBJ palette index")
        out.append(mapping[lo] | (mapping[hi] << 4))
    return bytes(out)


def flip_vertical_16x8(data):
    if len(data) != 64:
        raise SystemExit("expected two tiles for the 16x8 HGSS scroll arrow")
    # Reverse scanlines inside both horizontal tiles, preserving tile order.
    return b"".join(
        data[tile + row * 4:tile + row * 4 + 4]
        for tile in (0, 32)
        for row in range(7, -1, -1)
    )


def build_tiles():
    # Both source builders validate their complete retail-derived rasters;
    # the scroll builder also validates the extracted arrow's pixel checksum.
    pointer = remap_tiles(pointer_tiles(), {0: 0, 1: 1})
    up = remap_tiles(scroll_tiles()[:64], {0: 0, 1: 2, 2: 3, 3: 4})
    down = flip_vertical_16x8(up)
    if len(pointer) != 64 or flip_vertical_16x8(down) != up:
        raise SystemExit("invalid HGSS list-arrow geometry")
    return pointer + down + up


def build_palette():
    colors = [(0, 0, 0), POINTER_PALETTE[2], SCROLL_PALETTE[3],
              SCROLL_PALETTE[4], SCROLL_PALETTE[5]]
    colors += [(0, 0, 0)] * (16 - len(colors))
    return b"".join(struct.pack("<H", rgb555(color)) for color in colors)


def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--tiles", action="store_true")
    group.add_argument("--palette", action="store_true")
    parser.add_argument("output")
    args = parser.parse_args()
    data = build_tiles() if args.tiles else build_palette()
    with open(args.output, "wb") as output:
        output.write(data)
    print(f"{args.output}: authentic HGSS shared list arrows, {len(data)} bytes")


if __name__ == "__main__":
    main()
