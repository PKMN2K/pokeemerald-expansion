#!/usr/bin/env python3
"""Validate original Condition assets, live bindings and disjoint VRAM."""
import re
from pathlib import Path

from make_hgss_condition_graph import OUT, validate

ROOT = Path(__file__).resolve().parents[2]


def fields(block):
    return {key: int(value, 0) for key, value in
            re.findall(r"\.(\w+)\s*=\s*(0x[\dA-Fa-f]+|\d+)", block)}


def check():
    validate()
    source = (ROOT / "src/pokenav_conditions_gfx.c").read_text()
    block = re.search(r"sMenuBgTemplates\[\].*?\n};", source, re.S)[0]
    bgs = {entry["bg"]: entry for entry in (fields(b) for b in re.findall(r"\{([^{}]+)\}", block))}
    assert [bgs[i]["charBaseIndex"] for i in (1, 2, 3)] == [1, 3, 2]
    assert [bgs[i]["priority"] for i in (1, 2, 3)] == [1, 2, 3]
    regions = [("shared BG0 tiles", 0, 114 * 32), ("shared BG0 map", 5 * 2048, 6 * 2048),
               ("transparent BG1 tile", 0x4000, 0x4020),
               ("graph data tile", 0xC020, 0xC040),
               ("HGSS background", 0x8000, 0x8000 + (OUT / "graph_bg.4bpp").stat().st_size)]
    for bg, entry in bgs.items():
        start = entry["mapBaseIndex"] * 2048
        regions.append((f"BG{bg} map", start, start + 2048))
    for name in ("sTitleWindow", "sIndexWindow", "sNameWindow", "sLocationWindow", "sMarkingsWindow"):
        entry = fields(re.search(name + r"\s*=.*?;", source, re.S)[0])
        assert entry["bg"] == 1 and entry["paletteNum"] in (1, 12)
        assert entry["tilemapLeft"] + entry["width"] <= 30
        assert entry["tilemapTop"] + entry["height"] <= 16, "Help bar overlap"
        start = 0x4000 + entry["baseBlock"] * 32
        regions.append((name, start, start + entry["width"] * entry["height"] * 32))
    macros = {key: int(value) for key, value in re.findall(r"#define (MARKS_\w+) (\d+)", source)}
    assert macros["MARKS_LEFT"] + macros["MARKS_WIDTH"] <= 30
    assert macros["MARKS_TOP"] + macros["MARKS_HEIGHT"] <= 16
    size = (OUT / "markings_panel.4bpp").stat().st_size
    assert size == macros["MARKS_WIDTH"] * macros["MARKS_HEIGHT"] * 32
    start = 0x4000 + macros["MARKS_BASE_TILE"] * 32
    regions.append(("markings panel", start, start + size))
    regions.sort(key=lambda region: region[1])
    for left, right in zip(regions, regions[1:]):
        assert left[2] <= right[1], f"VRAM collision: {left[0]} / {right[0]}"
    assert regions[-1][2] <= 0x10000
    for name in ("graph_bg.4bpp", "graph_bg.bin", "sheen_stars.4bpp", "markings_panel.4bpp"):
        assert f'"graphics/gen4_ui/hgss_condition/{name}"' in source, name
    for legacy in ("graphics/pokenav/condition/", "DrawPokeGear", "LoadConditionSelectionIcons",
                   "LoadConditionSparkle", "SetMonMarkingsMenuPointer", "OpenMonMarkingsMenu"):
        assert legacy not in source, f"Legacy detail binding: {legacy}"
    # Feeding still owns these shared files; removing them would break it.
    feeding = (ROOT / "src/use_pokeblock.c").read_text()
    for name in ("graph_data.pal", "text.pal"):
        assert f'"graphics/pokenav/condition/{name}"' in feeding
    for name in ("graph_data.png", "graph_data.bin", "mon_markings.pal"):
        assert not (ROOT / "graphics/pokenav/condition" / name).exists(), f"Unused legacy detail art: {name}"
    print("Verified Condition detail bindings, help-bar clearance and disjoint VRAM; feeding retains its assets")


if __name__ == "__main__":
    check()
