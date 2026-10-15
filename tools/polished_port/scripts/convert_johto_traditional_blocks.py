#!/usr/bin/env python3
"""Stage authentic Polished Crystal Johto Traditional block geometry for Emerald.

Usage:
  python3 tools/polished_port/scripts/convert_johto_traditional_blocks.py \
      --source /path/to/polishedcrystal --output /tmp/polished-johto

This creates a *staging* 16x16 GBA metatile table, not a registered
game tileset. GBC VRAM bank-0/1 graphics are mapped to real imported 4bpp
tile IDs 1..288, with tile 0 an all-transparent upper-layer tile.
38 original per-8x8 priority flags use 12 new source-derived overlay
masks at tile IDs 289..300, mapping only CGB color0 to transparency.
This requires Emerald METATILE_LAYER_TYPE_NORMAL (BG2 ground, BG1
foreground) when these blocks are registered. Sprite elevation parity,
source animations and field collision are not yet validated live.
"""
import argparse
from pathlib import Path
import struct

from stage_johto_tile_graphics import gba_tile_id, priority_overlay_tile_ids

SOURCE_PREFIX = Path("data/tilesets/johto_traditional")
BLOCK_COUNT = 253
SOURCE_SIDE = 4
GBA_TILES_PER_METATILE = 8
GBC_TILE_BANK_BIT = 0x08
GBC_PRIORITY_BIT = 0x80
GBC_HFLIP_BIT = 0x20
GBC_VFLIP_BIT = 0x40


def convert(metatiles: bytes, attrs: bytes) -> bytes:
    expected = BLOCK_COUNT * SOURCE_SIDE * SOURCE_SIDE
    if len(metatiles) != expected or len(attrs) != expected:
        raise ValueError(f"expected {expected} bytes per source table, got "
                         f"{len(metatiles)} metatile and {len(attrs)} attribute bytes")

    overlay_ids = priority_overlay_tile_ids(metatiles, attrs)
    output = bytearray()
    # The pinned source has 253 blocks, each 32x32 with 4x4 8x8 tiles.
    # Each becomes FOUR 16x16 Emerald metatiles, in TL/TR/BL/BR order.
    for block in range(BLOCK_COUNT):
        start = block * 16
        for by in (0, 2):
            for bx in (0, 2):
                bottom = []
                top = []
                for y in (0, 1):
                    for x in (0, 1):
                        i = start + (by + y) * 4 + bx + x
                        attr = attrs[i]
                        tile_id = gba_tile_id(metatiles[i], int(bool(attr & GBC_TILE_BANK_BIT)))
                        entry = tile_id | ((attr & 7) << 12)
                        if attr & GBC_HFLIP_BIT:
                            entry |= 1 << 10
                        if attr & GBC_VFLIP_BIT:
                            entry |= 1 << 11
                        bottom.append(entry)
                        if attr & GBC_PRIORITY_BIT:
                            # Preserve flip/palette flags; replace ONLY the
                            # graphic ID in BG1 with its color0-masked clone.
                            # BG2 keeps the original pixel0 color opaque.
                            top.append((entry & ~0x03FF) | overlay_ids[tile_id])
                        else:
                            top.append(0)  # synthetic transparent tile 0
                # NORMAL draws bottom on BG2 and masked priority on BG1.
                # Do not use COVERED/SPLIT without a separate layer audit.
                for entry in bottom + top:
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
    print("Staged 38 source priority entries across 12 masked foreground tiles.")
    print("NOT registered: sprite elevation parity, animations and collision need review.")


if __name__ == "__main__":
    main()
