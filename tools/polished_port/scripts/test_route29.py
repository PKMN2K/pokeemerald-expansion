#!/usr/bin/env python3
"""Audit the live Route 29 import against its pinned source, not its report."""
from collections import Counter
import json
import re
import struct

from build_new_bark import ROOT, source_files
from polished_route29 import wild_entries


def main():
    files = source_files()
    wild = json.loads((ROOT / "src/data/wild_encounters.json").read_text())["wild_encounter_groups"][0]
    live = [r for r in wild["encounters"] if r["map"] == "MAP_POLISHED_ROUTE_29"]
    assert live == wild_entries(files)
    weights = next(f["encounter_rates"] for f in wild["fields"] if f["type"] == "land_mons")
    cumulative = list(map(int, re.findall(r"^\s*db (\d+)", files["data/wild/probabilities.asm"].decode().split("WaterMonProbTable:")[0], re.M)))
    source_weights = [b-a for a,b in zip([0]+cumulative, cumulative)]
    body = re.search(r"def_grass_wildmons ROUTE_29\n(.*?)end_grass_wildmons", files["data/wild/johto_grass.asm"].decode(), re.S)[1]
    source = re.findall(r"wildmon (\d+), (\w+)", body)
    for n, row in enumerate(live):
        actual, expected = Counter(), Counter()
        for mon, weight in zip(row["land_mons"]["mons"], weights):
            assert mon["min_level"] == mon["max_level"]
            actual[mon["species"], mon["min_level"]] += weight
        for (level, species), weight in zip(source[n*7:n*7+7], source_weights):
            expected["SPECIES_"+species, int(level)] += weight
        assert actual == expected and sum(actual.values()) == 100
    town = json.loads((ROOT / "data/maps/PolishedNewBarkTown/map.json").read_text())
    route = json.loads((ROOT / "data/maps/PolishedRoute29/map.json").read_text())
    gate = json.loads((ROOT / "data/maps/PolishedRoute2946Gate/map.json").read_text())
    assert town["connections"] == [{"map": route["id"], "offset": 0, "direction": "left"}]
    assert route["connections"] == [{"map": town["id"], "offset": 0, "direction": "right"}]
    assert route["map_type"] == "MAP_TYPE_ROUTE" and gate["map_type"] == "MAP_TYPE_INDOOR"
    assert route["warp_events"][0]["dest_warp_id"] == "0" and len(gate["warp_events"]) == 2
    cells = struct.unpack("<80H", (ROOT / "data/layouts/PolishedRoute2946Gate/map.bin").read_bytes())
    assert all(cells[x] & 0xC00 for x in (4,5))
    assert all(not cells[70+x] & 0xC00 for x in (4,5))
    # Source callbacks for late-game Lyra/Tuscany must not spawn them early.
    assert len(route["object_events"]) == 8 and len(gate["object_events"]) == 2
    assert not route["coord_events"] and len(route["bg_events"]) == 3
    flags = [r["flag"] for r in route["object_events"] if r["flag"] != "0"]
    assert len(flags) == len(set(flags)) == 3
    # Persistent flags must use previously unused IDs in every supported game.
    common = (ROOT / "include/constants/flags.h").read_text()
    frlg = (ROOT / "include/constants/flags_frlg.h").read_text()
    for name in flags:
        value = int(re.search(r"#define " + name + r"\s+(0x\w+)", common)[1], 16)
        for other, literal in re.findall(r"#define (\w+)\s+(0x[\da-fA-F]+)", common + frlg):
            assert int(literal,16) != value or other == name, (name, other)
    print("PASS: Route 29 live clock tables preserve source species/level weights, including merged duplicate slots")
    print("PASS: source connections, gate warp remapping, blocked onward exits, first-visit objects and shared flag IDs")


if __name__ == "__main__":
    main()
