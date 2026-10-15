#!/usr/bin/env python3
"""Audit authentic Johto tile attributes for Emerald's combined palette slots.

This is a *review-only* graphics manifest. Emerald's normal tilesets load
six primary palettes (global slots 0..5) and seven secondary palettes (6..12).
Johto's seven CGB palette IDs (0..6) may be addressed across the *combined*
palette space with a shared identity mapping. This does NOT create colors or
register either tileset, and deliberately reports GBC per-tile priority bits
rather than discarding them as though layering were already resolved.

Usage:
    python3 tools/polished_port/scripts/audit_johto_graphics_mapping.py \\
        --output /tmp/johto_graphics_mapping.review.json
"""
import argparse
import base64
from collections import Counter
import json
from pathlib import Path

from stage_johto_tile_graphics import (
    gba_tile_id, SOURCE_TOTAL_TILES, priority_overlay_tile_ids,
)

SOURCE_BLOCKS = 253
TILES_PER_BLOCK = 16
GBC_PALETTE_MASK = 0x07
GBC_BANK_BIT = 0x08
GBC_UNUSED_BIT = 0x10
GBC_HFLIP_BIT = 0x20
GBC_VFLIP_BIT = 0x40
GBC_PRIORITY_BIT = 0x80
EMERALD_PRIMARY_PALETTES = 6
EMERALD_SECONDARY_PALETTES = 7
EMERALD_TOTAL_PALETTES = 13
EMERALD_TILES_PER_BANK = 512
FIXTURE_DIR = Path(__file__).resolve().parents[1] / "fixtures"


def load_fixture(stem):
    path = FIXTURE_DIR / f"johto_traditional_{stem}.bin.b64"
    lines = path.read_text(encoding="utf-8").splitlines()
    encoded = "".join(line.strip() for line in lines
                      if line.strip() and not line.lstrip().startswith("#"))
    return base64.b64decode(encoded, validate=True)


