#!/usr/bin/env python3
"""Audit Sprout's live geography, trainer data and encounter distribution."""
from collections import Counter
import json
import re
import struct

from build_new_bark import ROOT, source_files
from build_johto_demo import collisions, behavior_ids
from polished_sprout import MAPS, SLOT_WEIGHTS


def main():
    files = source_files()
    names = (*MAPS, "VioletCity")
    maps = {name: json.loads((ROOT / f"data/maps/Polished{name}/map.json").read_text()) for name in names}
    ids = {m["id"]: name for name, m in maps.items()}
    symbols = collisions(files["data/tilesets/sprout_tower_collision.asm"].decode())
    permissions = {s: p for p, s in re.findall(r"db (LAND_TILE|WALL_TILE|WATER_TILE)\s*; COLL_(\w+)", files["data/collision/collision_permissions.asm"].decode())}
    attrs = struct.unpack("<1024H", b"".join((ROOT / f"data/tilesets/{side}/polished_new_bark_sprout/metatile_attributes.bin").read_bytes() for side in ("primary", "secondary")))
    wild = json.loads((ROOT / "src/data/wild_encounters.json").read_text())["wild_encounter_groups"][0]["encounters"]
    for name, constant in MAPS.items():
        m = maps[name]
        src = files[f"maps/{name}.asm"].decode()
        blocks = files[f"maps/{name}.ablk"]
        cells = struct.unpack("<256H", (ROOT / f"data/layouts/Polished{name}/map.bin").read_bytes())
        assert m["map_type"] == "MAP_TYPE_UNDERGROUND" and m["allow_escaping"] and not m["allow_cycling"]
        for i, cell in enumerate(cells):
            x, y = i % 16, i // 16
            tile = blocks[y // 2 * 8 + x // 2] * 4 + y % 2 * 2 + x % 2
            assert cell & 0x3ff == tile
            assert bool(cell & 0xc00) == (permissions[symbols[tile]] == "WALL_TILE")
            assert cell >> 12 == 3
            if symbols[tile] == "FLOOR":
                assert attrs[tile] == behavior_ids()["MB_CAVE"]
        original = re.findall(r"warp_event\s+(\d+),\s*(\d+), (\w+), (\d+)", src)
        assert len(m["warp_events"]) == len(original)
        for w, (x, y, dest, number) in zip(m["warp_events"], original, strict=True):
            assert (w["x"], w["y"], w["dest_map"]) == (int(x), int(y), "MAP_POLISHED_" + dest)
            target = maps[ids[w["dest_map"]]]
            source_warps = re.findall(r"warp_event\s+(\d+),\s*(\d+),", files["maps/" + ids[w["dest_map"]] + ".asm"].decode())
            back = target["warp_events"][int(w["dest_warp_id"])]
            assert (back["x"], back["y"]) == tuple(map(int, source_warps[int(number) - 1]))
            assert back["dest_map"] == m["id"]
        original_actors = [(int(x), int(y)) for x, y in re.findall(r"(?:object|itemball)_event\s+(\d+),\s*(\d+),", src)]
        assert [(o["x"], o["y"]) for o in m["object_events"]] == original_actors
        tables = [row for row in wild if row["map"] == m["id"]]
        if name == "SproutTower1F":
            assert not tables
            continue
        body = files["data/wild/johto_grass.asm"].decode().split("def_grass_wildmons " + constant + "\n")[1].split("end_grass_wildmons")[0]
        slots = re.findall(r"wildmon (\d+), (\w+)", body)
        assert len(tables) == 3
        for n, table in enumerate(tables):
            actual, expected = Counter(), Counter()
            for mon, weight in zip(table["land_mons"]["mons"], SLOT_WEIGHTS, strict=True):
                assert mon["min_level"] == mon["max_level"]
                actual[mon["species"], mon["min_level"]] += weight
            for (level, species), weight in zip(slots[n * 7:n * 7 + 7], (30, 30, 20, 10, 5, 3, 2), strict=True):
                expected["SPECIES_" + species, int(level)] += weight
            assert actual == expected, (name, n, actual, expected)
    defs = re.findall(r"^#define (\w+)\s+(0x[\da-fA-F]+)", (ROOT / "include/constants/flags.h").read_text() + (ROOT / "include/constants/flags_frlg.h").read_text(), re.M)
    for value in range(0x4d9, 0x4e6):
        names = [n for n, v in defs if int(v, 16) == value]
        assert len(names) == 1 and names[0].startswith("FLAG_POLISHED_SPROUT_"), names
    print("PASS: Sprout Tower source geography, all stairs, actor order, three time tables, exact seven-slot probabilities and isolated save flags")


if __name__ == "__main__":
    main()
