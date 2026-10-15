#!/usr/bin/env python3
"""Regression tests for authentic Johto graphics bank/palette staging audit."""
from pathlib import Path

from audit_johto_graphics_mapping import (
    EMERALD_PRIMARY_PALETTES,
    EMERALD_SECONDARY_PALETTES,
    EMERALD_TOTAL_PALETTES,
    audit,
    load_fixture,
)

ROOT = Path(__file__).resolve().parents[3]


def rejects(tiles, attrs):
    try:
        audit(tiles, attrs)
    except ValueError:
        return
    raise AssertionError("invalid or unexpected source attributes were accepted")


def main():
    # These exact source fixtures are pinned to the same Polished Crystal
    # revision as the collision declarations, not generated samples.
    tiles = load_fixture("metatiles")
    attrs = load_fixture("attributes")
    report = audit(tiles, attrs)
    src = report["source"]
    slots = report["emerald_palette_plan"]
    flags = report["unresolved_priority"]
    gfx = report["emerald_graphics"]
    assert len(tiles) == len(attrs) == 4048
    assert (EMERALD_PRIMARY_PALETTES, EMERALD_SECONDARY_PALETTES,
            EMERALD_TOTAL_PALETTES) == (6, 7, 13)
    assert src["tile_references"] == 4048
    assert src["gbc_palette_counts"] == {
        "0": 1171, "1": 232, "2": 955, "3": 398,
        "4": 32, "5": 1132, "6": 128,
    }
    assert src["gbc_tile_bank_counts"] == {"0": 3162, "1": 886}
    assert gfx["maximum_referenced_gba_tile_id"] == 279
    assert gfx["subtile_references_by_metatile_bank"] == {
        "primary": 2048, "secondary": 2000,
    }
    assert slots["source_to_global_slot"] == {str(i): i for i in range(7)}
    assert slots["primary_slots"] == list(range(6))
    assert slots["secondary_slots"] == list(range(6, 13))
    assert slots["primary_metatile_references_to_slot_6"] == 80
    assert slots["secondary_metatile_references_to_slot_6"] == 48
    assert slots["requires_tileset_pair"]
    assert gfx["pinned_authentic_tile_count"] == 288
    assert gfx["source_derived_priority_overlay_tiles"] == 12
    assert gfx["first_priority_overlay_gba_tile_id"] == 289
    assert gfx["last_priority_overlay_gba_tile_id"] == 300
    assert slots["colors_extracted"]
    assert slots["palette_time_of_day_verified"]

    assert src["gbc_priority_tile_count"] == flags["count"] == 38
    assert len(flags["source_priority_entries"]) == 38
    assert sum(entry["metatile_bank"] == "primary"
               for entry in flags["source_priority_entries"]) == 2
    assert sum(entry["metatile_bank"] == "secondary"
               for entry in flags["source_priority_entries"]) == 36
    for entry in flags["source_priority_entries"]:
        index = entry["source_tile_index"]
        assert attrs[index] & 0x80
        assert entry["map_metatile_id"] == (
            entry["source_block"] * 4
            + (entry["source_row"] // 2) * 2
            + entry["source_column"] // 2
        )
        assert entry["gba_subtile"] == (
            (entry["source_row"] % 2) * 2 + entry["source_column"] % 2
        )
        assert entry["palette_global_slot"] == attrs[index] & 7

    assert not report["ready_for_live_registration"]
    assert not flags["gba_layer_rules_verified"]
    assert flags["source_priority_overlay_conversion_staged"]
    assert flags["staged_priority_overlay_tile_count"] == 12
    assert flags["required_emerald_metatile_layer_type"] == "METATILE_LAYER_TYPE_NORMAL"
    assert flags["gbc_color_zero_masked_on_bg1_only"]
    assert flags["requires_gba_bg_obj_elevation_priority_validation"]
    assert not flags["needs_per_8x8_priority_layer_policy"]
    assert gfx["graphics_source_order_verified"]

    # Test that this assumption remains true for the actual engine:
    # primary 6 + secondary 7; secondary palette loader starts at index 6.
    fieldmap_h = (ROOT / "include/fieldmap.h").read_text()
    fieldmap_c = (ROOT / "src/fieldmap.c").read_text()
    assert "#define NUM_PALS_IN_PRIMARY 6" in fieldmap_h
    assert "#define NUM_PALS_TOTAL 13" in fieldmap_h
    assert "tileset->palettes[numPalsInPrimary]" in fieldmap_c

    rejects(tiles[:-1], attrs)
    rejects(tiles, attrs + b"\\x00")
    mutant = bytearray(attrs)
    mutant[0] |= 0x10  # GBC attribute bit 4 is unused.
    rejects(tiles, bytes(mutant))
    mutant = bytearray(attrs)
    mutant[0] = (mutant[0] & 0xF8) | 7  # An unplanned eighth source palette.
    rejects(tiles, bytes(mutant))
    print("PASS: seven authentic CGB palettes fit six+seven *global* Emerald slots")
    print("PASS: 38 per-8x8 GBC priority records preserved for future layer mapping")
    print("PASS: registration blocked until actual colors, graphics and layers validated")


if __name__ == "__main__":
    main()