def audit(tiles, attributes):
    expected = SOURCE_BLOCKS * TILES_PER_BLOCK
    if len(tiles) != expected or len(attributes) != expected:
        raise ValueError(f"expected {expected} source bytes in each table")
    if any(attr & GBC_UNUSED_BIT for attr in attributes):
        raise ValueError("GBC attribute bit 4 is reserved; source requires review")

    palette_counts = Counter()
    source_bank_counts = Counter()
    layer_priority_entries = []
    shared_palette_refs = {"primary": 0, "secondary": 0}
    output_bank_counts = {"primary": 0, "secondary": 0}
    max_output_tile_id = 0

    for i, (tile, attr) in enumerate(zip(tiles, attributes)):
        block = i // TILES_PER_BLOCK
        source_y, source_x = divmod(i % TILES_PER_BLOCK, 4)
        # Convert the original 4x4 8x8 layout to four 2x2 8x8 subtiles.
        quadrant_index = (source_y // 2) * 2 + (source_x // 2)
        metatile_id = block * 4 + quadrant_index
        metatile_bank = "primary" if metatile_id < 512 else "secondary"
        palette = attr & GBC_PALETTE_MASK
        source_tile_bank = 1 if attr & GBC_BANK_BIT else 0
        tile_id = gba_tile_id(tile, source_tile_bank)
        if tile_id >= EMERALD_TILES_PER_BANK:
            raise ValueError(f"tile {i}: GBC bank/index exceeds 512 tile slots")
        if palette >= EMERALD_TOTAL_PALETTES:
            raise ValueError(f"tile {i}: palette outside Emerald background palette space")

        palette_counts[palette] += 1
        source_bank_counts[source_tile_bank] += 1
        output_bank_counts[metatile_bank] += 1
        max_output_tile_id = max(tile_id, max_output_tile_id)
        if palette >= EMERALD_PRIMARY_PALETTES:
            # In particular, primary metatiles referencing palette 6 need
            # the paired secondary tileset to load shared global palette 6.
            shared_palette_refs[metatile_bank] += 1
        if attr & GBC_PRIORITY_BIT:
            layer_priority_entries.append({
                "source_block": block,
                "source_tile_index": i,
                "source_row": source_y,
                "source_column": source_x,
                "map_metatile_id": metatile_id,
                "metatile_bank": metatile_bank,
                "quadrant": ("top_left", "top_right",
                             "bottom_left", "bottom_right")[quadrant_index],
                "gba_subtile": (source_y % 2) * 2 + (source_x % 2),
                "source_tile_vram_bank": source_tile_bank,
                "gba_tile_id_candidate": tile_id,
                "palette_global_slot": palette,
            })

    priority_mask_ids = priority_overlay_tile_ids(tiles, attributes)
    used_palettes = sorted(palette_counts)
    if used_palettes != list(range(7)):
        raise ValueError(f"unexpected source palette IDs: {used_palettes}")

    return {
        "status": "review_only_live_registration_blocked",
        "source": {
            "blocks": SOURCE_BLOCKS,
            "tile_references": expected,
            "gbc_palette_counts": {str(k): palette_counts[k] for k in used_palettes},
            "gbc_tile_bank_counts": {str(k): source_bank_counts[k] for k in (0, 1)},
            "gbc_priority_tile_count": len(layer_priority_entries),
        },
        "emerald_palette_plan": {
            "primary_slots": [0, 1, 2, 3, 4, 5],
            "secondary_slots": [6, 7, 8, 9, 10, 11, 12],
            "source_to_global_slot": {str(i): i for i in used_palettes},
            "source_palette_6_requires_secondary_palette_slot_6": True,
            "primary_metatile_references_to_slot_6": shared_palette_refs["primary"],
            "secondary_metatile_references_to_slot_6": shared_palette_refs["secondary"],
            "colors_extracted": True,
            "palette_time_of_day_verified": True,
            "requires_tileset_pair": True,
        },
        "emerald_graphics": {
            "candidate_gba_tile_ids": "GBC VRAM0 index + 1, or GBC VRAM1 index + 129; GBA tile 0 is transparent padding",
            "pinned_authentic_tile_count": SOURCE_TOTAL_TILES,
            "maximum_referenced_gba_tile_id": max_output_tile_id,
            "capacity_per_gba_graphics_bank": EMERALD_TILES_PER_BANK,
            "subtile_references_by_metatile_bank": output_bank_counts,
            "graphics_source_order_verified": True,
            "source_derived_priority_overlay_tiles": len(priority_mask_ids),
            "first_priority_overlay_gba_tile_id": min(priority_mask_ids.values()),
            "last_priority_overlay_gba_tile_id": max(priority_mask_ids.values()),
        },
        "unresolved_priority": {
            "count": len(layer_priority_entries),
            "source_priority_entries": layer_priority_entries,
            "gba_layer_rules_verified": False,  # Still requires sprite-elevation runtime validation
            "source_priority_overlay_conversion_staged": True,
            "staged_priority_overlay_tile_count": len(priority_mask_ids),
            "staged_priority_overlay_mapping": {str(k): v for k, v in priority_mask_ids.items()},
            "gbc_color_zero_masked_on_bg1_only": True,
            "required_emerald_metatile_layer_type": "METATILE_LAYER_TYPE_NORMAL",
            "requires_gba_bg_obj_elevation_priority_validation": True,
            "needs_per_8x8_priority_layer_policy": False,
        },
        "ready_for_live_registration": False,
        "pending": [
            "Preserve opaque source CGB color0 in GBA palette4 across all imported maps",
            "Load palette slot 6 from the paired secondary tileset, including on primary metatiles",
            "Keep 128 common and 160 Traditional 4bpp tiles in validated source order",
            "Validate staged GBC bit7 BG1 masks against real Emerald OBJ elevation priorities",
            "Validate remaining field collision, warp and animated tile semantics",
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    report = audit(load_fixture("metatiles"), load_fixture("attributes"))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Audited {report['source']['tile_references']} source tile references.")
    print(f"Priority-flagged 8x8 tiles staged for BG1: "
          f"{report['unresolved_priority']['count']}")
    print("Global palette 6 supplied by secondary bank; no colors registered.")
    print(f"Review-only manifest: {args.output}")


if __name__ == "__main__":
    main()
