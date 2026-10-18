#!/usr/bin/env python3
"""Package a reproducible Johto outdoor integration fixture for emulator work.

Pairs authentic graphics/metatiles/palettes with the 8x8 mapgrid candidate.
This is an offline package only, not a live map registration.
"""
import argparse
import hashlib
import json
from pathlib import Path
from export_johto_tileset_bundle import export
from export_johto_integrated_mapgrid import build
import struct

def package(output):
    export(output)
    cells = build()
    grid = struct.pack("<64H", *(cell["packed_cell"] for cell in cells))
    (output / "johto_mapgrid_candidate.bin").write_bytes(grid)
    spec = json.loads((output / "tileset_bundle.json").read_text())
    assert len(grid) == 128
    assert all(0 <= cell["metatile_id"] < 1012 for cell in cells)
    spec["files"]["johto_mapgrid_candidate.bin"] = len(grid)
    spec["sha256"] = {
        name: hashlib.sha256((output / name).read_bytes()).hexdigest()
        for name in sorted(spec["files"])
    }
    spec["layout"] = {
        "width": 8, "height": 8,
        "metatile_id_range": [0, 1011],
        "cell_encoding": "little_endian_u16_id_collision_elevation",
        "collision_and_elevation_provisional": True,
    }
    spec["status"] = "offline_johto_integrated_fixture_unregistered"
    spec["attribute_tables_included"] = False
    spec["map_registered"] = False
    spec["tilesets_registered"] = False
    (output / "johto_integration_manifest.json").write_text(
        json.dumps(spec, indent=2) + "\n", encoding="utf-8")
    return spec

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--output", required=True, type=Path)
    data = package(p.parse_args().output)
    assert len(data["files"]) == 6
    print("PASS: Johto graphics/metatiles/palettes/mapgrid bundled with SHA-256 verification")
    print("OFFLINE ONLY: incomplete attributes and field semantics prevent map registration")
if __name__ == "__main__":
    main()
