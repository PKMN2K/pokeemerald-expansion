#!/usr/bin/env python3
"""Project pinned Johto 32x32 collision declarations onto 1012 GBA 16x16 IDs.

Preserves original symbolic behavior. Does NOT confuse Polished Crystal
collision constants with pokeemerald-expansion metatile behavior IDs.
"""
import argparse
import json
from collections import Counter
from pathlib import Path
from extract_johto_collision import extract, QUADRANTS
from split_johto_metatile_banks import source_block_address
from test_johto_metatile_banks import FIXTURES

SOURCE_COUNT = 253
def convert():
    original = extract((FIXTURES / "johto_traditional_collision.asm").read_text(encoding="utf-8"))
    assert len(original) == SOURCE_COUNT
    records = []
    for block in original:
        for quadrant in QUADRANTS:
            address = source_block_address(block["block"], quadrant)
            records.append({
                "metatile_id": address["map_metatile_id"],
                "source_block": block["block"],
                "quadrant": quadrant,
                "bank": address["bank"],
                "bank_local_index": address["bank_local_index"],
                "source_collision_symbol": block["quadrants"][quadrant],
                "emerald_behavior_id": None,
            })
    assert len(records) == SOURCE_COUNT * 4
    assert [r["metatile_id"] for r in records] == list(range(1012))
    primary = records[:512]
    secondary = records[512:]
    assert len(primary) == 512 and len(secondary) == 500
    frequencies = dict(sorted(Counter(r["source_collision_symbol"] for r in records).items()))
    return {
        "status": "source_collision_symbols_projected_not_registered",
        "source": "Rangi42/polishedcrystal@3fa43192379df5c3e7b09a08e4d5d79af4f02f42",
        "primary": primary,
        "secondary": secondary,
        "source_symbol_frequencies": frequencies,
        "unmapped_source_symbols": sorted(frequencies),
        "emerald_attribute_tables_emitted": False,
        "collision_semantics_verified": False,
        "unsupported_secondary_ids": list(range(1012, 1024)),
    }
def main():
    p = argparse.ArgumentParser()
    p.add_argument("--output", type=Path, required=True)
    args = p.parse_args()
    doc = convert()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
    print("PASS: 253 source blocks projected to 1012 source-faithful quadrant collision symbols")
    print(f"REVIEW: {len(doc['unmapped_source_symbols'])} symbols require Emerald semantic mapping")
    print("NOT LIVE: no unsafe metatile attribute IDs emitted")
if __name__ == "__main__":
    main()
