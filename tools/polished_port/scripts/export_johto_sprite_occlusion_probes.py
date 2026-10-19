#!/usr/bin/env python3
"""Create an unregistered Johto sprite-occlusion and movement test fixture.

The source holds both ordinary WALL tiles and a few GBC bit7 foreground
WALL variants. Pick one of each, plus five source obstacle families, and
place each beside unobstructed floor positions for controlled emulator tests.
The 16x16 foreground pixel masks are calculated from authentic converted
4bpp graphics. No sprite test has been executed; engine registration and
unresolved behavior attributes are deliberately left disabled.
"""
import hashlib
import json
import re
import struct
from pathlib import Path

from export_johto_basic_walkability_layout import build as walkability
from export_johto_sparse_attributes import build as build_attributes, UNRESOLVED
from project_johto_collision_symbols import convert as source_collisions
from stage_johto_tile_graphics import load_source_metatile_tables, priority_overlay_tile_ids
from audit_johto_obstacle_foreground_priority import expected_entry

ROOT = Path(__file__).resolve().parents[3]
SIDE = 8
ELEVATION = 3
IMPASSABLE = 3
CATEGORIES = (
    ("ordinary_wall", "WALL", False),
    ("priority_wall", "WALL", True),
    ("void", "VOID", False),
    ("upper_wall", "TOP_WALL", False),
    ("lower_wall", "BOTTOM_WALL", False),
    ("buoy", "BUOY", False),
    ("whirlpool", "WHIRLPOOL", False),
)


