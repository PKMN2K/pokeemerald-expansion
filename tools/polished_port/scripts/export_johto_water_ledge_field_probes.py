#!/usr/bin/env python3
"""Audit Johto water and ledges against actual Emerald field movement paths.

Provides isolated, UNREGISTERED 8x8 candidate maps for a shoreline, three
cardinal ledges and two source corner ledges from pinned Polished Crystal. All
source metatile IDs and geometry remain authentic. The map-cell elevation
and clear collision bits in each probe are HYPOTHESES for eventual emulator
tests, not claims of verified movement behavior.

Source DOWN_LEFT and DOWN_RIGHT ledges are NOT diagonal player movement:
Polished Crystal's facing masks allow south+west and south+east jumps.
This module verifies two isolated Emerald compatibility behaviors against
a pinned verbatim source table. No 0xFFFF attributes are converted to
normal terrain and no tilesets/maps are linked.
"""
import hashlib
import json
import re
import struct
from collections import Counter
from pathlib import Path

from audit_johto_ledge_water_fields import LEDGES, WATER
from export_johto_basic_walkability_layout import build as walkability
from export_johto_sparse_attributes import build as sparse, UNRESOLVED
from project_johto_collision_symbols import convert

ROOT = Path(__file__).resolve().parents[3]
SIDE = 8
ELEVATION_LAND = 3
ELEVATION_SURF = 1
COLLISION_CLEAR = 0
SYMBOL_COUNTS = {
    "WATER": 59, "LEDGE_DOWN": 7, "LEDGE_LEFT": 4,
    "LEDGE_RIGHT": 4, "LEDGE_DOWN_LEFT": 2, "LEDGE_DOWN_RIGHT": 2,
}
CARDINAL = {
    "LEDGE_DOWN": ("south", "MB_JUMP_SOUTH", (3, 2)),
    "LEDGE_LEFT": ("west", "MB_JUMP_WEST", (4, 3)),
    "LEDGE_RIGHT": ("east", "MB_JUMP_EAST", (2, 3)),
}
CORNER = {
    "LEDGE_DOWN_LEFT": ("MB_JOHTO_LEDGE_DOWN_LEFT", ("south", "west"), (3, 2)),
    "LEDGE_DOWN_RIGHT": ("MB_JOHTO_LEDGE_DOWN_RIGHT", ("south", "east"), (3, 2)),
}
DIAGONALS = set(CORNER)
SOURCE_FACING = {
    "FACE_RIGHT": "east", "FACE_LEFT": "west",
    "FACE_UP": "north", "FACE_DOWN": "south",
}


def verify_pinned_corner_masks():
    table = (ROOT / "tools/polished_port/fixtures/johto_ledge_facing_table.asm").read_text(
        encoding="utf-8"
    )
    expected = [
        "db FACE_RIGHT", "db FACE_LEFT", "db FACE_UP", "db FACE_DOWN",
        "db FACE_RIGHT | FACE_DOWN", "db FACE_DOWN | FACE_LEFT",
        "db FACE_UP | FACE_RIGHT", "db FACE_UP | FACE_LEFT",
    ]
    lines = [line.strip() for line in table.splitlines() if line.strip().startswith("db ")]
    assert lines == expected, "Pinned Crystal ledge-facing masks changed"
    right = {SOURCE_FACING[item.strip()] for item in lines[4][3:].split("|")}
    left = {SOURCE_FACING[item.strip()] for item in lines[5][3:].split("|")}
    assert right == set(CORNER["LEDGE_DOWN_RIGHT"][1]) == {"south", "east"}
    assert left == set(CORNER["LEDGE_DOWN_LEFT"][1]) == {"south", "west"}
    return {
        "LEDGE_DOWN_RIGHT": sorted(right),
        "LEDGE_DOWN_LEFT": sorted(left),
    }


