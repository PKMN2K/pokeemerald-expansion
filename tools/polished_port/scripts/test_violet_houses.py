#!/usr/bin/env python3
"""Compare the two houses with pinned source data and preserve existing IDs."""
import json
import re
import struct
from build_new_bark import ROOT, source_files
from polished_violet_houses import trade_data


def main():
    files=source_files()
    city=json.loads((ROOT/"data/maps/PolishedVioletCity/map.json").read_text())
    groups=json.loads((ROOT/"data/maps/map_groups.json").read_text())["gMapGroup_PolishedJohtoDemo"]
    assert groups[23:]==["Polished"+n for n in ("VioletCity","VioletPokeCenter1F",
        "EarlsPokemonAcademy","VioletMart","VioletNicknameSpeechHouse","VioletOnixTradeHouse")]
    assert [(w["x"],w["y"]) for w in city["warp_events"]]==[(31,25),(39,24),(39,25),(30,17),(9,17),(3,15),(21,29)]
    blocks=files["maps/TraditionalHouse.ablk"]
    for name,index in (("VioletNicknameSpeechHouse",5),("VioletOnixTradeHouse",6)):
        original=files["maps/"+name+".asm"].decode()
        data=json.loads((ROOT/("data/maps/Polished"+name+"/map.json")).read_text())
        assert [(o["x"],o["y"]) for o in data["object_events"]]==[
            tuple(map(int,p)) for p in re.findall(r"^\s*object_event\s+(\d+),\s*(\d+),",original,re.M)]
        assert [(w["x"],w["y"],w["dest_warp_id"]) for w in data["warp_events"]]==[(3,7,str(index)),(4,7,str(index))]
        assert city["warp_events"][index]["dest_map"]==data["id"]
        cells=struct.unpack("<64H",(ROOT/("data/layouts/Polished"+name+"/map.bin")).read_bytes())
        for i,cell in enumerate(cells):
            x,y=i%8,i//8
            assert cell&1023==blocks[y//2*4+x//2]*4+(y%2)*2+x%2
        assert {(e["x"],e["y"]) for e in data["bg_events"]}=={(0,1),(1,1)}
        assert cells[8]&0xc00 and cells[9]&0xc00
    live=(ROOT/"src/data/polished_kyle_trade.h").read_text()
    assert live==trade_data(files)
    flags=(ROOT/"include/constants/flags.h").read_text()
    assert re.findall(r"^#define (\w+)\s+0x4D8\b",flags,re.M)==["FLAG_POLISHED_KYLE_TRADE"]
    assert not re.search(r"^#define \w+\s+0x4D8\b",(ROOT/"include/constants/flags_frlg.h").read_text(),re.M)
    print("PASS: both source house layouts, actors, furniture, stable doors and Kyle's complete trade record")


if __name__=="__main__":
    main()
