#!/usr/bin/env python3
"""Audit Cherrygrove's live data against the pinned source map and encounter files."""
from collections import Counter
import json
import re
import struct

from build_new_bark import ROOT, source_files
from build_johto_demo import palette_rows
from polished_cherrygrove import MAPS, BLOCKS


def main():
    files = source_files()
    city = json.loads((ROOT/"data/maps/PolishedCherrygroveCity/map.json").read_text())
    route = json.loads((ROOT/"data/maps/PolishedRoute29/map.json").read_text())
    assert city["connections"] == [{"map":route["id"],"offset":0,"direction":"right"}]
    assert {"map":city["id"],"offset":0,"direction":"left"} in route["connections"]
    assert not any(n["graphics_id"]=="OBJ_EVENT_GFX_POLISHED_RIVAL" for n in city["object_events"])
    assert len(city["object_events"])==5 and len(city["coord_events"])==1
    warps = 0
    for name in MAPS:
        live=json.loads((ROOT/f"data/maps/Polished{name}/map.json").read_text())
        src=files[f"maps/{name}.asm"].decode()
        expected=[(int(x),int(y),dest,int(n)-1) for x,y,dest,n in re.findall(r"warp_event\s+(\d+),\s*(\d+),\s*(\w+),\s*(\d+)",src) if dest!="POKECENTER_2F"]
        actual=[(w["x"],w["y"],w["dest_map"].removeprefix("MAP_POLISHED_"),int(w["dest_warp_id"])) for w in live["warp_events"]]
        assert actual==expected,name
        warps+=len(actual)
        for obj in live["object_events"]:
            if obj["local_id"]=="LOCALID_CHERRYGROVE_NURSE":
                assert (obj["x"],obj["y"])==(5,1)
                continue
            if obj["local_id"]=="LOCALID_CHERRYGROVE_PIDGEY":
                assert re.search(r"pokemon_event\s+26,\s*13, PIDGEY",src)
                continue
            assert re.search(rf'object_event\s+{obj["x"]},\s*{obj["y"]},',src), obj
    assert warps==15
    # Every source guide/follower stop has walkable ground in the real map.
    cells=struct.unpack("<880H",(ROOT/"data/layouts/PolishedCherrygroveCity/map.bin").read_bytes())
    guide_script=(ROOT/"data/maps/PolishedCherrygroveCity/scripts.inc").read_text()
    pos=(32,6);player=(32,7)
    delta={"left":(-1,0),"right":(1,0),"up":(0,-1),"down":(0,1)}
    src=files["maps/CherrygroveCity.asm"].decode()
    for suffix in ("1","2","3","3_5","4","5"):
        body=re.search(r"^GuideGentMovement"+suffix+r":\n(.*?)step_end",src,re.M|re.S)[1]
        directions=re.findall(r"^\s*step_(left|right|up|down)\s*$",body,re.M)
        actual=re.search(r"^PolishedCherrygrove_Guide"+suffix+r":\n(.*?)step_end",guide_script,re.M|re.S)[1]
        assert re.findall(r"walk_(\w+)",actual)==directions
        for direction in directions:
            player=pos;dx,dy=delta[direction];pos=pos[0]+dx,pos[1]+dy
            assert cells[pos[1]*40+pos[0]]&0xfc00==0x3000,(suffix,pos)
    assert (pos,player)==((25,11),(24,11))
    # The map gift must be acknowledged only after native Bag insertion.
    assert re.search(r"giveitem ITEM_TOWN_MAP\s+goto_if_eq VAR_RESULT, FALSE, PolishedCherrygrove_MapFull\s+setflag FLAG_POLISHED_MAP_CARD",guide_script)
    common=(ROOT/"include/constants/flags.h").read_text();frlg=(ROOT/"include/constants/flags_frlg.h").read_text()
    for name in ("GUIDE_HOME","MAP_CARD","CHERRYGROVE_NUGGET","CHERRYGROVE_MYSTIC_WATER","READ_RED_JOURNAL"):
        full="FLAG_POLISHED_"+name;value=int(re.search(r"#define "+full+r"\s+(0x\w+)",common)[1],16)
        assert 0x4a7<=value<=0x4ab
        assert all(int(v,16)!=value or n==full for n,v in re.findall(r"#define (\w+)\s+(0x[\da-fA-F]+)",common+frlg))
    wild=json.loads((ROOT/"src/data/wild_encounters.json").read_text())["wild_encounter_groups"][0]
    water=next(r["water_mons"] for r in wild["encounters"] if r["map"]==city["id"])
    body=re.search(r"def_water_wildmons CHERRYGROVE_CITY\n(.*?)end_water_wildmons",files["data/wild/johto_water.asm"].decode(),re.S)[1]
    expected=Counter()
    for (level,species),weight in zip(re.findall(r"wildmon (\d+), (\w+)",body),(60,30,10)):
        expected["SPECIES_"+species,int(level)]+=weight
    actual=Counter()
    weights=next(f["encounter_rates"] for f in wild["fields"] if f["type"]=="water_mons")
    for mon,weight in zip(water["mons"],weights):
        assert mon["min_level"]==mon["max_level"]
        actual[mon["species"],mon["min_level"]]+=weight
    assert actual==expected
    assert water["encounter_rate"]==11 # Nearest native integer to source 6%: 11/180.
    # Validate the source special cases independently from the generator.
    special=files["engine/tilesets/tileset_palettes.asm"].decode()
    assert "ld hl, wBGPals1 palette PAL_BG_RED" in special
    assert "ld de, wBGPals1 palette PAL_BG_ROOF" in special
    assert "ld hl, MartBluePalette" in special
    assert re.search(r'CherrygroveMart_BlockData:\n(?:\w+_BlockData:\n)*GenericMart_BlockData::[^\n]*\n\s*INCBIN "maps/Mart.ablk.lz"',files["data/maps/blocks.asm"].decode())
    for bank in ("mart","pokecenter"):
        rows=palette_rows(files[f"gfx/tilesets/{bank}.pal"].decode())[:7]
        if bank=="pokecenter":
            rows[6]=rows[1]
        else:
            body=files["data/maps/palettes.asm"].decode().split("MartBluePalette:\n")[1].split("else")[0]
            rows[2]=[int(r)|int(g)<<5|int(b)<<10 for r,g,b in re.findall(r"RGB\s+(\d+),\s*(\d+),\s*(\d+)",body)]
        expected=b"".join(struct.pack("<16H",*row,row[0],*([0]*11)) for row in rows).ljust(512,b"\0")
        for side in ("primary","secondary"):
            assert (ROOT/f"data/tilesets/{side}/polished_new_bark_{bank}/palettes.bin").read_bytes()==expected
    for x,label in ((24,"MartSign"),(30,"PokeCenterSign")):
        assert any(e["x"]==x and e["y"]==3 and e["script"].endswith(label) for e in city["bg_events"])
    print("PASS: six source maps, fifteen reciprocal doors, exact guide route and first-visit residents")
    print("PASS: safe shared save flags, Bag-success-only Map Card and exact source water species/level weights")
    print("PASS: Johto Center red floor, generic Mart blue bank and source building signs")


if __name__=="__main__":
    main()
