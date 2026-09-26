#!/usr/bin/env python3
"""Build the authentic HGSS sparse BG1 overlay for the Pokédex list screen.

The live list text is drawn on BG2 and the Pokémon/stat elements are sprites.
This overlay therefore keeps only safe member-000 pixels:
- the left 8-pixel rail
- the right-side authentic frame/counter region from x=108 onward

The dynamic species-list column stays transparent.
"""

import argparse
import hashlib
import struct

from make_hgss_pokedex_list_member_000 import PALETTE, build_gba_layout, decode_source

OUT_WIDTH = 240
OUT_HEIGHT = 160
MAP_WIDTH = 32
MAP_HEIGHT = 32
BASE_TILE = 512
PALETTE_BANK = 11
EXPECTED_PIXEL_SHA256 = "e2069ed5787a2278d38e2c21fd125a480a34a2f29ca9309c5a3f09921d50ab1c"
EXPECTED_TILE_COUNT = 50

# Source indices 0..12 are the only colors used by the retained safe regions.
SOURCE_TO_OVERLAY = {source: source + 1 for source in range(13)}


def build_overlay():
    source = build_gba_layout(decode_source())
    out = [bytearray(OUT_WIDTH) for _ in range(OUT_HEIGHT)]

    for y in range(OUT_HEIGHT):
        for x in range(OUT_WIDTH):
            if x < 8 or x >= 108:
                source_index = source[y][x]
                mapped = SOURCE_TO_OVERLAY.get(source_index)
                if mapped is None:
                    raise SystemExit(
                        f"unexpected list-overlay source color {source_index} at {x},{y}"
                    )
                out[y][x] = mapped

    flat = b"".join(bytes(row) for row in out)
    if hashlib.sha256(flat).hexdigest() != EXPECTED_PIXEL_SHA256:
        raise SystemExit("HGSS list overlay pixel checksum mismatch")
    return out


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
                    entry = (BASE_TILE + index) | flags | (PALETTE_BANK << 12)
                    break

            if entry is None:
                index = len(unique)
                unique.append(tile)
                lookup[tile] = index
                entry = (BASE_TILE + index) | (PALETTE_BANK << 12)

            tilemap.append(entry)

    if len(unique) != EXPECTED_TILE_COUNT:
        raise SystemExit(
            f"unexpected HGSS list overlay tile count: {len(unique)} "
            f"(expected {EXPECTED_TILE_COUNT})"
        )

    if BASE_TILE + len(unique) > 768:
        raise SystemExit("HGSS list overlay collides with START-menu tile range")

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


def build_palette():
    # BG palette index 0 is transparent. Authentic member-000 source colors
    # 0..12 are moved to indices 1..13.
    colors = [(0, 0, 0)] + list(PALETTE[:13]) + [(0, 0, 0)] * 2
    return b"".join(struct.pack("<H", rgb555(color)) for color in colors)


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
        label = "palette"

    with open(args.output, "wb") as f:
        f.write(data)

    print(
        f"{args.output}: authentic HGSS list overlay {label}, "
        f"{len(tiles)} unique tiles, base tile {BASE_TILE}, palette bank {PALETTE_BANK}"
    )


if __name__ == "__main__":
    main()
