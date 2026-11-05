#!/usr/bin/env python3
"""Verify Violet Mart against the pinned source and the prior map IDs."""
import json
import re
import struct

from build_new_bark import ROOT, source_files
from polished_mart import ITEMS, stock


def main():
    files = source_files()
    mart = json.loads((ROOT/"data/maps/PolishedVioletMart/map.json").read_text())
    city = json.loads((ROOT/"data/maps/PolishedVioletCity/map.json").read_text())
    assert [(w["x"],w["y"]) for w in city["warp_events"]] == [(31,25),(39,24),(39,25),(30,17),(9,17),(3,15),(21,29),(23,1),(18,17)]
    assert city["warp_events"][4]["dest_warp_id"] == "1"
    assert [(w["x"],w["y"],w["dest_warp_id"]) for w in mart["warp_events"]] == [(2,7,"4"),(3,7,"4")]
    objects = mart["object_events"]
    assert [(o["x"],o["y"]) for o in objects] == [(1,3),(7,6),(5,2)]
    assert [o["graphics_id"] for o in objects] == ["OBJ_EVENT_GFX_POLISHED_"+n for n in ("CLERK","GRANNY","ACE_TRAINER_M")]
    original = files["maps/VioletMart.asm"].decode()
    live = (ROOT/"data/maps/PolishedVioletMart/scripts.inc").read_text()
    assert re.findall(r"\.2byte ITEM_(\w+)",live) == [ITEMS.get(item,item) for item in stock(files)]
    assert "mart_clerk_event  1,  3, MARTTYPE_STANDARD, MART_VIOLET" in original
    for basename in ("map.bin","border.bin"):
        assert (ROOT/"data/layouts/PolishedVioletMart"/basename).read_bytes() == (ROOT/"data/layouts/PolishedCherrygroveMart"/basename).read_bytes()
    assert re.search(r'VioletMart_BlockData:(?:\s*\w+_BlockData:{1,2}(?:[^\n]*)?)*\s*INCBIN "maps/Mart.ablk.lz"',files["data/maps/blocks.asm"].decode())
    palette = struct.unpack("<16H",(ROOT/"graphics/mail/polished_flower/palette.bin").read_bytes())
    rgb = re.findall(r"RGB\s+(\d+),\s*(\d+),\s*(\d+)",files["gfx/mail/mail.pal"].decode())[:4]
    assert list(palette[:4]) == [int(r)|int(g)<<5|int(b)<<10 for r,g,b in rgb]
    assert palette[4] == palette[0]
    chart = (ROOT/"graphics/mail/polished_flower/tiles.bin").read_bytes()
    tilemap = struct.unpack("<1024H",(ROOT/"graphics/mail/polished_flower/map.bin").read_bytes())
    assert len(chart) == 23*32 and max(tilemap) == 22
    assert all(0 < (value & 15) <= 4 and 0 < value >> 4 <= 4 for value in chart)
    print("PASS: source Violet Mart layout, 11-item stock, residents, stable warps and Flower Mail colors")


if __name__ == "__main__":
    main()
