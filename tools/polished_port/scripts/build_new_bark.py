#!/usr/bin/env python3
"""Reproduce New Bark Town's source geography and all seven connected interiors.

This imports map blocks, door/stair coordinates and static reading interactions.
Story scenes, conditional NPCs, item events and room decorations are separate
work. Route 29/27 connections are deliberately absent until those maps exist.
"""
import argparse
import base64
import hashlib
import json
import re
import struct

from build_johto_demo import (ROOT, FIXTURES, PIN, BEHAVIORS, source_files as demo_sources,
    behavior_ids, collisions, map_cells, lab_geometry, palette_rows, apply_new_bark_roof)
from convert_johto_traditional_blocks import convert
from stage_johto_tile_graphics import build_graphics
from stage_johto_time_palettes import stage, load_source_palettes
from test_johto_metatile_banks import load_binary_fixture

MAPS = {
    "NewBarkTown": "NEW_BARK_TOWN",
    "ElmsLab": "ELMS_LAB",
    "PlayersHouse1F": "PLAYERS_HOUSE_1F",
    "PlayersHouse2F": "PLAYERS_HOUSE_2F",
    "PlayersNeighborsHouse": "PLAYERS_NEIGHBORS_HOUSE",
    "LyrasHouse1F": "LYRAS_HOUSE_1F",
    "LyrasHouse2F": "LYRAS_HOUSE_2F",
    "ElmsHouse": "ELMS_HOUSE",
}
BEHAVIORS = {**BEHAVIORS,
    "WINDOW": ("MB_NORMAL", True),
    "COUNTER": ("MB_COUNTER", True),
    "FRIDGE": ("MB_NORMAL", True),
    "SINK": ("MB_NORMAL", True),
    "STOVE": ("MB_NORMAL", True),
    "TV": ("MB_NORMAL", True),
    "PC": ("MB_NORMAL", True),
    "RADIO": ("MB_NORMAL", True),
    "TOWN_MAP": ("MB_NORMAL", True),
    "STAIRCASE": ("MB_LADDER", False),
    "WARP_CARPET_LEFT": ("MB_WEST_ARROW_WARP", False),
    "WARP_CARPET_RIGHT": ("MB_EAST_ARROW_WARP", False),
}
# Scripts with an unconditional inline text body, rather than a story branch.
INLINE_TEXT = {"PlayersHouse1FTVScript", "LyrasFridgeScript", "LyrasTVScript"}


def source_files():
    directory = FIXTURES / "new_bark"
    manifest = json.loads((directory / "sources.json").read_text())
    assert manifest["commit"] == PIN
    files = demo_sources()
    for entry in manifest["files"]:
        raw = base64.b64decode((directory / entry["fixture"]).read_text().strip(), validate=True)
        assert hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest() == entry["git_blob_sha1"]
        files[entry["path"]] = raw
    return files


def json_bytes(value):
    return (json.dumps(value, indent=2) + "\n").encode()


def source_text(script, label):
    match = re.search(r"^" + re.escape(label) + r":\n(.*?)(?=^\S|\Z)", script, re.M | re.S)
    assert match, label
    body = match[1]
    assert re.search(r"^\s*done\s*$", body, re.M), f"Not plain text: {label}"
    lines = re.findall(r'^\s*(text|line|cont|para) "(.*)"$', body, re.M)
    assert lines and lines[0][0] == "text", label
    result = ""
    for command, line in lines:
        assert not re.search(r"<(?!PLAYER>)[^>]+>", line), (label, line)
        result += {"text": "", "line": "\\n", "cont": "\\l", "para": "\\p"}[command]
        result += line.replace("<PLAYER>", "{PLAYER}").replace("#mon", "POKéMON").replace("#", "POKé")
    return result + "$"


def text_definition(label, text):
    return f'{label}:\n\t.string "{text}"\n\n'


