#!/usr/bin/env python3
"""Source-backed but UNREGISTERED Johto Surf/shoreline collision probes.

Polished Crystal's generic WATER permission permits travel through water
and a return to LAND. Emerald needs a surfable behavior plus a 3 -> 1
elevation difference to start Surf, and 1 -> 3 to dismount. This exporter
makes a compact 3x3 patch of genuine source WATER quadrants and verifies
both sides of each shore for all four cardinal directions. Additional
negative probes catch blocked water, flat elevation and blocked shore.
Source IDs/geometry are authentic; synthesized placements are test-only.
No live attribute table or route map is generated.
"""
import hashlib
import json
import re
import struct
from pathlib import Path

from export_johto_basic_walkability_layout import build as basic_walkability
from project_johto_collision_symbols import convert

ROOT = Path(__file__).resolve().parents[3]
ELEV_LAND, ELEV_WATER = 3, 1
WIDTH = HEIGHT = 8
WATER_CELLS = {(x, y) for y in range(3, 6) for x in range(3, 6)}
# (standing on land, water cell faced from that shore, facing)
SHORES = (
    ((4, 2), (4, 3), "south"),
    ((4, 6), (4, 5), "north"),
    ((2, 4), (3, 4), "east"),
    ((6, 4), (5, 4), "west"),
)
VARIANT_MUTATIONS = {
    "baseline": None,
    "blocked_water": ((4, 3), 3, ELEV_WATER),
    "flat_water": ((4, 3), 0, ELEV_LAND),
    "blocked_shore": ((4, 2), 3, ELEV_LAND),
}


def check_native_permissions():
    pinned = (ROOT / "tools/polished_port/fixtures/johto_surf_permissions.asm").read_text(
        encoding="utf-8"
    )
    assert "Rangi42/polishedcrystal@3fa43192379df5c3e7b09a08e4d5d79af4f02f42" in pinned
    for line in ("call .CheckSurfable", "dec a ; cp WATER_TILE",
                 "jr z, .Land", "db WATER_TILE ; COLL_WATER",
                 "; Can walk back onto land from water."):
        assert line in pinned, "Source Crystal Surf permission changed: " + line

    header = (ROOT / "include/global.fieldmap.h").read_text(encoding="utf-8")
    fieldmap = (ROOT / "src/fieldmap.c").read_text(encoding="utf-8")
    move = (ROOT / "src/event_object_movement.c").read_text(encoding="utf-8")
    avatar = (ROOT / "src/field_player_avatar.c").read_text(encoding="utf-8")
    control = (ROOT / "src/field_control_avatar.c").read_text(encoding="utf-8")
    behavior = (ROOT / "src/metatile_behavior.c").read_text(encoding="utf-8")

    assert re.search(r"(?m)^\s*ELEVATION_SURF\s*=\s*1\s*,", header)
    assert re.search(r"(?m)^\s*ELEVATION_DEFAULT\s*=\s*3\s*,", header)
    assert re.search(r"(?m)^#define\s+MAPGRID_COLLISION_MASK\s+0x0C00\b", header)
    assert "return UNPACK_COLLISION(block);" in fieldmap
    assert "return MapGridGetMetatileAttributeAt(x, y, METATILE_ATTRIBUTE_BEHAVIOR);" in fieldmap

    native_collision = move.split("static enum Collision GetVanillaCollision(", 1)[1].split("\n}", 1)[0]
    assert native_collision.index("MapGridGetCollisionAt(x, y)") < native_collision.index(
        "IsElevationMismatchAt("
    )
    assert "return COLLISION_IMPASSABLE;" in native_collision
    assert "return COLLISION_ELEVATION_MISMATCH;" in native_collision
    mismatch = move.split("bool8 IsElevationMismatchAt(", 1)[1].split("\n}", 1)[0]
    assert "mapElevation != elevation" in mismatch
    assert "mapElevation == ELEVATION_TRANSITION" in mismatch

    enter = avatar.split("bool8 IsPlayerFacingSurfableFishableWater(", 1)[1].split("\n}", 1)[0]
    assert "GetCollisionAtCoords(" in enter and "COLLISION_ELEVATION_MISMATCH" in enter
    assert "PlayerGetElevation() == ELEVATION_DEFAULT" in enter
    assert "MetatileBehavior_IsSurfableFishableWater(" in enter
    exit_surf = avatar.split("static bool8 CanStopSurfing(", 2)[2].split("\n}", 1)[0]
    assert "PLAYER_AVATAR_FLAG_SURFING" in exit_surf
    assert "MapGridGetElevationAt(x, y) == ELEVATION_DEFAULT" in exit_surf
    assert "GetObjectEventIdByPosition(" in exit_surf
    check = avatar.split("enum Collision CheckForObjectEventCollision(", 1)[1].split("\n}", 1)[0]
    assert "collision == COLLISION_ELEVATION_MISMATCH && CanStopSurfing(x, y, direction)" in check
    assert "return COLLISION_STOP_SURFING;" in check
    assert "IsFieldMoveUnlocked(FIELD_MOVE_SURF)" in control
    assert "PartyHasMonWithSurf()" in control
    assert "CheckFollowerNPCFlag(FOLLOWER_NPC_FLAG_CAN_SURF)" in control

    fishable = behavior.split("bool8 MetatileBehavior_IsSurfableFishableWater(", 1)[1].split("\n}", 1)[0]
    diver = behavior.split("bool8 MetatileBehavior_IsDiveable(", 1)[1].split("\n}", 1)[0]
    flags = {}
    for kind in ("MB_POND_WATER", "MB_OCEAN_WATER"):
        assert kind in fishable
        match = re.search(r"(?m)^\s*\[" + kind + r"\]\s*=\s*([^,\n]+)", behavior)
        assert match, "Missing native water flags: " + kind
        flags[kind] = sorted(token.strip() for token in match.group(1).split("|"))
        assert "TILE_FLAG_SURFABLE" in flags[kind]
        assert "TILE_FLAG_HAS_ENCOUNTERS" in flags[kind]
        assert kind not in diver, "Standard Johto WATER must not become Dive water"
    assert flags["MB_POND_WATER"] == flags["MB_OCEAN_WATER"]
    return {
        "source_water_and_land_permissions_verified": True,
        "native_surf_entry_exit_gates_verified": True,
        "native_surf_field_move_and_follower_checks_verified": True,
        "pond_ocean_basic_movement_and_encounter_flags_equivalent": True,
        "native_water_flags": flags,
        "pond_ocean_gameplay_choice_still_unresolved": True,
        "live_transition_verified": False,
    }


