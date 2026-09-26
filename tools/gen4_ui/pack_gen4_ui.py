#!/usr/bin/env python3
"""Pack a gbagfx-produced 8bpp image into deduplicated GBA BG tiles + tilemap."""

import argparse
import struct

PNG_SIG = b"\x89PNG\r\n\x1a\n"
TILE_BYTES = 64
MAP_W = 32
MAP_H = 32


def png_size(path):
    with open(path, "rb") as f:
        header = f.read(24)
    if len(header) < 24 or header[:8] != PNG_SIG or header[12:16] != b"IHDR":
        raise ValueError(f"{path}: invalid PNG")
    return struct.unpack(">II", header[16:24])


def flip_h(tile):
    return bytes(tile[y * 8 + x] for y in range(8) for x in range(7, -1, -1))


def flip_v(tile):
    return bytes(tile[y * 8 + x] for y in range(7, -1, -1) for x in range(8))


def load_raw_tiles(path, width, height):
    expected = width * height
    with open(path, "rb") as f:
        raw = f.read()
    if len(raw) != expected:
        raise ValueError(
            f"{path}: expected {expected} bytes for {width}x{height} 8bpp image, got {len(raw)}"
        )
    return [raw[i:i + TILE_BYTES] for i in range(0, len(raw), TILE_BYTES)]


def pack(source_tiles, tiles_wide, tiles_high, max_tiles):
    blank = bytes(TILE_BYTES)
    unique = [blank]
    lookup = {blank: 0}
    tilemap = [0] * (MAP_W * MAP_H)

    for i, tile in enumerate(source_tiles):
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
                entry = index | flags
                break

        if entry is None:
            if len(unique) >= max_tiles:
                raise ValueError(
                    f"unique tile count exceeds {max_tiles}; crop/simplify the UI or raise --max-tiles"
                )
            entry = len(unique)
            unique.append(tile)
            lookup[tile] = entry

        y, x = divmod(i, tiles_wide)
        tilemap[y * MAP_W + x] = entry

    return unique, tilemap


def write_tiles(path, tiles):
    with open(path, "wb") as f:
        for tile in tiles:
            f.write(tile)


def write_tilemap(path, tilemap):
    with open(path, "wb") as f:
        for entry in tilemap:
            f.write(struct.pack("<H", entry))


def main():
    p = argparse.ArgumentParser()
    p.add_argument("png", help="source PNG used by gbagfx")
    p.add_argument("raw8bpp", help="non-deduplicated 8bpp output produced by gbagfx")
    p.add_argument("--tiles")
    p.add_argument("--tilemap")
    p.add_argument("--max-tiles", type=int, default=769)
    args = p.parse_args()

    if not args.tiles and not args.tilemap:
        p.error("request --tiles and/or --tilemap")
    if not 1 <= args.max_tiles <= 1023:
        p.error("--max-tiles must be in 1..1023")

    width, height = png_size(args.png)
    if width % 8 or height % 8:
        raise ValueError(f"{args.png}: dimensions must be multiples of 8, got {width}x{height}")
    tiles_wide, tiles_high = width // 8, height // 8
    if tiles_wide > MAP_W or tiles_high > MAP_H:
        raise ValueError(
            f"{args.png}: {tiles_wide}x{tiles_high} tiles exceeds the first 32x32 Gen 4 UI BG path"
        )

    source_tiles = load_raw_tiles(args.raw8bpp, width, height)
    tiles, tilemap = pack(source_tiles, tiles_wide, tiles_high, args.max_tiles)

    if args.tiles:
        write_tiles(args.tiles, tiles)
    if args.tilemap:
        write_tilemap(args.tilemap, tilemap)

    print(
        f"{args.png}: {width}x{height}, {len(source_tiles)} source tiles -> "
        f"{len(tiles)} unique tiles ({len(tiles) * TILE_BYTES} bytes)"
    )


if __name__ == "__main__":
    main()
