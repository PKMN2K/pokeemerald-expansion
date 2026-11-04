#!/usr/bin/env python3
"""Adapt verified HGSS summary source cells to Emerald's 240x160 screen.

No painting, scaling, quantization or cross-screen layer compositing. Each
output cell is an unchanged original 8x8 tile (with hardware flip flags).
Repeated interior cells shorten/extend the original panels for the GBA.
"""
import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from render_hgss_ribbon_backgrounds import character_data, narc_members, u16
from export_hgss_ribbon_gba_bg import verify_source_narc

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "graphics/gen4_ui/hgss_ribbons"
PALETTE_MAP = {0: 1, 4: 7}


def source_cells():
    # NCGR 1 / NSCR 9 is the retail SUB BG2 Ribbons page. BG3 is on
    # the other DS screen and must NOT be alpha-composited over this map.
    cells = [(19, 4, 0)] * 600  # original pale panel surface

    def put(x, y, sx, sy, flip=0):
        assert 0 <= x < 30 and 0 <= y < 20
        cells[y * 30 + x] = (sx, sy, flip)

    # Original yellow header, with the interior repeated horizontally.
    for y in range(4):
        for x in range(30):
            put(x, y, 19 if x == 0 else 31 if x == 29 else 20, y)

    def panel(x, y, w, h):
        # Original party portrait frame: source x=160..199, y=40..71.
        # Mirror its left edge to avoid copying the adjacent portrait at x=200.
        # Preserve its rounded corners and repeat only interior edge cells.
        for yy in range(h):
            for xx in range(w):
                put(x + xx, y + yy,
                    20 if xx in (0, w - 1) else 22,
                    5 if yy in (0, h - 1) else 6,
                    (0x400 if xx == w - 1 else 0) | (0x800 if yy == h - 1 else 0))

    panel(0, 4, 10, 4)    # owner index, text inside x=8..63/y=40..55
    panel(0, 8, 10, 8)    # Pokemon portrait, center (40,96)
    panel(10, 12, 20, 4)  # two-line details/count, above shared help bar

    # Native yellow medal case. The retail right edge also supplies the
    # left edge by the GBA's horizontal-flip bit; no pixels are redrawn.
    for y in range(9):
        for x in range(19):
            edge = x in (0, 18)
            put(10 + x, 3 + y, 18 if edge else 0,
                1, 0x400 if x == 0 else 0)
    # Reuse one intact 32x32 circle cell for each of the eight native icons.
    # HGSS's three-column touch grid becomes the existing four-column grid.
    for row in range(2):
        for col in range(4):
            for y in range(4):
                for x in range(4):
                    put(11 + col * 4 + x, 4 + row * 4 + y, 2 + x, 1 + y)
    return cells