def map_header(name, constant, width, height, script):
    prefix = "Polished" + name
    header = {
        "id": "MAP_POLISHED_" + constant, "name": prefix,
        "layout": "LAYOUT_POLISHED_" + constant,
        "music": "MUS_DUMMY", "region": "REGION_JOHTO", "region_map_section": "MAPSEC_NONE",
        "requires_flash": False, "weather": "WEATHER_NONE",
        "map_type": "MAP_TYPE_TOWN" if name == "NewBarkTown" else "MAP_TYPE_INDOOR",
        "allow_cycling": name == "NewBarkTown", "allow_escaping": False,
        "allow_running": True, "show_map_name": False, "battle_scene": "MAP_BATTLE_SCENE_NORMAL",
        "connections": None, "object_events": [], "warp_events": [], "coord_events": [], "bg_events": [],
    }
    code = f"@ Generated from pinned {name}.asm: geography and static text only.\n{prefix}_MapScripts::\n\t.byte 0\n\n"
    for x, y, destination, warp in re.findall(r"^\s*warp_event\s+(\d+),\s*(\d+),\s*(\w+),\s*(\d+)", script, re.M):
        assert destination in MAPS.values(), (name, destination)
        assert 0 <= int(x) < width and 0 <= int(y) < height
        header["warp_events"].append({"x": int(x), "y": int(y), "elevation": 0,
            "dest_map": "MAP_POLISHED_" + destination, "dest_warp_id": str(int(warp) - 1)})
    imported = set()
    for x, y, kind, label in re.findall(r"^\s*bg_event\s+(\d+),\s*(\d+),\s*(BGEVENT_\w+),\s*(\w+)\s*$", script, re.M):
        if kind != "BGEVENT_JUMPTEXT" and label not in INLINE_TEXT:
            continue
        event = prefix + "_" + label
        header["bg_events"].append({"type": "sign", "x": int(x), "y": int(y), "elevation": 0,
            "player_facing_dir": "BG_EVENT_PLAYER_FACING_NORTH" if kind == "BGEVENT_UP" else "BG_EVENT_PLAYER_FACING_ANY",
            "script": event})
        if label not in imported:
            code += f"{event}::\n\tmsgbox {event}_Text, MSGBOX_SIGN\n\tend\n\n"
            code += text_definition(event + "_Text", source_text(script, label))
            imported.add(label)
    if name == "NewBarkTown":
        # Retain the source teacher's position and artwork. The return menu is
        # explicitly demo navigation, independent of any Crystal story flags.
        header["object_events"].append({"local_id": "LOCALID_NEW_BARK_TEACHER",
            "graphics_id": "OBJ_EVENT_GFX_POLISHED_POKEFAN", "x": 6, "y": 8, "elevation": 3,
            "movement_type": "MOVEMENT_TYPE_FACE_DOWN", "movement_range_x": 0, "movement_range_y": 0,
            "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0",
            "script": prefix + "_Teacher", "flag": "0"})
        code += f"""{prefix}_Teacher::
\tlock
\tfaceplayer
\tmsgbox {prefix}_Breeze, MSGBOX_DEFAULT
\tmsgbox {prefix}_Garden, MSGBOX_YESNO
\tgoto_if_eq VAR_RESULT, NO, {prefix}_Stay
\tclosemessage
\twarp MAP_POLISHED_JOHTO_DEMO_OUTDOOR, 6, 8
\twaitstate
{prefix}_Stay::
\trelease
\tend

"""
        code += text_definition(prefix + "_Breeze", source_text(script, "Text_RefreshingBreeze"))
        code += text_definition(prefix + "_Garden", "Return to the demonstration\\ngarden?$")
    if name == "ElmsLab":
        header["bg_events"].append({"type": "sign", "x": 2, "y": 1, "elevation": 0,
            "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY", "script": prefix + "_Heal"})
        code += f"""{prefix}_Heal::
\tlockall
\tmsgbox {prefix}_HealText, MSGBOX_YESNO
\tgoto_if_eq VAR_RESULT, NO, {prefix}_HealDone
\tspecial HealPlayerParty
{prefix}_HealDone::
\treleaseall
\tend

"""
        code += text_definition(prefix + "_HealText", source_text(script, "ElmsLabHealingMachineText2"))
    return header, (code.rstrip() + "\n").encode()