def source_indices(metatile_id):
    block, quadrant = divmod(metatile_id, 4)
    y0, x0 = (quadrant // 2) * 2, (quadrant % 2) * 2
    return [(block * 16 + (y0 + y) * 4 + x0 + x)
            for y in range(2) for x in range(2)]


def mask_rows(top: tuple[int, ...], graphics: bytes):
    rows = []
    for y in range(16):
        mask = 0
        for x in range(16):
            tile = top[(y // 8) * 2 + x // 8] & 0x03FF
            assert tile < 512
            byte = graphics[tile * 32 + (y % 8) * 4 + (x % 8) // 2]
            pixel = (byte >> (4 if x % 2 else 0)) & 15
            assert pixel in (0, 1, 2, 3), "Foreground mask may not paint source color zero"
            if pixel:
                mask |= 1 << x
        rows.append(mask)
    return rows


def export(output: Path, original_map: bytes, primary_metatiles: bytes,
           secondary_metatiles: bytes, primary_graphics: bytes):
    output.mkdir(parents=True, exist_ok=True)
    assert len(original_map) == 128
    assert len(primary_metatiles) == 512 * 16
    assert len(secondary_metatiles) == 500 * 16
    assert len(primary_graphics) == 512 * 32
    original_words = struct.unpack("<64H", original_map)
    original_cells = walkability()
    assert original_words == tuple(x["packed_cell"] for x in original_cells)
    floor = next(x for x in original_cells if x["source_symbol"] == "FLOOR")
    assert floor["candidate_attribute_word"] == 0
    floor_word = floor["metatile_id"] | (ELEVATION << 12)
    assert floor_word == floor["packed_cell"]

    header = (ROOT / "include/global.fieldmap.h").read_text(encoding="utf-8")
    assert re.search(r"(?m)^\s*#define\s+MAPGRID_IMPASSABLE\s+MAPGRID_COLLISION_MASK\b", header)
    assert re.search(r"(?m)^\s*METATILE_LAYER_TYPE_NORMAL,\s*// Metatile uses middle and top bg layers", header)
    all_rows = source_collisions()
    source = all_rows["primary"] + all_rows["secondary"]
    words, bitmap, _ = build_attributes()
    raw_tiles, raw_attrs = load_source_metatile_tables()
    priority_ids = priority_overlay_tile_ids(raw_tiles, raw_attrs)
    assert len(source) == len(words) == 1012
    assert len(bitmap) == 127 and len(raw_attrs) == 253 * 16

    selected = []
    used = set()
    for label, symbol, expect_priority in CATEGORIES:
        options = [r for r in source
                   if r["source_collision_symbol"] == symbol
                   and bool(any(raw_attrs[i] & 0x80 for i in source_indices(r["metatile_id"])))
                   == expect_priority
                   and r["metatile_id"] not in used]
        assert options, f"Missing authentic {label} metatile"
        row = options[0]
        used.add(row["metatile_id"])
        selected.append((label, symbol, expect_priority, row))
    assert len(selected) == 7 and len(used) == 7

    words_out = [floor_word] * (SIDE * SIDE)
    cases = []
    for offset, (label, symbol, expect_priority, record) in enumerate(selected):
        x, y = offset + 1, 3
        metatile_id = record["metatile_id"]
        assert words[metatile_id] == UNRESOLVED
        assert not (bitmap[metatile_id // 8] & (1 << (metatile_id % 8)))
        bank = "primary" if metatile_id < 512 else "secondary"
        local = metatile_id if bank == "primary" else metatile_id - 512
        assert record["bank"] == bank and record["bank_local_index"] == local
        geometry = primary_metatiles if bank == "primary" else secondary_metatiles
        raw_geom = geometry[local * 16:(local + 1) * 16]
        assert len(raw_geom) == 16
        base, top = struct.unpack("<4H", raw_geom[:8]), struct.unpack("<4H", raw_geom[8:])
        indices = source_indices(metatile_id)
        for j, idx in enumerate(indices):
            expected = expected_entry(raw_tiles[idx], raw_attrs[idx])
            assert base[j] == expected
            if raw_attrs[idx] & 0x80:
                assert (top[j] & 0x03FF) == priority_ids[expected & 0x03FF]
                assert (top[j] & 0xFC00) == (expected & 0xFC00)
            else:
                assert top[j] == 0
        flags = [bool(raw_attrs[idx] & 0x80) for idx in indices]
        assert any(flags) == expect_priority
        mask = mask_rows(top, primary_graphics)
        pixel_count = sum(v.bit_count() for v in mask)
        assert (pixel_count > 0) == expect_priority
        packed = metatile_id | (IMPASSABLE << 10) | (ELEVATION << 12)
        assert packed & 0x0C00 == 0x0C00
        words_out[y * SIDE + x] = packed

        probes = []
        for side, yy, face in (("north", 2, "south"), ("south", 4, "north")):
            assert words_out[yy * SIDE + x] == floor_word
            probes.append({
                "side": side, "actor_x": x, "actor_y": yy,
                "actor_elevation": ELEVATION, "actor_facing": face,
                "actor_cell_expected_source_symbol": "FLOOR",
                "expected_floor_collision_bits": 0,
                "expected_target_collision_bits": IMPASSABLE,
                "movement_into_obstacle_should_be_blocked": True,
                "sprite_occlusion_must_be_visually_checked": True,
                "in_emulator_result": None,
            })
        cases.append({
            "case_id": label, "source_collision_symbol": symbol,
            "obstacle_x": x, "obstacle_y": y,
            "metatile_id": metatile_id, "source_block": metatile_id // 4,
            "bank": bank, "bank_local_index": local,
            "packed_map_cell": packed,
            "original_geometry_sha256": hashlib.sha256(raw_geom).hexdigest(),
            "source_priority_subtiles": sum(flags),
            "expected_foreground_opaque_pixels": pixel_count,
            "foreground_opaque_mask_rows_hex": [f"{row:04x}" for row in mask],
            "probes": probes, "runtime_occlusion_verified": False,
        })

    assert words_out[3 * SIDE] == floor_word  # corridor connects both sides
    for case in cases:
        for p in case["probes"]:
            assert words_out[p["actor_y"] * SIDE + p["actor_x"]] == floor_word
    assert sum((word >> 10) & 3 == IMPASSABLE for word in words_out) == 7
    assert sum(bool(case["source_priority_subtiles"]) for case in cases) == 1
    assert cases[1]["case_id"] == "priority_wall"
    assert cases[1]["expected_foreground_opaque_pixels"] > 0
    output_map = struct.pack("<64H", *words_out)
    (output / "johto_sprite_occlusion_probes.OFFLINE.bin").write_bytes(output_map)
    result = {
        "status": "OFFLINE_SPRITE_OCCLUSION_PROBES_NOT_FOR_ROM",
        "map_width": SIDE, "map_height": SIDE, "map_bytes": len(output_map),
        "base_map_sha256": hashlib.sha256(original_map).hexdigest(),
        "probe_map_sha256": hashlib.sha256(output_map).hexdigest(),
        "ground_source_metatile_id": floor["metatile_id"],
        "corridor_x": 0,
        "proposed_debug_spawn": {"x": 0, "y": 3, "elevation": ELEVATION},
        "obstacle_case_count": len(cases),
        "player_probe_count": 2 * len(cases),
        "priority_mask_case_count": 1,
        "original_source_graphics_used": True,
        "mapgrid_collision_bits_source_checked": True,
        "foreground_masks_source_checked": True,
        "complete_metatile_attribute_tables": False,
        "sprite_overdraw_and_movement_verified_in_emulator": False,
        "debug_spawn_registered": False,
        "map_registered": False,
        "safe_to_enable": False,
        "cases": cases,
    }
    (output / "johto_sprite_occlusion_probes.review.json").write_text(
        json.dumps(result, indent=2) + "\n", encoding="utf-8"
    )
    return result
