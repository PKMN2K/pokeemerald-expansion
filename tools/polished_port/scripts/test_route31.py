#!/usr/bin/env python3
"""Check the live Route 31 map, encounters and story against the pinned assets."""
from collections import Counter
import json
import re
import struct
from build_new_bark import ROOT, source_files
from polished_route30 import wild_entries
from polished_route31 import CLOSED


def main():
    files = source_files()
    route, gate, south = [json.loads((ROOT/f"data/maps/Polished{name}/map.json").read_text())
                           for name in ("Route31","Route31VioletGate","Route30")]
    attrs = files["data/maps/attributes.asm"].decode()
    assert "connection north, Route31, ROUTE_31, -10" in attrs
    assert "connection south, Route30, ROUTE_30, 10" in attrs
    assert route["connections"] == [{"map":south["id"],"offset":20,"direction":"down"}]
    assert {"map":route["id"],"offset":-20,"direction":"up"} in south["connections"]
    for m, name, expected in ((route, "Route31", [(4,6),(4,7)]),
                              (gate, "Route31VioletGate", [(9,4),(9,5)])):
        assert [(w["x"],w["y"]) for w in m["warp_events"]] == expected
        assert [int(w["dest_warp_id"]) for w in m["warp_events"]] == [0,1]
        raw = (ROOT/f"data/layouts/Polished{name}/map.bin").read_bytes()
        width = 44 if name == "Route31" else 10
        for x,y in CLOSED[name]:
            assert struct.unpack_from("<H", raw, 2*(width*y+x))[0] & 0xC00
        script = files[f"maps/{name}.asm"].decode()
        assert len(m["object_events"]) == len(re.findall(r"^\s*(?:object|cuttree|fruittree|itemball)_event ",script,re.M))
        for o in m["object_events"]:
            assert re.search(rf"(?:object|cuttree|fruittree|itemball)_event +{o['x']},\s*{o['y']},",script)
    live = json.loads((ROOT/"src/data/wild_encounters.json").read_text())["wild_encounter_groups"][0]["encounters"]
    actual = [w for w in live if w["map"] == route["id"]]
    assert actual == wild_entries(files,31)
    grass = files["data/wild/johto_grass.asm"].decode().split("def_grass_wildmons ROUTE_31\n")[1].split("end_grass_wildmons")[0]
    for index,row in enumerate(actual):
        dist, source = Counter(), Counter()
        for m,weight in zip(row["land_mons"]["mons"],(20,20,10,10,10,10,5,5,4,4,1,1)):
            assert m["min_level"]==m["max_level"]
            dist[m["species"],m["min_level"]]+=weight
        slots=re.findall(r"wildmon (\d+), (\w+)",grass)[index*7:index*7+7]
        for (level,species),weight in zip(slots,(30,30,20,10,5,3,2)):
            source["SPECIES_"+species,int(level)]+=weight
        assert dist==source
    phone = files["engine/phone/scripts/mom.asm"].decode().split("MomPhoneLectureScript:\n")[1]
    assert "setevent EVENT_TALKED_TO_MOM_AFTER_MYSTERY_EGG_QUEST" in phone and "iftrue MomPhoneSaveMoneyScript" in phone
    flags=(ROOT/"include/constants/flags.h").read_text()+(ROOT/"include/constants/flags_frlg.h").read_text()
    defs=re.findall(r"^#define (\w+)\s+(0x[\da-fA-F]+)",flags,re.M)
    for val in range(0x4C7,0x4CF):
        matches=[name for name,n in defs if int(n,16)==val]
        assert len(matches)==1 and matches[0].startswith("FLAG_POLISHED_ROUTE31_"),(val,matches)
    print("PASS: Route 31 connections and both reciprocal gate doors, blocked onward exits, source objects, encounters and save allocations")


if __name__ == "__main__":
    main()
