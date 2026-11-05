#!/usr/bin/env python3
"""Check Violet's first-visit maps against pinned Polished Crystal source data."""
from collections import Counter
import json
import re
import struct

from build_new_bark import ROOT, source_files
from build_johto_demo import behavior_ids, collisions, FIXTURES
from polished_violet import CLOSED


def map_data(name):
    return json.loads((ROOT / f"data/maps/Polished{name}/map.json").read_text())


def main():
    files = source_files()
    city, center, gate, route, academy, mart, nickname, trade, sprout = map(map_data, ("VioletCity", "VioletPokeCenter1F", "Route31VioletGate", "Route31", "EarlsPokemonAcademy", "VioletMart", "VioletNicknameSpeechHouse", "VioletOnixTradeHouse", "SproutTower1F"))
    maps = {row["id"]: row for row in (city, center, gate, route, academy, mart, nickname, trade, sprout)}
    for row, expected in (
        (city, [(31, 25), (39, 24), (39, 25), (30, 17), (9, 17), (3, 15), (21, 29), (23, 1)]),
        (academy, [(3, 15), (4, 15)]),
        (mart, [(2, 7), (3, 7)]),
        (center, [(5, 7), (6, 7)]),
        (gate, [(9, 4), (9, 5), (0, 4), (0, 5)]),
    ):
        assert [(w["x"], w["y"]) for w in row["warp_events"]] == expected
        src = files["maps/"+row["name"].removeprefix("Polished")+".asm"].decode()
        original_warps = {(int(x),int(y)): (dest,int(target)) for x,y,dest,target in
                          re.findall(r"warp_event\s+(\d+),\s*(\d+),\s*(\w+),\s*(\d+)", src)}
        for w in row["warp_events"]:
            dest, target = original_warps[w["x"], w["y"]]
            destination = maps[w["dest_map"]]
            assert destination["id"] == "MAP_POLISHED_"+dest
            d_script = files["maps/"+destination["name"].removeprefix("Polished")+".asm"].decode()
            d_xy = re.findall(r"warp_event\s+(\d+),\s*(\d+),", d_script)[target-1]
            back = destination["warp_events"][int(w["dest_warp_id"])]
            assert back["dest_map"] == row["id"] and (back["x"],back["y"]) == tuple(map(int,d_xy))

    blocks = files["maps/VioletCity.ablk"]
    data = (ROOT/"data/layouts/PolishedVioletCity/map.bin").read_bytes()
    assert len(blocks) == 20*18 and len(data) == 2*40*36
    cells = struct.unpack("<1440H", data)
    symbols = collisions((FIXTURES/"johto_traditional_collision.asm").read_text())
    permissions = dict((sym,perm) for perm,sym in re.findall(r"db (LAND_TILE|WALL_TILE|WATER_TILE)\s*; COLL_(\w+)",
                                                              files["data/collision/collision_permissions.asm"].decode()))
    for i, cell in enumerate(cells):
        x, y = i%40, i//40
        tile = blocks[y//2*20+x//2]*4 + (y%2)*2 + x%2
        assert cell & 0x3ff == tile
        assert bool(cell & 0xc00) == (permissions[symbols[tile]]=="WALL_TILE" or (x,y) in CLOSED["VioletCity"])
        assert cell >> 12 == (1 if symbols[tile]=="WATER" else 3)
    assert "connection east, Route31, ROUTE_31, 9" in files["data/maps/attributes.asm"].decode()
    assert "connection west, VioletCity, VIOLET_CITY, -9" in files["data/maps/attributes.asm"].decode()
    assert city["connections"] == [{"map":route["id"], "offset":18, "direction":"right"}]
    assert {"map":city["id"], "offset":-18, "direction":"left"} in route["connections"]
    assert "ROOF_VIOLET   ; 10" in files["data/maps/roofs.asm"].decode()
    assert re.search(r"VIOLET_CITY,\s+PAL_TIMEOFDAY, VioletEcruteakPalette", files["data/maps/palettes.asm"].decode())
    ids = behavior_ids()
    attrs = b"".join((ROOT/f"data/tilesets/{side}/polished_new_bark_outdoor/metatile_attributes.bin").read_bytes()
                     for side in ("primary","secondary"))
    for tile in {c & 0x3ff for c in cells if symbols[c & 0x3ff] == "TOP_WALL"}:
        assert struct.unpack_from("<H", attrs, tile*2)[0] == ids["MB_IMPASSABLE_NORTH"]

    src = files["maps/VioletCity.asm"].decode()
    # Earl is appended so earlier city saves keep their resident IDs.
    coordinates = [(int(x),int(y)) for x,y in re.findall(r"(?:object|cuttree|fruittree|itemball)_event\s+(\d+),\s*(\d+),", src)]
    assert Counter((o["x"],o["y"]) for o in city["object_events"]) == Counter(coordinates)
    hidden = [(int(x),int(y),"ITEM_"+item) for x,y,item in re.findall(r"bg_event\s+(\d+),\s*(\d+), BGEVENT_ITEM \+ (\w+),",src)]
    assert [(e["x"],e["y"],e["item"]) for e in city["bg_events"] if e["type"]=="hidden_item"] == hidden
    water = files["data/wild/johto_water.asm"].decode().split("def_water_wildmons VIOLET_CITY\n")[1].split("end_water_wildmons")[0]
    assert "2 percent" in water
    original = Counter({("SPECIES_"+sp,int(level)):prob for (level,sp),prob in
                        zip(re.findall(r"wildmon (\d+), (\w+)",water),(60,30,10),strict=True)})
    groups = json.loads((ROOT/"src/data/wild_encounters.json").read_text())["wild_encounter_groups"][0]
    live, = [w["water_mons"] for w in groups["encounters"] if w["map"]==city["id"]]
    assert live["encounter_rate"] == 4
    actual = Counter()
    for mon,weight in zip(live["mons"],(60,30,5,4,1),strict=True):
        assert mon["min_level"] == mon["max_level"]
        actual[mon["species"],mon["min_level"]] += weight
    assert original == actual
    headers = (ROOT/"include/constants/flags.h").read_text()+(ROOT/"include/constants/flags_frlg.h").read_text()
    allocations = re.findall(r"^#define (\w+)\s+(0x[\da-fA-F]+)",headers,re.M)
    for value in range(0x4cf,0x4d6):
        names = [name for name,address in allocations if int(address,16)==value]
        assert len(names)==1 and names[0].startswith("FLAG_POLISHED_"), names
    print("PASS: Violet City geography, original warp destinations, directional walls, encounters, residents and save allocations")


if __name__ == "__main__":
    main()
