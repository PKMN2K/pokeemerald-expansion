#!/usr/bin/env python3
"""Compare the Academy and Type Chart with the pinned GBC source."""
from hashlib import sha256
import json
import re
import struct
from build_new_bark import ROOT, source_files

# RGB555 frame independently decoded using the original gfx.c tile
# deduplication/interleaving, signed GBC tile IDs, attrmap and 40 OAM entries.
CHART_FRAME_SHA256 = "f368c10584eb4bd8578590ae77f341a8e612b6addec3d1da2c0f5cc2f984b3b2"


def chart_frame():
    stem = ROOT / "graphics/polished_type_chart"
    tilemap = struct.unpack("<1024H",(stem/"map.bin").read_bytes())
    gfx = (stem/"tiles.bin").read_bytes()
    pal = (stem/"palette.bin").read_bytes()
    frame = bytearray()
    for y in range(144):
        for x in range(160):
            tile=tilemap[(1+y//8)*32+5+x//8]
            color=gfx[tile*64+(y%8)*8+x%8]
            assert color and len(pal)>color*2
            frame+=pal[color*2:color*2+2]
    return frame


def main():
    files=source_files()
    assert sha256(chart_frame()).hexdigest()==CHART_FRAME_SHA256
    original = files["maps/EarlsPokemonAcademy.asm"].decode()
    academy=json.loads((ROOT/"data/maps/PolishedEarlsPokemonAcademy/map.json").read_text())
    source_obj = re.findall(r"^\s*object_event\s+(\d+),\s*(\d+),",original,re.M)
    assert [(o["x"],o["y"]) for o in academy["object_events"]] == [tuple(map(int,p)) for p in source_obj]
    source_bg = [(int(x),int(y),label) for x,y,label in
                 re.findall(r"^\s*bg_event\s+(\d+),\s*(\d+), BGEVENT_READ, (\w+)",original,re.M)]
    assert [(e["x"],e["y"],e["script"].removeprefix("PolishedEarlsPokemonAcademy_"))
            for e in academy["bg_events"][:4]] == source_bg
    blocks=files["maps/EarlsPokemonAcademy.ablk"]
    binary=(ROOT/"data/layouts/PolishedEarlsPokemonAcademy/map.bin").read_bytes()
    tiles=struct.unpack("<128H",binary)
    for i,tile in enumerate(tiles):
        x,y=i%8,i//8
        assert tile&1023==blocks[y//2*4+x//2]*4+(y%2)*2+x%2
    attrs=files["data/items/attributes.asm"].decode()
    assert "; TYPE CHART\n\tkey_item_attribute 0, ITEMMENU_CURRENT, ITEMMENU_CURRENT" in attrs
    assert 'and PAD_A | PAD_B' in files["engine/items/type_chart.asm"].decode()
    live=(ROOT/"data/maps/PolishedVioletCity/scripts.inc").read_text()
    steps=re.findall(r"^\s*run_step_(\w+)$",files["maps/VioletCity.asm"].decode(),re.M)
    groups=re.findall(r"^PolishedEarl_(?:FirstStep|Path\d+):\n(.*?)^\s*step_end",live,re.M|re.S)
    assert [d for group in groups for d in re.findall(r"walk_fast_(\w+)",group)]==steps
    assert re.search(r"removeobject LOCALID_VIOLET_EARL\s+setflag FLAG_POLISHED_EARL_IN_ACADEMY",live)
    print("PASS: source Academy layout, journal and board positions, six actors, tour steps and all 23,040 RGB555 Type Chart pixels")


if __name__=="__main__":
    main()