def validate_engine():
    header = (ROOT / "include/global.fieldmap.h").read_text(encoding="utf-8")
    fieldmap = (ROOT / "src/fieldmap.c").read_text(encoding="utf-8")
    avatar = (ROOT / "src/field_player_avatar.c").read_text(encoding="utf-8")
    movement = (ROOT / "src/event_object_movement.c").read_text(encoding="utf-8")
    behavior = (ROOT / "src/metatile_behavior.c").read_text(encoding="utf-8")
    constants = (ROOT / "include/constants/metatile_behaviors.h").read_text(encoding="utf-8")
    assert re.search(r"(?m)^\s*ELEVATION_SURF\s*=\s*1\s*,", header)
    assert re.search(r"(?m)^\s*ELEVATION_DEFAULT\s*=\s*3\s*,", header)
    assert re.search(r"(?m)^#define\s+MAPGRID_COLLISION_MASK\s+0x0C00\b", header)
    assert "return UNPACK_COLLISION(block);" in fieldmap
    assert "return MapGridGetMetatileAttributeAt(x, y, METATILE_ATTRIBUTE_BEHAVIOR);" in fieldmap
    for const in (*LEDGES.values(), *WATER.values(), "MB_POND_WATER",
                  *[spec[0] for spec in CORNER.values()]):
        assert re.search(r"(?m)^\s*" + const + r"\s*,", constants), const
    enum_lines = [
        line.strip().rstrip(",") for line in constants.splitlines()
        if re.match(r"\s*MB_[A-Z0-9_]+\s*,", line)
    ]
    assert enum_lines.index("MB_JOHTO_LEDGE_DOWN_RIGHT") == enum_lines.index("MB_PRISM_MINING") + 1
    assert enum_lines.index("MB_JOHTO_LEDGE_DOWN_LEFT") == enum_lines.index("MB_JOHTO_LEDGE_DOWN_RIGHT") + 1
    assert len(enum_lines) < 255, "Metatile behavior IDs must fit u8"
    source_masks = verify_pinned_corner_masks()
    waterfn = behavior.split("bool8 MetatileBehavior_IsSurfableFishableWater(", 1)[1].split("\n}", 1)[0]
    assert "MB_OCEAN_WATER" in waterfn and "MB_POND_WATER" in waterfn
    playerSurf = avatar.split("bool8 IsPlayerFacingSurfableFishableWater(", 1)[1].split("\n}", 1)[0]
    assert "GetCollisionAtCoords(" in playerSurf
    assert "COLLISION_ELEVATION_MISMATCH" in playerSurf
    assert "PlayerGetElevation() == ELEVATION_DEFAULT" in playerSurf
    assert "MetatileBehavior_IsSurfableFishableWater(" in playerSurf
    stopSurf = avatar.split("static bool8 CanStopSurfing(", 2)[2].split("\n}", 1)[0]
    assert "PLAYER_AVATAR_FLAG_SURFING" in stopSurf
    assert "MapGridGetElevationAt(x, y) == ELEVATION_DEFAULT" in stopSurf
    jumpfn = movement.split("enum Direction GetLedgeJumpDirection(", 1)[1].split("\n}", 1)[0]
    for direction in ("South", "North", "East", "West"):
        assert "MetatileBehavior_IsJump" + direction in jumpfn
    assert "MetatileBehavior_IsJumpSoutheast" not in jumpfn
    assert "MetatileBehavior_IsJumpSouthwest" not in jumpfn
    assert "behavior == MB_JOHTO_LEDGE_DOWN_RIGHT" in jumpfn
    assert "behavior == MB_JOHTO_LEDGE_DOWN_LEFT" in jumpfn
    assert "index == DIR_SOUTH - 1 || index == DIR_EAST - 1" in jumpfn
    assert "index == DIR_SOUTH - 1 || index == DIR_WEST - 1" in jumpfn
    assert jumpfn.index("MB_JOHTO_LEDGE_DOWN_RIGHT") < jumpfn.index("ledgeBehaviorFuncs[index](behavior)")
    playerJump = avatar.split("static bool8 ShouldJumpLedge(", 2)[2].split("\n}", 1)[0]
    assert "GetLedgeJumpDirection(x, y, direction)" in playerJump
    assert "return COLLISION_LEDGE_JUMP;" in avatar
    return {
        "engine_elevation_constants_verified": True,
        "surfable_water_behavior_and_shore_entry_path_verified": True,
        "surf_exit_elevation_gate_verified": True,
        "cardinal_ledge_jump_dispatch_verified": True,
        "corner_ledge_cardinal_compatibility_supported_in_engine": True,
        "crystal_corner_facing_permissions": source_masks,
        "diagonal_jump_engine_support": False,
        "source_paths": [
            "include/global.fieldmap.h", "src/fieldmap.c",
            "src/metatile_behavior.c", "src/field_player_avatar.c",
            "src/event_object_movement.c",
        ],
    }


