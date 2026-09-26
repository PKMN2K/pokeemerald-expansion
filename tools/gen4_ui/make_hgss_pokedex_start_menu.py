#!/usr/bin/env python3
"""Build authentic HGSS-derived sliding Pokédex start-menu assets.

The live menu geometry is preserved exactly:
- 14 tiles wide at BG0 x=16
- main list: 10 tile rows, slides by 80 px
- search results: 12 tile rows, slides by 96 px

Panel pixels come from authentic HGSS Pokédex member 057. The existing English
menu labels are stored here as compact 1-bit masks so the legacy adapted
tilemap_start_menu*.bin files are not runtime or build dependencies.
"""

import argparse
import base64
import hashlib
import struct
import zlib

from make_hgss_pokedex_stats import PALETTE, decode_member

PANEL_WIDTH = 112
MAP_WIDTH = 32
PANEL_TILE_X = 16
BASE_TILE = 768
PALETTE_BANK = 12
TEXT_INDEX = 2  # source member 057 dark purple index 1, shifted by +1

MAIN_MASKS = (
    "eNpjYCAAPh5XkrdQ+Hm+gYGhS2MJi4uCkhIDiKkBYjKBmQYw5icwU/EQkNn9AsRUQKiFMuEmfFJXYrGw+3mIgTQAACGEFUg=",
    "eNpjYCAAmp/byVs+ADMbOjpYXAWgTA5szHYEswGo9gGCyYAwAcL8/ZyDxZKBZAAAs8sRhg==",
    "eNpjYCAAmp/byVl+P+cCZDZ0dAi6OArmgJkcaMwQELOdQw7GbACrdUFjdsCZv59zyFk48rkwkAYA4mIXJQ==",
    "eNpjYCAAithlflQ8cav84sDQxbGowVWkw5XFgaGJY0GDqyiDK1MDhCnGB2HO+eAqJuj6kAHIbGqoFJOHiDY1OIoKgJggExxFJoJMKH4u88PhiR/IXFIAAJ+zH7I=",
)

SEARCH_RESULTS_MASKS = (
    MAIN_MASKS[0],
    MAIN_MASKS[1],
    MAIN_MASKS[2],
    "eNpjYCAAPh5XkrewKVK2+aXQpbGExUWpy0WpCcjUADEblBodujQMgMxudiDzE4TJsehDQ/cLINOmm38RSAFQrUJXB1jtEhDTY1GTwid1JRYLhSJ1oLkMJAAAA9ogqw==",
    MAIN_MASKS[3],
)

EXPECTED = {
    "main": {
        "pixel_sha256": "822809d719c6a259377b7bc7a793d66d5489bae6f2a52445da68293a961f2fd6",
        "tiles": 60,
    },
    "search-results": {
        "pixel_sha256": "73f4515ccefc102f0be0ea9cf366c9a5c2235d7a418423b536e2a56a7e1da5dc",
        "tiles": 76,
    },
}


