#!/usr/bin/env python3
"""Audit live Falkner geography, source encounters, rewards and stable save IDs."""
from collections import Counter
import json
import re
import struct

from build_new_bark import ROOT, source_files
from build_johto_demo import collisions, behavior_ids, palette_rows, FIXTURES
from polished_falkner import MAPS, CLOSED, wild_entries
from polished_sprout import SLOT_WEIGHTS


def main():
    files = source_files()
    maps = {n: json.loads((ROOT / f"data/maps/Polished{n}/map.json").read_text())
            for n in (*MAPS, "VioletCity", "Route31", "VioletPokeCenter1F", "ElmsLab")}
    permissions = {s: p for p, s in re.findall(r"db (LAND_TILE|WALL_TILE|WATER_TILE)\s*; COLL_(\w+)", files["data/collision/collision_permissions.asm"].decode())}
    for name, constant in MAPS.items():
        bank = "cave" if name.startswith("Dark") else "gym"
        symbols = collisions(files[f"data/tilesets/{bank}_collision.asm"].decode())
        blocks = files[f"maps/{name}.ablk"]
        width, height = map(int, re.search(r"map_const " + constant + r",\s*(\d+),\s*(\d+)", files["constants/map_constants.asm"].decode()).groups())
        cells = struct.unpack(f"<{width * height * 4}H", (ROOT / f"data/layouts/Polished{name}/map.bin").read_bytes())
        for i, cell in enumerate(cells):
            x, y = i % (width * 2), i // (width * 2)
            tile = blocks[y // 2 * width + x // 2] * 4 + y % 2 * 2 + x % 2
            assert cell & 0x3ff == tile
            assert bool(cell & 0xc00) == (permissions[symbols[tile]] == "WALL_TILE" or (x, y) in CLOSED.get(name, ()))
            assert cell >> 12 == (1 if symbols[tile] == "WATER" else 3)
        src = files[f"maps/{name}.asm"].decode()
        expected = [(int(x), int(y)) for x, y in re.findall(r"(?:pokemon|object|smashrock|itemball)_event\s+(\d+),\s*(\d+),?", src)]
        assert [(o["x"], o["y"]) for o in maps[name]["object_events"]] == expected
        assert maps[name]["requires_flash"] == (bank == "cave")
        for warp in maps[name]["warp_events"]:
            dest = next(m for m in maps.values() if m["id"] == warp["dest_map"])
            back = dest["warp_events"][int(warp["dest_warp_id"])]
            assert back["dest_map"] == maps[name]["id"]
    assert maps["VioletCity"]["warp_events"][8]["dest_map"] == maps["VioletGym"]["id"]
    assert maps["Route31"]["warp_events"][2]["dest_map"] == maps["DarkCaveVioletEntrance"]["id"]
    assert len(maps["DarkCaveVioletEntrance"]["warp_events"]) == 1
    rows = palette_rows((FIXTURES / "johto_bg_tiles.pal").read_text())
    native = struct.unpack("<256H", (ROOT / "data/tilesets/primary/polished_new_bark_cave/palettes.bin").read_bytes())
    for i in range(8):
        assert list(native[16*i:16*i+4]) == rows[16+i]
    wild = json.loads((ROOT / "src/data/wild_encounters.json").read_text())["wild_encounter_groups"][0]["encounters"]
    live = [r for r in wild if r["map"] == maps["DarkCaveVioletEntrance"]["id"]]
    assert live == wild_entries(files)
    source = files["data/wild/johto_grass.asm"].decode().split("def_grass_wildmons DARK_CAVE_VIOLET_ENTRANCE\n")[1].split("end_grass_wildmons")[0]
    slots = re.findall(r"wildmon (\d+), (\w+)", source)
    for n, row in enumerate(live):
        actual, expected = Counter(), Counter()
        for m, weight in zip(row["land_mons"]["mons"], SLOT_WEIGHTS):
            actual[m["species"], m["min_level"]] += weight
        for (level, species), weight in zip(slots[n*7:n*7+7], (30, 30, 20, 10, 5, 3, 2)):
            expected["SPECIES_" + species, int(level)] += weight
        assert actual == expected
        water = Counter()
        for m, weight in zip(row["water_mons"]["mons"], (60, 30, 5, 4, 1)):
            water[m["species"], m["min_level"]] += weight
        assert water == {("SPECIES_MAGIKARP", 15): 60, ("SPECIES_MAGIKARP", 10): 30, ("SPECIES_MAGIKARP", 5): 10}
    defs = re.findall(r"^#define (\w+)\s+(0x[\da-fA-F]+)\b", (ROOT / "include/constants/flags.h").read_text() + (ROOT / "include/constants/flags_frlg.h").read_text(), re.M)
    for value in (*range(0x4e6, 0x4f0), 0x4f9, 0x4fa, 0x28e, 0x28f, 0x299):
        names = [n for n, v in defs if int(v, 16) == value]
        assert len(names) == 1 and names[0].startswith("FLAG_POLISHED_"), names
    assert maps["VioletPokeCenter1F"]["object_events"][5]["local_id"] == "LOCALID_VIOLET_EGG_AIDE"
    print("PASS: Dark Cave and Violet Gym source geometry, actors, night palettes, reciprocal doors, exact encounter weights and isolated save flags")


if __name__ == "__main__":
    main()