def pack(mid, elevation, collision=COLLISION_CLEAR):
    assert 0 <= mid < 1012
    assert 0 <= elevation <= 15 and 0 <= collision <= 3
    word = mid | (collision << 10) | (elevation << 12)
    assert word & 0x03FF == mid
    assert (word >> 10) & 3 == collision
    assert word >> 12 == elevation
    return word


def export(output: Path, coverage: dict, primary_metatiles: bytes,
           secondary_metatiles: bytes, original_map: bytes):
    output.mkdir(parents=True, exist_ok=True)
    engine = validate_engine()
    assert len(primary_metatiles) == 512 * 16
    assert len(secondary_metatiles) == 500 * 16
    assert len(original_map) == 8 * 8 * 2
    assert tuple(r["packed_cell"] for r in walkability()) == struct.unpack("<64H", original_map)
    floor = next(r for r in walkability()
                 if r["source_symbol"] == "FLOOR"
                 and r["candidate_attribute_word"] == 0)
    assert floor["packed_cell"] == pack(floor["metatile_id"], ELEVATION_LAND)
    source = convert()
    records = source["primary"] + source["secondary"]
    candidates, bitmap, _ = sparse()
    assert len(records) == len(coverage["metatiles"]) == len(candidates) == 1012
    assert len(bitmap) == 127
    assert not coverage["source_metatile_attribute_table_emitted"]

    selected = {}
    affected = []
    for mid, record in enumerate(records):
        symbol = record["source_collision_symbol"]
        if symbol not in SYMBOL_COUNTS:
            continue
        assert record["metatile_id"] == mid
        layer = coverage["metatiles"][mid]
        assert layer["metatile_id"] == mid and layer["source_collision_symbol"] == symbol
        assert layer["source_derived_layer_type"] == "METATILE_LAYER_TYPE_NORMAL"
        assert layer["candidate_behavior_word"] is None
        assert layer["full_attribute_word"] is None
        assert candidates[mid] == UNRESOLVED
        assert not bitmap[mid // 8] & (1 << (mid % 8))
        bank = "primary" if mid < 512 else "secondary"
        assert record["bank"] == layer["bank"] == bank
        local = mid if bank == "primary" else mid - 512
        geometry = primary_metatiles if bank == "primary" else secondary_metatiles
        raw = geometry[local * 16: (local + 1) * 16]
        assert hashlib.sha256(raw).hexdigest() == layer["source_geometry_sha256"]
        selected.setdefault(symbol, mid)

        if symbol == "WATER":
            direction, behavior, support = None, WATER["WATER"], "candidate_surf_behavior"
            expected_elevation = ELEVATION_SURF
        elif symbol in CARDINAL:
            direction, behavior, _ = CARDINAL[symbol]
            support, expected_elevation = "native_cardinal_jump_dispatch", ELEVATION_LAND
        else:
            behavior, directions, _ = CORNER[symbol]
            direction = "south_or_" + next(d for d in directions if d != "south")
            support = "two_cardinal_directions_supported_by_emerald_extension"
            expected_elevation = ELEVATION_LAND
        affected.append({
            "metatile_id": mid, "source_symbol": symbol,
            "source_block": record["source_block"],
            "source_quadrant": record["quadrant"],
            "bank": bank, "bank_local_index": local,
            "source_geometry_sha256": layer["source_geometry_sha256"],
            "candidate_emerald_behavior": behavior,
            "candidate_emerald_behavior_word": None,
            "candidate_layer_type": "METATILE_LAYER_TYPE_NORMAL",
            "proposed_direction": direction,
            "source_permitted_jump_directions": (
                list(CORNER[symbol][1]) if symbol in CORNER
                else [direction] if direction is not None else []
            ),
            "engine_support_status": support,
            "candidate_mapcell_elevation": expected_elevation,
            "candidate_mapcell_collision": COLLISION_CLEAR,
            "mapcell_collision_and_elevation_verified_in_emulator": False,
            "surf_jump_and_shore_transition_verified": False,
        })
    counts = dict(Counter(r["source_symbol"] for r in affected))
    assert counts == SYMBOL_COUNTS
    assert len(affected) == 78
    assert sum(r["source_symbol"] in DIAGONALS for r in affected) == 4
    assert all(x in selected for x in SYMBOL_COUNTS)

    maps = []
    # Six independent layouts: one Surf, three single-direction ledges and
    # two corner-ledge probes. Never modify the base Floor/Tall Grass map.
    for symbol in ("WATER", "LEDGE_DOWN", "LEDGE_LEFT", "LEDGE_RIGHT",
                   "LEDGE_DOWN_LEFT", "LEDGE_DOWN_RIGHT"):
        mid = selected[symbol]
        cells = [pack(floor["metatile_id"], ELEVATION_LAND)] * 64
        target = (3, 3)
        if symbol == "WATER":
            spawn, direction, elev = (3, 2), "south", ELEVATION_SURF
        elif symbol in CARDINAL:
            direction, _, spawn = CARDINAL[symbol]
            elev = ELEVATION_LAND
        else:
            direction = "south"
            spawn = CORNER[symbol][2]
            elev = ELEVATION_LAND
        target_word = pack(mid, elev)
        cells[target[1] * 8 + target[0]] = target_word
        assert cells[spawn[1] * 8 + spawn[0]] == pack(floor["metatile_id"], ELEVATION_LAND)
        assert sum(x != pack(floor["metatile_id"], ELEVATION_LAND) for x in cells) == 1
        raw_map = struct.pack("<64H", *cells)
        assert len(raw_map) == 128
        path = "johto_" + symbol.lower() + "_field_probe.OFFLINE.bin"
        (output / path).write_bytes(raw_map)
        maps.append({
            "source_symbol": symbol, "source_metatile_id": mid,
            "filename": path, "sha256": hashlib.sha256(raw_map).hexdigest(),
            "actor_spawn_xy": list(spawn), "actor_elevation": ELEVATION_LAND,
            "test_target_xy": list(target),
            "test_target_elevation_candidate": elev,
            "test_target_collision_candidate": COLLISION_CLEAR,
            "test_facing": direction,
            "additional_allowed_direction_probes": (
                [{"facing": "west" if symbol == "LEDGE_DOWN_LEFT" else "east",
                  "spawn_xy": [4, 3] if symbol == "LEDGE_DOWN_LEFT" else [2, 3]}]
                if symbol in CORNER else []
            ),
            "source_permitted_jump_directions": (
                list(CORNER[symbol][1]) if symbol in CORNER else [direction]
            ),
            "expected_runtime_action_to_check": (
                "surf_entry_then_shore_exit" if symbol == "WATER"
                else "directional_ledge_jump"
            ),
            "expected_runtime_result_verified": False,
            "geometry_and_map_encoding_source_verified": True,
        })
    assert len(maps) == 6 and len(set(m["sha256"] for m in maps)) == 6
    assert all(m["additional_allowed_direction_probes"] for m in maps[-2:])
    assert hashlib.sha256(original_map).hexdigest() != maps[0]["sha256"]

    report = {
        "status": "JOHTO_WATER_LEDGE_OFFLINE_ENGINE_REVIEW_NOT_FOR_ROM",
        "source": source["source"],
        "original_map_sha256": hashlib.sha256(original_map).hexdigest(),
        "source_water_metatiles": counts["WATER"],
        "source_cardinal_ledge_metatiles": sum(counts[s] for s in CARDINAL),
        "source_diagonal_ledge_metatiles": sum(counts[s] for s in DIAGONALS),
        "source_metatiles_checked": len(affected),
        "counts": counts, "native_engine_audit": engine,
        "water_behavior_candidate_from_existing_audit": WATER["WATER"],
        "pond_vs_ocean_water_choice_unresolved": True,
        "diagonal_ledges_require_new_engine_logic": False,
        "corner_ledge_engine_extension_compiled_but_not_playtested": True,
        "corner_ledge_source_facing_masks_verified": True,
        "offline_map_variants": maps,
        "full_attribute_words_emitted": False,
        "live_map_registered": False,
        "surf_transitions_verified_in_emulator": False,
        "ledge_jumps_verified_in_emulator": False,
        "safe_to_enable": False,
        "metatiles": affected,
    }
    (output / "johto_water_ledge_field_probes.review.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    return report