def pack(mid, elevation, collision=0):
    assert 0 <= mid < 1012 and 0 <= collision <= 3 and 0 <= elevation <= 15
    word = mid | (collision << 10) | (elevation << 12)
    assert word & 0x03FF == mid
    assert (word >> 10) & 3 == collision
    assert word >> 12 == elevation
    return word


def export(output: Path, coverage: dict, primary: bytes, secondary: bytes,
           original_map: bytes):
    output.mkdir(parents=True, exist_ok=True)
    gates = check_native_permissions()
    assert len(primary) == 512 * 16 and len(secondary) == 500 * 16
    assert len(original_map) == 128
    assert len(coverage["metatiles"]) == 1012
    source = convert()
    rows = source["primary"] + source["secondary"]
    assert len(rows) == 1012
    all_water = [row for row in rows if row["source_collision_symbol"] == "WATER"]
    assert len(all_water) == 59
    water = all_water[0]
    mid = water["metatile_id"]
    source_geometry = primary if mid < 512 else secondary
    local = mid if mid < 512 else mid - 512
    assert hashlib.sha256(source_geometry[local*16:(local+1)*16]).hexdigest() == (
        coverage["metatiles"][mid]["source_geometry_sha256"]
    )
    assert coverage["metatiles"][mid]["candidate_behavior_word"] is None
    assert coverage["metatiles"][mid]["full_attribute_word"] is None
    floor = next(row for row in basic_walkability()
                 if row["source_symbol"] == "FLOOR"
                 and row["candidate_attribute_word"] == 0)
    floor_id = floor["metatile_id"]
    assert floor["packed_cell"] == pack(floor_id, ELEV_LAND)
    assert floor_id != mid
    assert len(WATER_CELLS) == 9 and len(SHORES) == 4

    variants = []
    for name, mutation in VARIANT_MUTATIONS.items():
        cells = []
        for y in range(HEIGHT):
            for x in range(WIDTH):
                cells.append(pack(mid, ELEV_WATER) if (x, y) in WATER_CELLS
                             else pack(floor_id, ELEV_LAND))
        if mutation is not None:
            (x, y), collision, elevation = mutation
            target = mid if (x, y) in WATER_CELLS else floor_id
            cells[y * WIDTH + x] = pack(target, elevation, collision)
        raw = struct.pack("<64H", *cells)
        assert len(raw) == 128
        filename = "johto_surf_" + name + ".OFFLINE.bin"
        (output / filename).write_bytes(raw)
        assert sum((word & 0x03FF) == mid for word in cells) == 9
        transitions = []
        for land, waterxy, facing in SHORES:
            landword = cells[land[1]*WIDTH + land[0]]
            waterword = cells[waterxy[1]*WIDTH + waterxy[0]]
            assert landword & 0x03FF == floor_id
            assert waterword & 0x03FF == mid
            enabled_entry = (landword >> 12 == ELEV_LAND
                             and (waterword >> 12) == ELEV_WATER
                             and not (waterword & 0x0C00))
            enabled_exit = (waterword >> 12 == ELEV_WATER
                            and landword >> 12 == ELEV_LAND
                            and not (landword & 0x0C00))
            transitions.append({
                "land_xy": list(land), "water_xy": list(waterxy),
                "land_facing": facing,
                "water_facing_for_shore_exit": {
                    "south": "north", "north": "south",
                    "east": "west", "west": "east",
                }[facing],
                "land_elevation": landword >> 12,
                "water_elevation": waterword >> 12,
                "land_collision": (landword >> 10) & 3,
                "water_collision": (waterword >> 10) & 3,
                "candidate_surf_entry_unblocked": enabled_entry,
                "candidate_shore_exit_unblocked": enabled_exit,
                "emulator_verified": False,
            })
        variants.append({
            "name": name, "filename": filename,
            "sha256": hashlib.sha256(raw).hexdigest(),
            "water_cells": len(WATER_CELLS),
            "transitions": transitions,
            "live_registered": False,
        })
    assert len({item["sha256"] for item in variants}) == 4
    baseline, blocked_water, flat_water, blocked_shore = variants
    assert all(t["candidate_surf_entry_unblocked"] and t["candidate_shore_exit_unblocked"]
               for t in baseline["transitions"])
    assert not blocked_water["transitions"][0]["candidate_surf_entry_unblocked"]
    assert blocked_water["transitions"][0]["water_collision"] == 3
    assert not flat_water["transitions"][0]["candidate_surf_entry_unblocked"]
    assert flat_water["transitions"][0]["water_elevation"] == ELEV_LAND
    assert not blocked_shore["transitions"][0]["candidate_shore_exit_unblocked"]
    assert blocked_shore["transitions"][0]["land_collision"] == 3
    assert hashlib.sha256(original_map).hexdigest() != baseline["sha256"]

    report = {
        "status": "OFFLINE_JOHTO_SURF_SHORELINE_PROBES_NOT_FOR_ROM",
        "source": source["source"],
        "source_water_quadrants": len(all_water),
        "source_water_metatile_id_used_for_probe": mid,
        "source_floor_metatile_id_used_for_probe": floor_id,
        "source_water_geometry_sha256": hashlib.sha256(
            source_geometry[local*16:(local+1)*16]
        ).hexdigest(),
        "source_base_map_sha256": hashlib.sha256(original_map).hexdigest(),
        "engine_and_original_surf_permissions": gates,
        "variants": variants,
        "candidate_pond_behavior": "MB_POND_WATER",
        "candidate_ocean_behavior": "MB_OCEAN_WATER",
        "source_water_does_not_distinguish_pond_from_ocean": True,
        "water_behavior_attribute_words_emitted": False,
        "map_registered": False,
        "emulator_surf_and_shore_exit_verified": False,
        "safe_to_enable": False,
    }
    (output / "johto_surf_shoreline_probes.review.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    return report
