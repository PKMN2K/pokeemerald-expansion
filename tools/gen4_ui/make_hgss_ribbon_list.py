#!/usr/bin/env python3
"""Adapt the verified HGSS summary panels to the Emerald ribbon owner list.

HGSS has no PokéNav owner list. Reuse its original summary header and party
card cells at native size, with lossless flips and repeated interior cells.
The committed summary source manifest links these cells to retail a/1/6/2.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

if __package__:
    from .make_hgss_ribbon_summary import ASSETS, validate_assets
else:
    from make_hgss_ribbon_summary import ASSETS, validate_assets

OUT = ASSETS / "owner_list"
LEFT, TOP, WIDTH, ROWS = 3, 3, 25, 6


def build():
    validate_assets()
    tiles = (ASSETS / "summary_bg.4bpp").read_bytes()
    screen = struct.unpack("<1024H", (ASSETS / "summary_bg.bin").read_bytes())

    def cell(x, y):
        return screen[y * 32 + x]

    # BG1 masks the circular BG2 scrolling buffer outside the six visible
    # rows. Tile zero is transparent; the other cells are original art.
    background = [0] * 1024
    for y in range(20):
        for x in range(30):
            background[y * 32 + x] = cell(x, y) if y < TOP else cell(29, 4)
            if LEFT <= x < LEFT + WIDTH and TOP <= y < TOP + ROWS * 2:
                background[y * 32 + x] = 0

    def transformed_tile(attr):
        assert attr >> 12 == 1, "Owner cards must use the original text palette"
        start = (attr & 1023) * 32
        source = tiles[start:start + 32]
        pixels = []
        for y in range(8):
            for x in range(8):
                sx = 7 - x if attr & 0x400 else x
                sy = 7 - y if attr & 0x800 else y
                pixels.append((source[sy * 4 + sx // 2] >> (4 * (sx % 2))) & 15)
        return bytes(pixels[i] | pixels[i + 1] << 4 for i in range(0, 64, 2))

    # The summary's party-panel top/bottom corners and edge interiors.
    # Window blitting has no hardware flip flags, so apply those losslessly.
    row_cells = [cell(0 if x == 0 else 9 if x == WIDTH - 1 else 2, y)
                 for y in (4, 7) for x in range(WIDTH)]
    outputs = {
        "background.bin": struct.pack("<1024H", *background),
        "row.4bpp": b"".join(transformed_tile(attr) for attr in row_cells),
    }
    manifest = {
        "source": "../summary_bg.source.json",
        "source_sha256": hashlib.sha256((ASSETS / "summary_bg.source.json").read_bytes()).hexdigest(),
        "adaptation": "Retail summary header and party frames; no scaling, repainting or recoloring",
        "viewport_tiles": [LEFT, TOP, WIDTH, ROWS * 2],
        "row_source_entries": row_cells,
        "outputs": {name: hashlib.sha256(data).hexdigest() for name, data in outputs.items()},
    }
    outputs["source.json"] = (json.dumps(manifest, indent=2) + "\n").encode()
    return outputs


def validate():
    for name, data in build().items():
        assert (OUT / name).read_bytes() == data, f"Stale HGSS owner-list asset: {name}"
    source = (ASSETS.parents[2] / "src/pokenav_ribbons_list.c").read_text()
    validate_list_layout(source, "sMonRibbonListBgTemplates", (
        "sRibbonsMonListWindowTemplate", "sRibbonsListTitleWindowTemplate"))
    print("Verified HGSS owner-list header, six-row mask and lossless party-card cells")


def validate_list_layout(source, bg_template, window_templates):
    """Check VRAM ownership for a user of the shared six-row HGSS panels."""
    def fields(block):
        return {key: int(value, 0) for key, value in re.findall(r"\.(\w+)\s*=\s*(0x[\dA-Fa-f]+|\d+)", block)}

    block = re.search(bg_template + r"\[\].*?\n};", source, re.S).group()
    bgs = {entry["bg"]: entry for entry in (fields(b) for b in re.findall(r"\{([^{}]+)\}", block))}
    assert [bgs[i]["charBaseIndex"] for i in (1, 2, 3)] == [1, 2, 1]
    assert [bgs[i]["priority"] for i in (1, 2, 3)] == [2, 3, 1]
    regions = [("shared BG0 tiles", 0, 114 * 32), ("shared BG0 map", 5 * 2048, 6 * 2048),
               ("HGSS panels", 0x4000, 0x4000 + len((ASSETS / "summary_bg.4bpp").read_bytes()))]
    for bg, entry in bgs.items():
        start = entry["mapBaseIndex"] * 2048
        regions.append((f"BG{bg} map", start, start + 2048))
    for name in window_templates:
        block = re.search(name + r"\s*=.*?\n};", source, re.S).group()
        entry = fields(block)
        assert entry["bg"] == 3 and entry["paletteNum"] == 1
        assert entry["tilemapTop"] + entry["height"] <= TOP
        start = 0x4000 + entry["baseBlock"] * 32
        regions.append((name, start, start + entry["width"] * entry["height"] * 32))
    # The standard list uses a full 32-tile-high circular window, including
    # two leading base tiles; it fits in character blocks 2 and 3.
    for field, value in (("item_X", LEFT), ("windowWidth", WIDTH), ("listTop", TOP), ("maxShowed", ROWS), ("fillValue", 1)):
        assert re.search(r"template\." + field + r"\s*=\s*" + str(value) + r";", source)
    regions.append(("owner rows", 0x8000, 0x8000 + (2 + WIDTH * 32) * 32))
    regions.sort(key=lambda item: item[1])
    for left, right in zip(regions, regions[1:]):
        assert left[2] <= right[1], f"VRAM collision: {left[0]} / {right[0]}"
    assert regions[-1][2] <= 0x10000


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.check:
        validate()
    else:
        OUT.mkdir(parents=True, exist_ok=True)
        for name, data in build().items():
            (OUT / name).write_bytes(data)
        print(f"Wrote original HGSS owner-list cells to {OUT}")