def decode_mask(payload):
    raw = zlib.decompress(base64.b64decode(payload))
    if len(raw) != 176:
        raise SystemExit(f"bad start-menu label mask size: {len(raw)}")

    mask = [[False] * 88 for _ in range(16)]
    for bit_index in range(16 * 88):
        byte = raw[bit_index // 8]
        bit = 7 - (bit_index % 8)
        mask[bit_index // 88][bit_index % 88] = bool((byte >> bit) & 1)
    return mask


def build_panel(variant):
    member = decode_member(57)
    masks = MAIN_MASKS if variant == "main" else SEARCH_RESULTS_MASKS
    height = 8 + len(masks) * 16 + 8

    # Authentic HGSS member-057 source tiles:
    # - (32, 8): flat pale-blue list interior
    # - (32,24): orange/purple list rule
    fill = [bytearray(row[32:40]) for row in member[8:16]]
    rule = [bytearray(row[32:40]) for row in member[24:32]]
    rule_top = list(reversed([bytearray(row) for row in rule]))

    out = [bytearray(PANEL_WIDTH) for _ in range(height)]

    # Fill the whole panel with the authentic pale-blue tile, shifting source
    # indices by +1 so palette index 0 remains transparent outside the panel.
    for y in range(0, height, 8):
        for x in range(0, PANEL_WIDTH, 8):
            for py in range(8):
                for px in range(8):
                    out[y + py][x + px] = fill[py][px] + 1

    # Authentic HGSS rule pixels make the top/bottom borders and item dividers.
    for x in range(0, PANEL_WIDTH, 8):
        for py in range(8):
            for px in range(8):
                out[py][x + px] = rule_top[py][px] + 1
                out[height - 8 + py][x + px] = rule[py][px] + 1

    for item in range(1, len(masks)):
        y0 = 8 + item * 16 - 4
        for x in range(0, PANEL_WIDTH, 8):
            for py in range(8):
                for px in range(8):
                    out[y0 + py][x + px] = rule[py][px] + 1

    # Preserve the established labels and their exact 16px menu-row spacing.
    for item, payload in enumerate(masks):
        mask = decode_mask(payload)
        y0 = 8 + item * 16
        for y in range(16):
            for x in range(88):
                if mask[y][x]:
                    out[y0 + y][16 + x] = TEXT_INDEX

    flat = b"".join(bytes(row) for row in out)
    expected = EXPECTED[variant]["pixel_sha256"]
    if hashlib.sha256(flat).hexdigest() != expected:
        raise SystemExit(f"{variant} start-menu pixel checksum mismatch")
    return out


def flip_h(tile):
    return tuple(tile[y * 8 + x] for y in range(8) for x in range(7, -1, -1))


def flip_v(tile):
    return tuple(tile[y * 8 + x] for y in range(7, -1, -1) for x in range(8))


def pack(panel, variant):
    tiles_high = len(panel) // 8
    blank = (0,) * 64
    unique = [blank]
    lookup = {blank: 0}
    tilemap = []

    for ty in range(tiles_high):
        for tx in range(MAP_WIDTH):
            if tx < PANEL_TILE_X or tx >= PANEL_TILE_X + PANEL_WIDTH // 8:
                tile = blank
            else:
                sx = (tx - PANEL_TILE_X) * 8
                tile = tuple(
                    panel[ty * 8 + y][sx + x]
                    for y in range(8)
                    for x in range(8)
                )

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

    expected_tiles = EXPECTED[variant]["tiles"]
    if len(unique) != expected_tiles:
        raise SystemExit(
            f"{variant} start-menu tile count {len(unique)} != {expected_tiles}"
        )
    if BASE_TILE + len(unique) > 1024:
        raise SystemExit("start-menu tiles exceed charblock 0")

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
    # Index 0 remains transparent; authentic member-057 colors become 1..15.
    colors = [(0, 0, 0)] + list(PALETTE)
    return b"".join(struct.pack("<H", rgb555(color)) for color in colors)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--variant",
        choices=("main", "search-results"),
        default="main",
    )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--tiles", action="store_true")
    group.add_argument("--tilemap", action="store_true")
    group.add_argument("--palette", action="store_true")
    parser.add_argument("output")
    args = parser.parse_args()

    if args.palette:
        data = build_palette()
        label = "palette"
        tile_count = 0
    else:
        panel = build_panel(args.variant)
        tiles, tilemap = pack(panel, args.variant)
        tile_count = len(tiles)
        if args.tiles:
            data = b"".join(encode_4bpp(tile) for tile in tiles)
            label = "4bpp tiles"
        else:
            data = b"".join(struct.pack("<H", entry) for entry in tilemap)
            label = "tilemap"

    with open(args.output, "wb") as f:
        f.write(data)

    print(
        f"{args.output}: HGSS start menu {args.variant} {label}, "
        f"base tile {BASE_TILE}, palette bank {PALETTE_BANK}, "
        f"{tile_count} unique tiles"
    )


if __name__ == "__main__":
    main()
