#!/usr/bin/env python3
"""Build a transparent HGSS chrome overlay for the live Pokédex Area map."""

import argparse
import hashlib
import struct

from make_hgss_pokedex_stats import PALETTE, crop, decode_member

OUT_WIDTH = 240
OUT_HEIGHT = 160
MAP_WIDTH = 32
MAP_HEIGHT = 32
BASE_TILE = 640
PALETTE_BANK = 12
EXPECTED_LAYOUT_SHA256 = "60e3f61deda31116fb2d27e3855f5a82ea0735b37ba7209713e9b929a0c5a548"
EXPECTED_TILE_COUNT = 27


def remap(src):
    # Palette index 0 is transparent on the GBA overlay, so authentic source
    # colors move to indices 1..15 without changing their RGB values.
    return [bytearray(pixel + 1 for pixel in row) for row in src]


def paste(dst, src, x, y):
    for row_index, row in enumerate(src):
        dst[y + row_index][x:x + len(row)] = row


def build_layout():
    member = decode_member(65)
    out = [bytearray(OUT_WIDTH) for _ in range(OUT_HEIGHT)]

    # Authentic rounded HGSS header plus two genuine rule bands. The map area
    # between them remains transparent so BG2/BG3 keep rendering live.
    paste(out, remap(crop(member, 8, 4, 229, 30)), 8, 0)
    paste(out, remap(crop(member, 16, 57, 240, 63)), 16, 32)
    paste(out, remap(crop(member, 16, 128, 240, 134)), 16, 136)

    pixels = b"".join(bytes(row) for row in out)
    if hashlib.sha256(pixels).hexdigest() != EXPECTED_LAYOUT_SHA256:
        raise SystemExit("HGSS Area chrome layout checksum mismatch")
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
            tile.append(rows[py][px] if py < OUT_HEIGHT and px < OUT_WIDTH else 0)
    return tuple(tile)


def pack(rows):
    unique = []
    lookup = {}
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
                if candidate in lookup:
                    entry = (BASE_TILE + lookup[candidate]) | flags | (PALETTE_BANK << 12)
                    break
            if entry is None:
                index = len(unique)
                unique.append(tile)
                lookup[tile] = index
                entry = (BASE_TILE + index) | (PALETTE_BANK << 12)
            tilemap.append(entry)

    if len(unique) != EXPECTED_TILE_COUNT:
        raise SystemExit(
            f"unexpected Area chrome tile count: {len(unique)} "
            f"(expected {EXPECTED_TILE_COUNT})"
        )
    return unique, tilemap


def encode_4bpp(tile):
    out = bytearray()
    for i in range(0, 64, 2):
        out.append((tile[i] & 0xF) | ((tile[i + 1] & 0xF) << 4))
    return bytes(out)


def rgb555(rgb):
    r, g, b = rgb
    return min(31, (r + 4) // 8) | (min(31, (g + 4) // 8) << 5) | (min(31, (b + 4) // 8) << 10)


def build_palette():
    # Index 0 is transparent; authentic member-065 colors occupy 1..15.
    colors = [(0, 0, 0)] + list(PALETTE)
    return b"".join(struct.pack("<H", rgb555(color)) for color in colors)


def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--tiles", action="store_true")
    group.add_argument("--tilemap", action="store_true")
    group.add_argument("--palette", action="store_true")
    parser.add_argument("output")
    args = parser.parse_args()

    tiles, tilemap = pack(build_layout())
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
        f"{args.output}: authentic HGSS Area chrome {label}, "
        f"{len(tiles)} unique tiles, base tile {BASE_TILE}, palette bank {PALETTE_BANK}"
    )


if __name__ == "__main__":
    main()
