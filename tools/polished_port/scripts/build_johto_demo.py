#!/usr/bin/env python3
"""Reproduce the two opt-in playable Johto demonstration layouts.

The courtyard is a new arrangement of unchanged source blocks, NOT New Bark
Town. The lab uses Elm's original block layout. No source story is implied.
Unknown collision symbols may never occur in a registered map or its border.
Unused attributes are MB_INVALID, not guessed floor behaviors.
"""
import argparse
import base64
import hashlib
import json
import re
import struct
import tempfile
from pathlib import Path

from convert_player_walking import convert as convert_npc
from convert_johto_traditional_blocks import convert
from stage_johto_tile_graphics import build_graphics, parse_grayscale2_png, priority_overlay_tile_ids
from stage_johto_time_palettes import stage, load_source_palettes
from test_johto_metatile_banks import load_binary_fixture

ROOT = Path(__file__).resolve().parents[3]
FIXTURES = ROOT / "tools/polished_port/fixtures"
PIN = "3fa43192379df5c3e7b09a08e4d5d79af4f02f42"

# Every mapped symbol is reviewed for this deliberately limited demonstration.
# Wall intent is carried by map-grid collision, separately from field behavior.
BEHAVIORS = {
    "FLOOR": ("MB_NORMAL", False),
    "WALL": ("MB_NORMAL", True),
    "VOID": ("MB_NORMAL", True),
    "BUOY": ("MB_NORMAL", True),
    "HEADBUTT_TREE": ("MB_JOHTO_HEADBUTT_TREE", True),
    "TALL_GRASS": ("MB_TALL_GRASS", False),
    "WATER": ("MB_JOHTO_WATER", False),
    "DOOR": ("MB_NON_ANIMATED_DOOR", False),
    "WARP_CARPET_DOWN": ("MB_SOUTH_ARROW_WARP", False),
    "BOOKSHELF": ("MB_BOOKSHELF", True),
    "WINDOW": ("MB_NORMAL", True),
    "LEDGE_DOWN": ("MB_JUMP_SOUTH", False),
    "LEDGE_LEFT": ("MB_JUMP_WEST", False),
    "LEDGE_RIGHT": ("MB_JUMP_EAST", False),
    "LEDGE_DOWN_LEFT": ("MB_JOHTO_LEDGE_DOWN_LEFT", False),
    "LEDGE_DOWN_RIGHT": ("MB_JOHTO_LEDGE_DOWN_RIGHT", False),
}

# 12 x 10 original 32px blocks -> 24 x 20 Emerald 16px cells.
COURTYARD = """
05 05 05 05 05 05 05 05 05 05 05 05
05 04 18 1f 19 02 02 02 02 02 04 05
05 47 1c 77 1e 02 02 02 02 02 02 05
05 02 01 01 01 01 01 01 02 02 02 05
05 02 01 01 01 01 01 01 02 02 02 05
05 03 03 01 01 01 01 01 30 31 32 05
05 03 03 01 01 01 01 01 34 35 36 05
05 02 02 01 01 01 01 01 38 b0 3a 05
05 02 02 02 02 02 02 02 02 02 02 05
05 05 05 05 05 05 05 05 05 05 05 05
"""


def source_files():
    manifest = json.loads((FIXTURES / "demo/sources.json").read_text())
    assert manifest["commit"] == PIN
    files = {}
    for entry in manifest["files"]:
        raw = base64.b64decode((FIXTURES / "demo" / entry["fixture"]).read_text().strip(), validate=True)
        assert hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest() == entry["git_blob_sha1"]
        files[entry["path"]] = raw
    return files


def collisions(text):
    rows = []
    for line in text.splitlines():
        match = re.match(r"\s*tilecoll\s+([^;]+)", line)
        if match:
            row = [x.strip() for x in match[1].split(",")]
            assert len(row) == 4
            rows.extend(row)
    return rows


def behavior_ids():
    text = (ROOT / "include/constants/metatile_behaviors.h").read_text()
    text = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
    body = text[text.index("{") + 1:text.index("}")]
    names = [v.strip() for v in body.split(",") if v.strip()]
    assert all(re.fullmatch(r"[A-Z0-9_]+", n) for n in names)
    return dict(zip(names, range(len(names))))


def gba_tile(rows, transparent=False):
    remap = (3, 2, 1, 0 if transparent else 4)
    return bytes(remap[row[x]] | remap[row[x + 1]] << 4 for row in rows for x in range(0, 8, 2))


