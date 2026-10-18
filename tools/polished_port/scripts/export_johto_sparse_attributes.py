#!/usr/bin/env python3
"""Export sparse Emerald Johto metatile attributes for offline integration.

0xFFFF is an INVALID placeholder, not a safe Emerald attribute. This file
must never be linked as a tileset: a separate validity bitmap identifies the
few candidate words eligible for subsequent field-engine review.
"""
import argparse
import json
import struct
from pathlib import Path
from merge_johto_behavior_candidates import merge

UNRESOLVED = 0xFFFF

def build():
    review = merge()
    records = review["primary"] + review["secondary"]
    assert len(records) == 1012
    attributes = []
    mask = bytearray((len(records) + 7) // 8)
    for i, row in enumerate(records):
        word = row["candidate_attribute_word"]
        if word is None:
            attributes.append(UNRESOLVED)
        else:
            assert 0 <= word <= 255
            attributes.append(word)
            mask[i // 8] |= 1 << (i % 8)
    known = sum(b.bit_count() for b in mask)
    assert 0 < known < 1012
    assert all((attributes[i] != UNRESOLVED) == bool(mask[i // 8] & (1 << (i % 8))) for i in range(1012))
    return attributes, bytes(mask), known

def export(output):
    output.mkdir(parents=True, exist_ok=True)
    values, mask, known = build()
    for name, start, end in (("primary", 0, 512), ("secondary", 512, 1012)):
        data = struct.pack("<" + "H" * (end - start), *values[start:end])
        (output / ("johto_" + name + "_attribute_candidates.bin")).write_bytes(data)
    (output / "johto_attribute_candidate_validity.bin").write_bytes(mask)
    result = {
        "status": "sparse_attribute_candidates_NOT_FOR_ROM",
        "primary_entries": 512, "secondary_entries": 500,
        "candidate_entries": known, "unresolved_entries": 1012 - known,
        "invalid_unresolved_sentinel": UNRESOLVED,
        "validity_bitmap_bytes": len(mask),
        "candidate_behavior_ids_only": ["MB_NORMAL", "MB_TALL_GRASS"],
        "water_and_ledge_candidates_not_encoded": True,
        "live_binary_suitable": False, "tileset_registered": False,
    }
    (output / "johto_attribute_candidates.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--output", required=True, type=Path)
    doc = export(p.parse_args().output)
    print(f"PASS: {doc['candidate_entries']} sparse attribute candidates, {doc['unresolved_entries']} unresolved")
    print("OFFLINE ONLY: 0xFFFF sentinels prohibit use as real tileset attributes")
if __name__ == "__main__":
    main()
