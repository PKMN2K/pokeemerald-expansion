#!/usr/bin/env python3
"""Export only source-backed basic Johto metatiles as a safe test subset.

Select FLOOR/TALL_GRASS metatiles with valid candidate attributes, preserving
their original 10-bit metatile IDs. Do not substitute unresolved terrain.
This is an offline selection fixture, not a tileset or map registration.
"""
import argparse
import json
import struct
from pathlib import Path
from export_johto_sparse_attributes import build, UNRESOLVED
from project_johto_collision_symbols import convert

def select():
    attributes, bitmap, _ = build()
    source = convert()["primary"] + convert()["secondary"]
    eligible = []
    for index, (word, row) in enumerate(zip(attributes, source)):
        if word == UNRESOLVED:
            continue
        assert row["metatile_id"] == index
        assert row["source_collision_symbol"] in ("FLOOR", "TALL_GRASS")
        assert bitmap[index // 8] & (1 << (index % 8))
        eligible.append({"metatile_id": index, "source_symbol": row["source_collision_symbol"],
                         "attribute_word": word, "bank": row["bank"]})
    assert eligible and len(eligible) < len(source)
    return eligible

def export(output):
    output.mkdir(parents=True, exist_ok=True)
    eligible = select()
    binary = struct.pack("<" + "H" * len(eligible), *(x["metatile_id"] for x in eligible))
    (output / "johto_basic_terrain_ids.bin").write_bytes(binary)
    report = {
        "status": "basic_terrain_only_offline_selection",
        "eligible_count": len(eligible),
        "eligible": eligible,
        "unresolved_terrain_excluded": True,
        "actual_test_map_registered": False,
        "field_walkability_verified": False,
    }
    (output / "johto_basic_terrain_selection.json").write_text(json.dumps(report, indent=2) + "\n")
    return report

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    report = export(parser.parse_args().output)
    print(f"PASS: {report['eligible_count']} source-backed basic metatiles selected")
    print("OFFLINE ONLY: no unsafe fallback attributes or live map registration")
if __name__ == "__main__":
    main()
