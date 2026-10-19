#!/usr/bin/env python3
"""Audit the offline Johto border's source-derived subtile geometry.

Structural provenance only: no pixel-level visual or emulator verification.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from convert_johto_traditional_blocks import convert
from test_johto_metatile_banks import load_binary_fixture
from export_johto_basic_border_candidate import export as export_border
from stage_johto_tile_graphics import build_graphics, mask_source_zero_for_priority

def audit(output):
    output.mkdir(parents=True, exist_ok=True)
    border = export_border(output)
    metatile_id = border["metatile_id"]
    source_tiles = load_binary_fixture("metatiles")
    source_attributes = load_binary_fixture("attributes")
    geometry = convert(source_tiles, source_attributes)
    assert len(geometry) == 1012 * 16
    assert 0 <= metatile_id < 1012
    words = struct.unpack_from("<8H", geometry, metatile_id * 16)
    bottom, overlay = words[:4], words[4:]
    tile_ids = [word & 0x03FF for word in words]
    palettes = [((word >> 12) & 0xF) for word in words]
    assert all(1 <= tile_id <= 288 for tile_id in tile_ids[:4])
    assert all(tile_id == 0 or 289 <= tile_id <= 300 for tile_id in tile_ids[4:])
    assert all(0 <= palette <= 6 for palette in palettes)
    assert border["packed_cells"] == [
        metatile_id | (3 << 12)
    ] * 4
    primary, secondary, graphics_report = build_graphics()
    assert len(primary) == len(secondary) == 16384
    assert all(x == 0 for x in secondary)
    for lower, upper in zip(tile_ids[:4], tile_ids[4:]):
        raw = primary[lower * 32:(lower + 1) * 32]
        assert len(raw) == 32
        assert all(nibble in (1, 2, 3, 4)
                   for byte in raw for nibble in (byte & 15, byte >> 4))
        if upper:
            foreground = primary[upper * 32:(upper + 1) * 32]
            assert foreground == mask_source_zero_for_priority(raw)
    assert graphics_report["gba_pixel_zero_present_in_source_tiles"] is False
    report = {
        "status": "OFFLINE_SOURCE_GEOMETRY_AUDIT",
        "source": "Rangi42/polishedcrystal@3fa43192379df5c3e7b09a08e4d5d79af4f02f42",
        "metatile_id": metatile_id,
        "bank": "primary" if metatile_id < 512 else "secondary",
        "source_block": metatile_id // 4,
        "source_quadrant": ["top_left", "top_right", "bottom_left", "bottom_right"][metatile_id % 4],
        "bottom_layer_entries": list(bottom),
        "foreground_entries": list(overlay),
        "source_derived_geometry_sha256": hashlib.sha256(geometry[metatile_id * 16:(metatile_id + 1) * 16]).hexdigest(),
        "palette_ids": sorted(set(palettes)),
        "geometry_checked": True,
        "converted_4bpp_pixels_checked": True,
        "converted_graphics_sha256": hashlib.sha256(primary).hexdigest(),
        "pixel_visuals_verified": False,
        "border_art_verified": False,
        "runtime_priority_verified": False,
        "live_registration_allowed": False,
    }
    (output / "johto_border_graphics_audit.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    return report

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    audit(parser.parse_args().output)
    print("PASS: source-derived Johto border metatile geometry and palettes")
    print("OFFLINE: pixel visuals and live field behavior remain unverified")
