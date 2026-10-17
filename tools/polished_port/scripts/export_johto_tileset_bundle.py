#!/usr/bin/env python3
"""Prepare authentic Johto tileset binaries for an isolated future map.

Only exports offline 4bpp graphics, split metatiles and source day palettes.
Never writes registered tilesets, map headers or existing Hoenn/Prism assets.
"""
import argparse
import json
from pathlib import Path
from stage_johto_tile_graphics import build_graphics
from test_johto_metatile_banks import load_binary_fixture
from convert_johto_traditional_blocks import convert
from split_johto_metatile_banks import split_geometry, PRIMARY_BYTES, SECONDARY_BYTES
from stage_johto_time_palettes import stage, load_source_palettes

def export(out):
    primary, secondary, gfx_report = build_graphics()
    tiles = load_binary_fixture("metatiles")
    attrs = load_binary_fixture("attributes")
    all_meta = convert(tiles, attrs)
    pri_meta, sec_meta = split_geometry(all_meta)
    palettes, palette_report = stage(load_source_palettes())
    day = palettes["standard.day"]
    assert (len(primary), len(secondary)) == (16384, 16384)
    assert (len(pri_meta), len(sec_meta)) == (PRIMARY_BYTES, SECONDARY_BYTES)
    assert len(day) == 224
    out.mkdir(parents=True, exist_ok=True)
    binaries = {
        "johto_primary.4bpp": primary,
        "johto_secondary.4bpp": secondary,
        "johto_primary_metatiles.bin": pri_meta,
        "johto_secondary_metatiles.bin": sec_meta,
        "johto_standard_day_palette_0_to_6.gbapal": day,
    }
    for name, raw in binaries.items():
        (out / name).write_bytes(raw)
    spec = {
        "status": "offline_tileset_bundle_unregistered",
        "source": "Rangi42/polishedcrystal@3fa43192379df5c3e7b09a08e4d5d79af4f02f42",
        "files": {name: len(raw) for name, raw in binaries.items()},
        "primary_metatiles": 512,
        "secondary_metatiles": 500,
        "missing_source_secondary_slots": list(range(1012, 1024)),
        "source_palette_family": "standard",
        "source_palette_time": "day",
        "required_palette_slots_primary": list(range(6)),
        "required_palette_slots_secondary": [6],
        "opaque_source_color_zero": gfx_report["palette_gba_index_4_must_duplicate_source_color_0"],
        "source_palette_bank_count": palette_report["source_banks_per_time"],
        "requires_map_group_roof_override": True,
        "requires_collision_attributes": True,
        "requires_object_elevation_review": True,
        "map_registered": False,
        "tilesets_registered": False,
    }
    assert spec["opaque_source_color_zero"] and spec["source_palette_bank_count"] == 7
    (out / "tileset_bundle.json").write_text(json.dumps(spec, indent=2) + "\n")
    print("PASS: authentic Johto graphics, metatiles and seven RGB555 palette banks exported")
    print("STAGING ONLY: no registered tileset, map, collision or OBJ priority claims")
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    export(parser.parse_args().output)
if __name__ == "__main__":
    main()