def lab_geometry(files, tileset="lab"):
    """Convert a source indoor tileset with no foreground-priority tiles."""
    png = files[f"gfx/tilesets/{tileset}.png"]
    width, height = struct.unpack_from(">II", png, 16)
    pixels = parse_grayscale2_png(png, expected_width=width, expected_height=height)
    graphics = bytearray(32)  # transparent overlay tile
    for y in range(0, height, 8):
        for x in range(0, width, 8):
            graphics.extend(gba_tile([row[x:x + 8] for row in pixels[y:y + 8]]))
    tiles = files[f"data/tilesets/{tileset}_metatiles.bin"]
    attrs = files[f"data/tilesets/{tileset}_attributes.bin"]
    assert len(tiles) == len(attrs) and len(tiles) % 16 == 0
    assert not any(a & 0x80 for a in attrs), "Review new foreground priority before importing"
    result = bytearray()
    for b in range(len(tiles) // 16):
        for q in range(4):
            words = []
            for n in range(4):
                index = b * 16 + (q // 2 * 2 + n // 2) * 4 + q % 2 * 2 + n % 2
                a = attrs[index]
                tile = 1 + tiles[index] + (128 if a & 8 else 0)
                assert tile * 32 < len(graphics)
                assert a & 7 < 7
                words.append(tile | (0x400 if a & 0x20 else 0) | (0x800 if a & 0x40 else 0) | (a & 7) << 12)
            result.extend(struct.pack("<8H", *words, 0, 0, 0, 0))
    return bytes(graphics).ljust(16384, b"\0"), bytes(result)


def palette_rows(text):
    rows = []
    for line in text.split("else")[0].splitlines():
        match = re.match(r"\s*RGB\s+([^;]+)", line)
        if match:
            channels = [int(x.strip()) for x in match[1].split(",")]
            assert len(channels) == 12 and all(0 <= c < 32 for c in channels)
            rows.append([channels[i] | channels[i + 1] << 5 | channels[i + 2] << 10 for i in range(0, 12, 3)])
    return rows


def apply_group_roof(graphics, files, name, group):
    # LoadMapGroupRoof overwrites nine bank-0 tiles at $0a. Common PNG roof
    # slots are intentionally blank until this source overlay has been loaded.
    assert "vTiles2 tile $0a" in files["engine/tilesets/mapgroup_roofs.asm"].decode()
    assert re.search(rf"db ROOF_{name.upper()}\s*; {group}\b", files["data/maps/roofs.asm"].decode())
    png = files[f"gfx/tilesets/roofs/{name}.png"]
    width, height = struct.unpack_from(">II", png, 16)
    pixels = parse_grayscale2_png(png, expected_width=width, expected_height=height)
    roof = b"".join(gba_tile([row[x:x + 8] for row in pixels[y:y + 8]])
                    for y in range(0, height, 8) for x in range(0, width, 8))
    assert len(roof) == 9 * 32
    result = bytearray(graphics)
    result[11 * 32:20 * 32] = roof  # GBA tile 0 is the transparent sentinel.
    # Foreground masks must use the replaced roof, not the blank placeholders.
    masks = priority_overlay_tile_ids(load_binary_fixture("metatiles"), load_binary_fixture("attributes"))
    for source, target in masks.items():
        raw = result[source * 32:(source + 1) * 32]
        result[target * 32:(target + 1) * 32] = bytes(
            (0 if v & 15 == 4 else v & 15) | (0 if v >> 4 == 4 else v >> 4) << 4 for v in raw)
    return bytes(result)


def map_cells(blocks, width, symbols, behaviors=BEHAVIORS):
    assert len(blocks) % width == 0
    cells = []
    used = set()
    for y in range(len(blocks) // width * 2):
        for x in range(width * 2):
            metatile = blocks[y // 2 * width + x // 2] * 4 + y % 2 * 2 + x % 2
            symbol = symbols[metatile]
            assert symbol in behaviors, f"Unreviewed collision {symbol} at ({x}, {y})"
            used.add(metatile)
            # Elevation 3 uses native ground-level OBJ priority; BG1 carries only
            # the source foreground masks.
            # Native Surf needs an elevation mismatch: land 3, water 1.
            # Equal-elevation water suppresses the Surf prompt and is unsafe.
            elevation = 1 if symbol == "WATER" else 3
            cells.append(metatile | (0xC00 if behaviors[symbol][1] else 0) | elevation << 12)
    return struct.pack(f"<{len(cells)}H", *cells), used


def generate():
    files = source_files()
    ids = behavior_ids()
    products = {}
    meta = convert(load_binary_fixture("metatiles"), load_binary_fixture("attributes"))
    outdoor = apply_group_roof(build_graphics()[0], files, "new_bark", 24)
    lab, lab_meta = lab_geometry(files)
    outdoor_symbols = collisions((FIXTURES / "johto_traditional_collision.asm").read_text())
    lab_symbols = collisions(files["data/tilesets/lab_collision.asm"].decode())
    assert len(outdoor_symbols) == 1012 and len(lab_symbols) == 212
    day = stage(load_source_palettes())[0]["standard.day"].ljust(512, b"\0")
    indoor = palette_rows((FIXTURES / "johto_bg_tiles.pal").read_text())[32:39]
    assert len(indoor) == 7
    indoor_pal = b"".join(struct.pack("<16H", *p, p[0], *([0] * 11)) for p in indoor).ljust(512, b"\0")
    report = {"source_commit": PIN, "maps": {}, "scope": "playable visual demonstration; no source story or live Headbutt"}
    for name, graphics, geometry, symbols, palette, blocks, width, border in (
        ("Outdoor", outdoor, meta, outdoor_symbols, day, bytes.fromhex(COURTYARD), 12, 5),
        ("Lab", lab, lab_meta, lab_symbols, indoor_pal, files["maps/ElmsLab.ablk"], 5, 0),
    ):
        map_raw, used = map_cells(blocks, width, symbols)
        border_raw, border_used = map_cells(bytes([border]), 1, symbols)
        used |= border_used
        # Attribute words only authorize cells used by this isolated pair of maps.
        attributes = [0xFF] * 1024
        for metatile in used:
            attributes[metatile] = ids[BEHAVIORS[symbols[metatile]][0]]
        geometry = geometry.ljust(1024 * 16, b"\0")
        for bank, start in (("primary", 0), ("secondary", 512)):
            base = f"data/tilesets/{bank}/polished_johto_demo_{name.lower()}"
            products[f"{base}/tiles.bin"] = graphics if start == 0 else bytes(16384)
            products[f"{base}/metatiles.bin"] = geometry[start * 16:(start + 512) * 16]
            products[f"{base}/metatile_attributes.bin"] = struct.pack("<512H", *attributes[start:start + 512])
            products[f"{base}/palettes.bin"] = palette
        products[f"data/layouts/PolishedJohtoDemo{name}/map.bin"] = map_raw
        products[f"data/layouts/PolishedJohtoDemo{name}/border.bin"] = border_raw
        report["maps"][name] = {"width": width * 2, "height": len(blocks) // width * 2,
            "used_metatiles": sorted(used), "source_collision_symbols": sorted({symbols[m] for m in used})}
    npc_rows = palette_rows(files["gfx/overworld/npc_sprites.pal"].decode())
    assert len(npc_rows) == 80  # 20 source OBJ palettes per period
    npc_pal = npc_rows[20]  # daytime red; fixed indoor baseline
    products["graphics/object_events/polished_pokefan/palette.bin"] = struct.pack("<16H", *npc_pal, *([0] * 12))
    # Chris and Pokefan F both use PAL_OW_RED. Preserve source period order,
    # which differs from Expansion: morning, day, night, evening.
    products["graphics/object_events/polished_pokefan/time_palettes.bin"] = b"".join(
        struct.pack("<16H", *npc_rows[period * 20], *([0] * 12)) for period in range(4))
    with tempfile.TemporaryDirectory() as tmp:
        inp, out = Path(tmp) / "pokefan.png", Path(tmp) / "walking.png"
        inp.write_bytes(files["gfx/sprites/pokefan_f.png"])
        convert_npc(inp, out)
        products["graphics/object_events/polished_pokefan/walking.png"] = out.read_bytes()
    report["output_sha256"] = {p: hashlib.sha256(raw).hexdigest() for p, raw in products.items()}
    products["tools/polished_port/manifests/johto_demo.json"] = (json.dumps(report, indent=2) + "\n").encode()
    return products


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    products = generate()
    for relative, raw in products.items():
        path = ROOT / relative
        if args.check:
            assert path.read_bytes() == raw, f"Regenerate {relative}"
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(raw)
    print(f"PASS: {len(products)} deterministic Johto demo assets; every used collision reviewed")


if __name__ == "__main__":
    main()
