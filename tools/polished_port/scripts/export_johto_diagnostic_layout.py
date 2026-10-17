#!/usr/bin/env python3
"""Export an isolated Johto diagnostic layout binary, without map registration.

This is a render fixture, not a playable map: collision and elevation are
intentionally unverified. No production data/layouts or data/maps is edited.
"""
import argparse
import json
from pathlib import Path
import struct
from audit_johto_outdoor_test_grid import audit

ROOT = Path(__file__).resolve().parents[3]

def export(output):
    report = audit()
    rows = report["converted_metatile_ids_8x8"]
    assert len(rows) == 8 and all(len(row) == 8 for row in rows)
    # Emerald layout cells: low ten bits are the metatile ID. Do not
    # invent collision/elevation flags before source-parity review.
    ids = [v for row in rows for v in row]
    assert len(ids) == 64 and all(0 <= v < 1024 for v in ids)
    layout = struct.pack("<64H", *ids)
    assert len(layout) == 128
    output.mkdir(parents=True, exist_ok=True)
    (output / "map.bin").write_bytes(layout)
    (output / "layout.json").write_text(json.dumps({
        "status": "offline_unregistered_render_fixture",
        "width": 8,
        "height": 8,
        "blockdata_filepath": "map.bin",
        "source_block_ids": report["source_blocks_4x4"],
        "metatile_ids": rows,
        "collision_elevation_status": "not_verified_no_assertion_of_walkability",
        "animations_status": "callback_compiled_not_live",
        "map_header": None,
        "tileset_registration": None,
    }, indent=2) + "\n", encoding="utf-8")
    # Safety gate: never allow a staged fixture to shadow a real layout.
    assert not (ROOT / "data/layouts/PolishedJohtoDiagnostic").exists()
    print("PASS: standalone 8x8 Johto layout binary generated (128 bytes)")
    print("NOT PLAYABLE: no map header, tileset registration, collision or elevation validation")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    export(args.output)

if __name__ == "__main__":
    main()