def export(archive, out):
    verify_source_narc(archive)
    members = narc_members(archive)
    raw = character_data(members[1], 512)
    tiles = [bytes(32)]
    tile_ids = {}
    screen = [0] * 1024
    sources = []
    for i, (x, y, flip) in enumerate(source_cells()):
        attr = u16(members[9], 36 + (y * 32 + x) * 2)
        source_id, bank = attr & 1023, attr >> 12
        tile = raw[source_id * 32:(source_id + 1) * 32]
        assert bank in PALETTE_MAP, (x, y, bank)
        if tile not in tile_ids:
            tile_ids[tile] = len(tiles)
            tiles.append(tile)
            sources.append({"tile": len(tiles) - 1, "ncgr": 1, "source_tile": source_id,
                            "sha256": hashlib.sha256(tile).hexdigest()})
        screen[(i // 30) * 32 + i % 30] = tile_ids[tile] | ((attr ^ flip) & 0xC00) | (PALETTE_MAP[bank] << 12)
    assert len(tiles) <= 512  # BG2 has its own 16 KiB character block
    outputs = {"summary_bg.4bpp": b"".join(tiles),
               "summary_bg.bin": struct.pack("<1024H", *screen)}
    for bank, dest in PALETTE_MAP.items():
        outputs[f"summary_bg.bank_{dest}.gbapal"] = members[0][40 + bank * 32:72 + bank * 32]
    manifest = {
        "source_narc_sha256": hashlib.sha256(archive).hexdigest(),
        "ncgr": 1, "nscr": 9, "nclr": 0,
        "source_screen": "sub engine BG2 (global layer 6), not main BG3",
        "adaptation": "Original header, party-panel frame and medal-case cells; repeated interiors, native 8x8 pixels",
        "palette_map": PALETTE_MAP, "tile_count": len(tiles), "tiles": sources,
        "outputs": {name: hashlib.sha256(data).hexdigest() for name, data in outputs.items()},
    }
    out.mkdir(parents=True, exist_ok=True)
    for name, data in outputs.items():
        (out / name).write_bytes(data)
    (out / "summary_bg.source.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Exported {len(tiles)} original tiles; BG palettes 1/7; no recoloring")


def render_assets(folder):
    from PIL import Image
    raw = (folder / "summary_bg.4bpp").read_bytes()
    screen = struct.unpack("<1024H", (folder / "summary_bg.bin").read_bytes())
    pals = {bank: struct.unpack("<16H", (folder / f"summary_bg.bank_{bank}.gbapal").read_bytes())
            for bank in PALETTE_MAP.values()}
    image = Image.new("RGB", (240, 160))
    for y in range(160):
        for x in range(240):
            attr = screen[(y // 8) * 32 + x // 8]
            px, py = x % 8, y % 8
            if attr & 0x400: px = 7 - px
            if attr & 0x800: py = 7 - py
            byte = raw[(attr & 1023) * 32 + py * 4 + px // 2]
            color = pals[attr >> 12][(byte >> (4 * (px % 2))) & 15]
            image.putpixel((x, y), tuple(((color >> shift) & 31) * 255 // 31 for shift in (0, 5, 10)))
    return image


def validate_assets(folder=ASSETS):
    manifest = json.loads((folder / "summary_bg.source.json").read_text())
    assert manifest["source_narc_sha256"] == "003c80069fcd1f9c9eb842ae0294ecbdeffabbff40593cc471236e81239d9eed"
    assert (manifest["ncgr"], manifest["nscr"], manifest["nclr"]) == (1, 9, 0)
    for name, expected in manifest["outputs"].items():
        assert hashlib.sha256((folder / name).read_bytes()).hexdigest() == expected, name
    raw = (folder / "summary_bg.4bpp").read_bytes()
    assert len(raw) == manifest["tile_count"] * 32 <= 0x4000
    assert raw[:32] == bytes(32)
    for tile in manifest["tiles"]:
        start = tile["tile"] * 32
        assert hashlib.sha256(raw[start:start + 32]).hexdigest() == tile["sha256"]
    entries = struct.unpack("<1024H", (folder / "summary_bg.bin").read_bytes())
    for i, attr in enumerate(entries):
        if i % 32 < 30 and i // 32 < 20:
            assert attr & 1023 < manifest["tile_count"]
            assert attr >> 12 in PALETTE_MAP.values()
        else:
            assert attr == 0
    source = (ROOT / "src/pokenav_ribbons_summary.c").read_text()
    # BG1 holds 193 icon tiles, then disjoint text allocations. A full BG1
    # clear on a page change would erase these window map entries.
    used = set(range(193))
    for name in ("sRibbonCountWindowTemplate", "sRibbonSummaryMonNameWindowTemplate", "sRibbonMonListIndexWindowTemplate", "sRibbonTitleWindowTemplate"):
        block = re.search(r"static const struct WindowTemplate " + name + r".*?\n};", source, re.S).group()
        fields = {key: int(value, 0) for key, value in re.findall(r"\.(\w+)\s*=\s*(0x[\dA-Fa-f]+|\d+)", block)}
        assert fields["bg"] == fields["paletteNum"] == 1
        start = fields["baseBlock"]
        end = start + fields["width"] * fields["height"]
        assert 193 <= start < end <= 512
        assert not used.intersection(range(start, end))
        used.update(range(start, end))
        x, y, w, h = (fields[k] for k in ("tilemapLeft", "tilemapTop", "width", "height"))
        assert x + w <= 30 and y + h <= 16
        assert not (x < 27 and x + w > 11 and y < 12 and y + h > 4), name
    print(f"Verified {manifest['tile_count']} source-exact summary tiles, BG banks 1/7 and disjoint text/icon VRAM")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("narc", type=Path)
    parser.add_argument("--out", type=Path, default=ASSETS)
    parser.add_argument("--preview", type=Path)
    args = parser.parse_args()
    export(args.narc.read_bytes(), args.out)
    if args.preview:
        render_assets(args.out).save(args.preview)