def generate():
    files = source_files()
    ids = behavior_ids()
    products, maps, layouts = {}, {}, []
    indoor = palette_rows((FIXTURES / "johto_bg_tiles.pal").read_text())[32:39]
    indoor_palette = b"".join(struct.pack("<16H", *p, p[0], *([0] * 11)) for p in indoor).ljust(512, b"\0")
    outdoor = apply_new_bark_roof(build_graphics()[0], files)
    geometry = convert(load_binary_fixture("metatiles"), load_binary_fixture("attributes"))
    house_graphics, house_geometry = lab_geometry(files, "johto_house")
    banks = {
        "Outdoor": (outdoor, geometry, stage(load_source_palettes())[0]["standard.day"].ljust(512, b"\0"),
                    collisions((FIXTURES / "johto_traditional_collision.asm").read_text())),
        "House": (house_graphics, house_geometry, indoor_palette,
                  collisions(files["data/tilesets/johto_house_collision.asm"].decode())),
        "Lab": (*lab_geometry(files), indoor_palette, collisions(files["data/tilesets/lab_collision.asm"].decode())),
    }
    used_by_bank = {key: set() for key in banks}
    # Invert without collapsing the many source symbols sharing a permission.
    permissions = {symbol: permission for permission, symbol in re.findall(
        r"db (LAND_TILE|WALL_TILE|WATER_TILE)\s*; COLL_(\w+)", files["data/collision/collision_permissions.asm"].decode())}
    for name, constant in MAPS.items():
        bank = "Outdoor" if name == "NewBarkTown" else "Lab" if name == "ElmsLab" else "House"
        source_tileset = {"Outdoor": "JOHTO_TRADITIONAL", "Lab": "LAB", "House": "JOHTO_HOUSE"}[bank]
        assert re.search(r"map " + name + r", TILESET_" + source_tileset + r",", files["data/maps/maps.asm"].decode())
        source_layout = "JohtoHouse4" if name == "PlayersNeighborsHouse" else name
        if name == "PlayersNeighborsHouse":
            assert re.search(r'PlayersNeighborsHouse_BlockData:(?:\s*\w+_BlockData:)*\s*INCBIN "maps/JohtoHouse4.ablk.lz"', files["data/maps/blocks.asm"].decode())
        width, height = map(int, re.search(r"map_const " + constant + r",\s*(\d+),\s*(\d+)", files["constants/map_constants.asm"].decode()).groups())
        blocks = files[f"maps/{source_layout}.ablk"]
        assert len(blocks) == width * height
        symbols = banks[bank][3]
        raw, used = map_cells(blocks, width, symbols, BEHAVIORS)
        border = 5 if bank == "Outdoor" else 0
        assert re.search(r"map_attributes " + name + ", " + constant + r", \$" + str(border) + ",", files["data/maps/attributes.asm"].decode())
        border_raw, border_used = map_cells(bytes([border]), 1, symbols, BEHAVIORS)
        for metatile in used | border_used:
            symbol = symbols[metatile]
            assert permissions[symbol] == ("WATER_TILE" if symbol == "WATER" else "WALL_TILE" if BEHAVIORS[symbol][1] else "LAND_TILE"), symbol
        used_by_bank[bank] |= used | border_used
        prefix = "Polished" + name
        products[f"data/layouts/{prefix}/map.bin"] = raw
        products[f"data/layouts/{prefix}/border.bin"] = border_raw
        header, script = map_header(name, constant, width * 2, height * 2, files[f"maps/{name}.asm"].decode())
        products[f"data/maps/{prefix}/map.json"] = json_bytes(header)
        products[f"data/maps/{prefix}/scripts.inc"] = script
        tileset = "PolishedNewBark" + bank
        layouts.append({"id": header["layout"], "name": prefix + "_Layout", "width": width * 2, "height": height * 2,
            "primary_tileset": f"gTileset_{tileset}Primary", "secondary_tileset": f"gTileset_{tileset}Secondary",
            "border_filepath": f"data/layouts/{prefix}/border.bin", "blockdata_filepath": f"data/layouts/{prefix}/map.bin",
            "layout_version": "emerald"})
        maps[constant] = {"name": name, "width": width * 2, "height": height * 2, "tileset": bank,
            "source_blocks": source_layout, "used_metatiles": sorted(used | border_used),
            "warps": header["warp_events"], "static_interactions": len(header["bg_events"])}
    # Validate all source warp indexes against their actual destination, never
    # merely against an unrelated Emerald map with a matching numeric ID.
    for constant, info in maps.items():
        for warp in info["warps"]:
            destination = maps[warp["dest_map"].removeprefix("MAP_POLISHED_")]
            target = destination["warps"][int(warp["dest_warp_id"])]
            assert target["dest_map"] == "MAP_POLISHED_" + constant
    header_text = "// Generated by build_new_bark.py from the pinned source.\n\n"
    for bank, (graphics, geometry, palette, symbols) in banks.items():
        attrs = [0xFF] * 1024
        for metatile in used_by_bank[bank]:
            attrs[metatile] = ids[BEHAVIORS[symbols[metatile]][0]]
        geometry = geometry.ljust(1024 * 16, b"\0")
        for secondary, start in ((False, 0), (True, 512)):
            side = "Secondary" if secondary else "Primary"
            stem = f"PolishedNewBark{bank}{side}"
            base = f"data/tilesets/{side.lower()}/polished_new_bark_{bank.lower()}"
            entries = {"Tiles": ("u32", "tiles.bin", graphics if not secondary else bytes(16384)),
                       "Palettes": ("u16", "palettes.bin", palette),
                       "Metatiles": ("u16", "metatiles.bin", geometry[start * 16:(start + 512) * 16]),
                       "Attributes": ("u16", "metatile_attributes.bin", struct.pack("<512H", *attrs[start:start + 512]))}
            for suffix, (ctype, filename, raw) in entries.items():
                products[f"{base}/{filename}"] = raw
                header_text += f'static const {ctype} s{stem}{suffix}[] = INCBIN_{ctype.upper()}("{base}/{filename}");\n'
            callback = "InitTilesetAnim_PolishedJohtoOutdoorPrimary" if bank == "Outdoor" and not secondary else "NULL"
            header_text += f"""const struct Tileset gTileset_{stem} =
{{
    .isCompressed = FALSE,
    .isSecondary = {str(secondary).upper()},
    .tiles = s{stem}Tiles,
    .palettes = (const u16 (*)[16])s{stem}Palettes,
    .metatiles = s{stem}Metatiles,
    .metatileAttributes = s{stem}Attributes,
    .callback = {callback},
}};

"""
    products["src/data/tilesets/polished_new_bark.h"] = (header_text.rstrip() + "\n").encode()
    report = {"source_commit": PIN, "scope": "source geography, connected interiors and static interactions; story/NPC population deferred",
        "maps": maps, "layouts": layouts, "output_sha256": {p: hashlib.sha256(raw).hexdigest() for p, raw in products.items()}}
    products["tools/polished_port/manifests/new_bark.json"] = json_bytes(report)
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
            if not path.exists() or path.read_bytes() != raw:
                path.write_bytes(raw)
    report = json.loads(products["tools/polished_port/manifests/new_bark.json"])
    warps = sum(len(info["warps"]) for info in report["maps"].values())
    print(f"PASS: {len(products)} deterministic New Bark assets; 8 maps, {warps} reciprocal warp events")


if __name__ == "__main__":
    main()
