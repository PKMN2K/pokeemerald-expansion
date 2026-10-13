#!/usr/bin/env python3
"""Stage authentic Polished Crystal Johto Traditional block geometry for Emerald.

Usage:
  python3 tools/polished_port/scripts/convert_johto_traditional_blocks.py \
      --source /path/to/polishedcrystal --output /tmp/polished-johto

This creates a *staging* 16x16 GBA metatile table, not a registered
game tileset. Palette, VRAM bank layout and collision must be validated
before it can be wired into live map headers.
"""
import argparse
from pathlib import Path
import struct

SOURCE_PREFIX = Path("data/tilesets/johto_traditional")
BLOCK_COUNT = 253
SOURCE_SIDE = 4
GBA_TILES_PER_METATILE = 8
GBC_TILE_BANK_BIT = 0x08
GBC_HFLIP_BIT = 0x20
GBC_VFLIP_BIT = 0x40


def convert(metatiles: bytes, attrs: bytes) -> bytes:
    expected = BLOCK_COUNT * SOURCE_SIDE * SOURCE_SIDE
    if len(metatiles) != expected or len(attrs) != expected:
        raise ValueError(f"expected {expected} bytes per source table, got "
                         f"{len(metatiles)} metatile and {len(attrs)} attribute bytes")

    output = bytearray()
    # The pinned source has 253 blocks, each 32x32 with 4x4 8x8 tiles.
    # Each becomes FOUR 16x16 Emerald metatiles, in TL/TR/BL/BR order.
    for block in range(BLOCK_COUNT):
        start = block * 16
        for by in (0, 2):
            for bx in (0, 2):
                bottom = []
                for y in (0, 1):
                    for x in (0, 1):
                        i = start + (by + y) * 4 + bx + x
                        attr = attrs[i]
                        tile_id = metatiles[i] | (0x100 if attr & GBC_TILE_BANK_BIT else 0)
                        entry = tile_id | ((attr & 7) << 12)
                        if attr & GBC_HFLIP_BIT:
                            entry |= 1 << 10
                        if attr & GBC_VFLIP_BIT:
                            entry |= 1 << 11
                        bottom.append(entry)
                # Top layer uses tile 0 as a provisional transparent tile.
                # Not safe to register until tile 0 transparency is verified.
                for entry in bottom + [0] * 4:
                    output.extend(struct.pack("<H", entry))
    assert len(output) == BLOCK_COUNT * 4 * GBA_TILES_PER_METATILE * 2
    return bytes(output)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    meta = (args.source / f"{SOURCE_PREFIX}_metatiles.bin").read_bytes()
    attrs = (args.source / f"{SOURCE_PREFIX}_attributes.bin").read_bytes()
    result = convert(meta, attrs)
    args.output.mkdir(parents=True, exist_ok=True)
    target = args.output / "metatiles.staging.bin"
    target.write_bytes(result)
    print(f"Staged {BLOCK_COUNT * 4} metatiles ({len(result)} bytes) at {target}")
    print("NOT registered: GBC priority flags, palette, transparency and collision need review.")


if __name__ == "__main__":
    main()
