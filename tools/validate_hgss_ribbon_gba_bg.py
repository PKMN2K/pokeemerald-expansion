#!/usr/bin/env python3
"""Validate exported GBA HGSS ribbon 4bpp background assets (read-only).

Checks binary structure, referenced tile/palette bounds, and 240x160 viewport
padding. This does not certify live BG bank availability or visual fidelity.
"""
import argparse
from pathlib import Path

MAP_WIDTH = 32
VISIBLE_WIDTH = 30
VISIBLE_HEIGHT = 20
TILE_BYTES = 32
PALETTE_BYTES = 512


def validate(tiles, palette, tilemap):
    if len(tiles) < TILE_BYTES or len(tiles) % TILE_BYTES:
        raise ValueError("4bpp tile graphics must contain whole 32-byte tiles")
    if len(tiles) // TILE_BYTES > 1024:
        raise ValueError("4bpp tile count exceeds 10-bit BG tile index")
    if len(palette) != PALETTE_BYTES:
        raise ValueError("Expected exactly 16 BGR555 palette banks (512 bytes)")
    if len(tilemap) != MAP_WIDTH * MAP_WIDTH * 2:
        raise ValueError("Expected a 32x32 GBA screen map (2048 bytes)")
    if tiles[:TILE_BYTES] != bytes(TILE_BYTES):
        raise ValueError("Tile 0 must be transparent/blank")
    count = len(tiles) // TILE_BYTES
    used = set()
    for position in range(MAP_WIDTH * MAP_WIDTH):
        attr = int.from_bytes(tilemap[2 * position:2 * position + 2], "little")
        x, y = position % MAP_WIDTH, position // MAP_WIDTH
        if x >= VISIBLE_WIDTH or y >= VISIBLE_HEIGHT:
            if attr != 0:
                raise ValueError(f"Nonblank offscreen map cell ({x}, {y})")
            continue
        index = attr & 1023
        if index >= count:
            raise ValueError(f"Tile index {index} out of range at ({x}, {y})")
        used.add(attr >> 12)
    return {"tile_count": count, "palette_banks": sorted(used)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("prefix", type=Path, help="Export basename without extension")
    args = parser.parse_args()
    prefix = args.prefix
    result = validate(
        prefix.with_suffix(".4bpp").read_bytes(),
        prefix.with_suffix(".gbapal").read_bytes(),
        prefix.with_suffix(".bin").read_bytes(),
    )
    print(f"Validated {result['tile_count']} original 4bpp tiles; "
          f"referenced palette banks: {result['palette_banks']}")
    print("Structure only; live palette allocation and emulator screenshots remain gated.")


if __name__ == "__main__":
    main()
