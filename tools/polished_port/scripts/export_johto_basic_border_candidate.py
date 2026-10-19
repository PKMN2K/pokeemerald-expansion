#!/usr/bin/env python3
"""Export an offline 2x2 border candidate from the source-backed Johto floor.

The candidate is deliberately not copied into data/layouts or registered.
Visual border art and runtime field behavior still require verification.
"""
import argparse
import json
import struct
from pathlib import Path
from export_johto_basic_walkability_layout import build, LAND_ELEVATION

def export(output):
    output.mkdir(parents=True, exist_ok=True)
    cells = build()
    floors = [cell for cell in cells if cell["source_symbol"] == "FLOOR"]
    assert floors, "Source-backed floor cell required"
    source = floors[0]
    metatile = source["metatile_id"]
    assert 0 <= metatile < 1024
    assert source["candidate_attribute_word"] == 0
    packed = metatile | (LAND_ELEVATION << 12)
    assert source["packed_cell"] == packed
    border = [packed] * 4
    raw = struct.pack("<4H", *border)
    assert len(raw) == 8
    (output / "johto_basic_border_candidate.bin").write_bytes(raw)
    report = {
        "status": "OFFLINE_BORDER_CANDIDATE_NOT_FOR_ROM",
        "width": 2, "height": 2, "bytes": len(raw),
        "metatile_id": metatile, "source_symbol": source["source_symbol"],
        "candidate_attribute_word": source["candidate_attribute_word"],
        "packed_cells": border,
        "source_backed": True,
        "visually_verified": False,
        "registered": False,
        "movement_verified_in_emulator": False,
    }
    (output / "johto_basic_border_candidate.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    return report

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    export(parser.parse_args().output)
    print("PASS: isolated source-backed Johto 2x2 border candidate (8 bytes)")
    print("OFFLINE ONLY: no live layout registration or visual verification")
