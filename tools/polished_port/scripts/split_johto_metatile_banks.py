#!/usr/bin/env python3
"""Split the authentic 253-block Johto geometry into Emerald's two metatile banks.

The original tables contain 253 blocks x 16 8x8 entries = 4048 bytes each.
Each block yields four 16x16 Emerald metatiles (1012 total). Emerald's
normal primary bank holds 512 metatiles and the secondary has capacity 512.
Therefore the *source* contributes 512 primary and 500 secondary metatiles.
The last 12 secondary slots have NO authentic source data; this staging
step never invents artwork or live metatile entries.
"""
import argparse
import json
from pathlib import Path

METATILES_PER_BLOCK = 4
SOURCE_BLOCKS = 253
PRIMARY_METATILES = 512
SECONDARY_METATILES = SOURCE_BLOCKS * METATILES_PER_BLOCK - PRIMARY_METATILES
METATILE_CAPACITY = 1024
BYTES_PER_METATILE = 8 * 2
PRIMARY_BYTES = PRIMARY_METATILES * BYTES_PER_METATILE
SECONDARY_BYTES = SECONDARY_METATILES * BYTES_PER_METATILE
ORDER = ("top_left", "top_right", "bottom_left", "bottom_right")


def source_block_address(block, quadrant):
    """Return the standard Emerald map metatile ID and bank-local index."""
    if not 0 <= block < SOURCE_BLOCKS:
        raise ValueError(f"invalid source block: {block}")
    if quadrant not in ORDER:
        raise ValueError(f"invalid quadrant: {quadrant}")
    full_id = block * METATILES_PER_BLOCK + ORDER.index(quadrant)
    bank = "primary" if full_id < PRIMARY_METATILES else "secondary"
    return {
        "bank": bank,
        "map_metatile_id": full_id,
        "bank_local_index": full_id if bank == "primary" else full_id - PRIMARY_METATILES,
    }


def split_geometry(raw):
    if len(raw) != PRIMARY_BYTES + SECONDARY_BYTES:
        raise ValueError(f"expected {PRIMARY_BYTES + SECONDARY_BYTES} bytes for 1012 source metatiles; got {len(raw)}")
    return raw[:PRIMARY_BYTES], raw[PRIMARY_BYTES:]


def manifest():
    return {
        "status": "staging_only_not_registered",
        "game": "Emerald (512 primary + 512 secondary capacity)",
        "metatile_bytes": BYTES_PER_METATILE,
        "primary_metatiles": PRIMARY_METATILES,
        "secondary_source_metatiles": SECONDARY_METATILES,
        "source_blocks": SOURCE_BLOCKS,
        "primary_source_blocks": [0, 127],
        "secondary_source_blocks": [128, 252],
        "unused_secondary_map_metatile_ids": list(range(SOURCE_BLOCKS * 4, METATILE_CAPACITY)),
        "source_quadrant_order": list(ORDER),
        "block_address": [
            {"source_block": block,
             "quadrants": {q: source_block_address(block, q) for q in ORDER}}
            for block in range(SOURCE_BLOCKS)
        ],
        "unresolved_before_live_use": [
            "Source VRAM tile banks and palette indices must be reconciled with GBA tileset graphics",
            "GBC palette 0..6 references need GBA primary-six / secondary-seven allocation",
            "Source priority bit 7 and provisional upper-layer transparency must be resolved",
            "Collision, ledge, warp and sign behaviors are not registered",
            "Missing 12 secondary slots cannot be treated as authentic source tiles",
            "Both bank halves must be registered together as one map pair",
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True,
                        help="metatiles.staging.bin from the 253-block geometry converter")
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
    print("Remaining 12 secondary IDs absent in source; no invented entries.")
    print("Not registered: graphics/palette and collision work remains.")


if __name__ == "__main__":
    main()
