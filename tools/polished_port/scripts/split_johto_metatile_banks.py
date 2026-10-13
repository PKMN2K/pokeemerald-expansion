#!/usr/bin/env python3
"""Split staged Johto Traditional geometry into Emerald primary/secondary banks.

Emerald maps address 1024 metatiles (512 primary + 512 secondary).
256 original Polished Crystal blocks each yield four 16x16 metatiles.
The resulting 1024-metatile geometry must therefore be split exactly
between the two halves; neither half can stand alone.

This tool reads convert_johto_traditional_blocks.py's unregistered staging
output. It does NOT wire tilesets into the ROM. Graphics tile indices,
palette slots, upper-layer transparency, and collision/event behaviors
must be verified separately before registration.
"""
import argparse
import json
from pathlib import Path

METATILES_PER_BLOCK = 4
BLOCKS = 256
METATILES_PER_BANK = 512
BYTES_PER_METATILE = 8 * 2
BANK_BYTES = METATILES_PER_BANK * BYTES_PER_METATILE
ORDER = ("top_left", "top_right", "bottom_left", "bottom_right")


def source_block_address(block, quadrant):
    """Return the standard Emerald map metatile ID and bank-local index."""
    if not 0 <= block < BLOCKS:
        raise ValueError(f"invalid source block: {block}")
    if quadrant not in ORDER:
        raise ValueError(f"invalid quadrant: {quadrant}")
    full_id = block * METATILES_PER_BLOCK + ORDER.index(quadrant)
    bank = "primary" if full_id < METATILES_PER_BANK else "secondary"
    return {
        "bank": bank,
        "map_metatile_id": full_id,
        "bank_local_index": full_id % METATILES_PER_BANK,
    }


def split_geometry(raw):
    if len(raw) != BANK_BYTES * 2:
        raise ValueError(f"expected {BANK_BYTES * 2} bytes for 1024 metatiles; "
                         f"got {len(raw)}")
    return raw[:BANK_BYTES], raw[BANK_BYTES:]


def manifest():
    return {
        "status": "staging_only_not_registered",
        "game": "Emerald (512 primary + 512 secondary metatiles)",
        "metatile_bytes": BYTES_PER_METATILE,
        "primary_metatiles": METATILES_PER_BANK,
        "secondary_metatiles": METATILES_PER_BANK,
        "primary_source_blocks": [0, 127],
        "secondary_source_blocks": [128, 255],
        "source_quadrant_order": list(ORDER),
        "block_address": [
            {"source_block": block,
             "quadrants": {q: source_block_address(block, q) for q in ORDER}}
            for block in range(BLOCKS)
        ],
        "unresolved_before_live_use": [
            "Source tile banks and palette indices must be reconciled with the two GBA graphics banks",
            "Provisional upper-layer tile 0 transparency must be validated",
            "Collision, ledge, warp and sign behaviors are not registered",
            "Both bank halves must be registered together as one map pair",
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True,
                        help="metatiles.staging.bin from the geometry converter")
    parser.add_argument("--output", type=Path, required=True,
                        help="directory for primary and secondary staging outputs")
    args = parser.parse_args()

    primary, secondary = split_geometry(args.input.read_bytes())
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "metatiles.primary.staging.bin").write_bytes(primary)
    (args.output / "metatiles.secondary.staging.bin").write_bytes(secondary)
    (args.output / "metatiles.bankmap.staging.json").write_text(
        json.dumps(manifest(), indent=2) + "\n", encoding="utf-8")
    print(f"Staged {len(primary)} primary and {len(secondary)} secondary bytes.")
    print("Not registered: palette, tile-index, top-layer and collision work remains.")


if __name__ == "__main__":
    main()
