#!/usr/bin/env python3
"""Validate requirements for registering an isolated Johto outdoor tileset.

The package is deliberately NOT a live tileset. A source-derived 16x16
metatile attribute table, map collision behavior and BG/OBJ elevation
validation must be supplied before a playable registration can be made.
"""
import json
from pathlib import Path
from export_johto_tileset_bundle import export

REQUIREMENTS = {
    "primary_graphics_4bpp": True,
    "secondary_graphics_4bpp": True,
    "source_16x16_metatiles": True,
    "daytime_rgb555_palette": True,
    "primary_metatile_attributes": False,
    "secondary_metatile_attributes": False,
    "map_collision_and_ledge_semantics": False,
    "sprite_elevation_and_bg_priority": False,
    "roof_palette_routing": False,
    "isolated_map_header_and_warp": False,
}

def inspect(output):
    export(output)
    bundle = json.loads((output / "tileset_bundle.json").read_text())
    assert not bundle["map_registered"] and not bundle["tilesets_registered"]
    files = bundle["files"]
    assert files["johto_primary.4bpp"] == 16384
    assert files["johto_secondary.4bpp"] == 16384
    assert files["johto_primary_metatiles.bin"] == 512 * 16
    assert files["johto_secondary_metatiles.bin"] == 500 * 16
    assert files["johto_standard_day_palette_0_to_6.gbapal"] == 7 * 32
    unresolved = [key for key, ready in REQUIREMENTS.items() if not ready]
    report = {
        "status": "registration_blocked_pending_source_parity",
        "source_bundle_verified": True,
        "requirements": REQUIREMENTS,
        "blockers": unresolved,
        "first_test_map_registered": False,
        "existing_map_replacements": 0,
    }
    (output / "registration_preflight.json").write_text(json.dumps(report, indent=2) + "\n")
    assert unresolved and report["existing_map_replacements"] == 0
    return report

def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    report = inspect(parser.parse_args().output)
    print("PASS: Johto source bundle verified and registration preflight recorded")
    print("BLOCKED: " + ", ".join(report["blockers"]))
    print("No live tileset or map registration")

if __name__ == "__main__":
    main()
