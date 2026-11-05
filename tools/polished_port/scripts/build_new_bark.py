#!/usr/bin/env python3
"""Reproduce the first journey from New Bark to Mr. Pokémon.

This imports map blocks, door/stair coordinates, reading interactions and the
six ambient residents, the rival's lab-window encounter, and Lyra/Elm's
introduction, starter selection, Lyra battle, first Potion and Mom's opening.
Cherrygrove includes the guide tour, Town Map, residents and native services.
Route 30 includes Mulch, the Mystery Egg and Oak. The return errand includes
Elm's call, the rival, the theft investigation, Egg delivery and Joey's battle.
Mom's savings, Lyra's catching tutorial and Tuesday's Tuscany follow Egg delivery.
"""
import argparse
import base64
import hashlib
import json
import re
import struct
import tempfile
from pathlib import Path

from build_johto_demo import (ROOT, FIXTURES, PIN, BEHAVIORS, source_files as demo_sources,
    behavior_ids, collisions, map_cells, lab_geometry, palette_rows, apply_new_bark_roof)
from convert_johto_traditional_blocks import convert
from stage_johto_tile_graphics import build_graphics
from stage_johto_time_palettes import stage, load_source_palettes
from test_johto_metatile_banks import load_binary_fixture
from convert_player_walking import (convert as convert_npc, _read_png_2bit_grayscale,
    _write_indexed_4bit_png)
import new_bark_intro
import new_bark_battle
import new_bark_mom
import polished_route29
import polished_cherrygrove
import polished_route30
import polished_route31
import polished_return
import polished_departure

NEW_BARK_MAPS = {
    "NewBarkTown": "NEW_BARK_TOWN",
    "ElmsLab": "ELMS_LAB",
    "PlayersHouse1F": "PLAYERS_HOUSE_1F",
    "PlayersHouse2F": "PLAYERS_HOUSE_2F",
    "PlayersNeighborsHouse": "PLAYERS_NEIGHBORS_HOUSE",
    "LyrasHouse1F": "LYRAS_HOUSE_1F",
    "LyrasHouse2F": "LYRAS_HOUSE_2F",
    "ElmsHouse": "ELMS_HOUSE",
}
MAPS = {**NEW_BARK_MAPS, "Route29": "ROUTE_29", "Route2946Gate": "ROUTE_29_46_GATE", **polished_cherrygrove.MAPS, **polished_route30.MAPS, **polished_route31.MAPS}
BEHAVIORS = {**BEHAVIORS,
    "BOTTOM_WALL": ("MB_IMPASSABLE_SOUTH", False),
    "CAVE": ("MB_NON_ANIMATED_DOOR", False),
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
    "MART_SHELF": ("MB_NORMAL", True),
    "LADDER": ("MB_LADDER", False),
    "MART_SIGN": ("MB_NORMAL", True),
    "POKECENTER_SIGN": ("MB_NORMAL", True),
    "TRASH_CAN": ("MB_NORMAL", True),
}
# Scripts with an unconditional inline text body, rather than a story branch.
INLINE_TEXT = {"PlayersHouse1FTVScript", "LyrasFridgeScript", "LyrasTVScript"}

# Only source jumptextfaceplayer residents with no story flag are imported.
# The teacher retains the explicitly separate demo travel prompt.
RESIDENTS = {
    "Route31": {"Route31YoungsterText": "ROUTE31_SCHOOLBOY"},
    "Route31VioletGate": {"Route31VioletGateOfficerText": "ROUTE31_GATE_OFFICER", "Route31VioletGateCooltrainerFText":"ROUTE31_GATE_BATTLE_GIRL"},
    "Route29": {"Route29YoungsterText": "ROUTE29_YOUNGSTER", "Route29TeacherText": "ROUTE29_TEACHER", "Route29FisherText": "ROUTE29_FISHER"},
    "Route2946Gate": {"Route29Route46GateOfficerText": "ROUTE29_GATE_OFFICER", "Route29Route46GateYoungsterText": "ROUTE29_GATE_CAMPER"},
    "NewBarkTown": {"Text_ElmDiscoveredNewMon": "NEW_BARK_FAT_GUY",
                    "Text_GearIsImpressive": "NEW_BARK_SCHOOLBOY"},
    "PlayersNeighborsHouse": {"PlayersNeighborsDaughterText": "NEIGHBORS_DAUGHTER",
                              "PlayersNeighborsHusbandText": "NEIGHBORS_HUSBAND"},
    "ElmsHouse": {"ElmsWifeText": "ELMS_WIFE", "ElmsSonText": "ELMS_SON"},
}
NPC_GRAPHICS = {
    ("SPRITE_SCHOOLBOY", "GREEN"): "POLISHED_SCHOOLBOY_GREEN",
    ("SPRITE_FAT_GUY", "BLUE"): "POLISHED_FAT_GUY_BLUE",
    ("SPRITE_TEACHER", "GREEN"): "POLISHED_TEACHER",
    ("SPRITE_OFFICER", "RED"): "POLISHED_OFFICER",
    ("SPRITE_CAMPER", "GREEN"): "POLISHED_CAMPER",
    ("SPRITE_FAT_GUY", "GREEN"): "POLISHED_FAT_GUY",
    ("SPRITE_SCHOOLBOY", "BLUE"): "POLISHED_SCHOOLBOY",
    ("SPRITE_BATTLE_GIRL", "BLUE"): "POLISHED_BATTLE_GIRL_BLUE",
    ("SPRITE_BATTLE_GIRL", "RED"): "POLISHED_BATTLE_GIRL",
    ("SPRITE_POKEFAN_F", "GREEN"): "POLISHED_POKEFAN_GREEN",
    ("SPRITE_CHILD", "BROWN"): "POLISHED_CHILD",
}
NPC_MOVEMENTS = {
    "SPRITEMOVEDATA_WANDER": "MOVEMENT_TYPE_WANDER_AROUND",
    "SPRITEMOVEDATA_STANDING_DOWN": "MOVEMENT_TYPE_FACE_DOWN",
    "SPRITEMOVEDATA_SPINRANDOM_SLOW": "MOVEMENT_TYPE_LOOK_AROUND",
    "SPRITEMOVEDATA_WALK_UP_DOWN": "MOVEMENT_TYPE_WANDER_UP_AND_DOWN",
    "SPRITEMOVEDATA_WALK_LEFT_RIGHT": "MOVEMENT_TYPE_WANDER_LEFT_AND_RIGHT",
    "SPRITEMOVEDATA_STANDING_UP": "MOVEMENT_TYPE_FACE_UP",
    "SPRITEMOVEDATA_STANDING_LEFT": "MOVEMENT_TYPE_FACE_LEFT",
    "SPRITEMOVEDATA_STANDING_RIGHT": "MOVEMENT_TYPE_FACE_RIGHT",
}


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
    match = re.search(r"^" + re.escape(label) + r":{1,2}\n(.*?)(?=^[.\w]+:{1,2}|\Z)", script, re.M | re.S)
    assert match, label
    # Resolve release conditionals before parsing text commands. In particular,
    # Mom's DEBUG branch contains its own done; it must not truncate release text.
    body = []
    stack = []
    for raw in match[1].splitlines():
        command = raw.strip()
        if command in ("if DEF(DEBUG)", "if !DEF(DEBUG)"):
            stack.append(command == "if !DEF(DEBUG)")
        elif command == "else":
            assert stack, label
            stack[-1] = not stack[-1]
        elif command == "endc":
            assert stack, label
            stack.pop()
        elif all(stack):
            body.append(raw)
    assert not stack, label
    body = "\n".join(body)
    assert re.search(r"^\s*done\s*$", body, re.M), f"Not plain text: {label}"
    # Fail on unsupported commands instead of silently dropping dialogue.
    lines = []
    for raw in body.splitlines():
        raw = raw.strip()
        if not raw or raw.startswith(";"):
            continue
        if raw == "jumpthistext" and label in INLINE_TEXT and not lines:
            continue
        if raw == "done":
            break
        elif raw == "text_ram wStringBuffer3":
            lines.append(("text", "{STR_VAR_3}"))
        else:
            part = re.fullmatch(r'(text|line|cont|para) "(.*)"', raw)
            assert part, (label, raw)
            lines.append(part.groups())
    assert lines and lines[0][0] == "text", label
    result = ""
    for command, line in lines:
        assert not re.search(r"<(?!PLAYER>)[^>]+>", line), (label, line)
        result += {"text": "", "line": "\\n", "cont": "\\l", "para": "\\p"}[command]
        result += line.replace("<PLAYER>", "{PLAYER}").replace("#mon", "POKéMON").replace("#", "POKé")
    return result + "$"


def text_definition(label, text):
    return f'{label}:\n\t.string "{text}"\n\n'


def residents(name, script, files):
    objects, code, report = [], "", []
    selected = RESIDENTS.get(name, {})
    for line in re.findall(r"^\s*object_event (.*)$", script, re.M):
        fields = [field.strip() for field in line.split(",")]
        if len(fields) != 12 or fields[10] not in selected:
            continue
        x, y, sprite, movement, ry, rx, time, palette, kind, command, label, flag = fields
        assert (kind, command, flag) == ("OBJECTTYPE_COMMAND", "jumptextfaceplayer", "-1")
        if palette in ("0", "-1"):
            # A source default is sprite-specific; it is not palette zero.
            sprite_name = {"SPRITE_SCHOOLBOY": "Schoolboy", "SPRITE_TEACHER": "Teacher", "SPRITE_CAMPER": "Camper"}[sprite]
            palette = "PAL_NPC_" + re.search(sprite_name + r"SpriteGFX, WALKING_SPRITE, PAL_OW_(\w+)",
                files["data/sprites/sprites.asm"].decode())[1]
        color = palette.removeprefix("PAL_NPC_")
        event = "Polished" + name + "_" + label
        scheduled = time != "-1"
        objects.append({"local_id": "LOCALID_" + selected[label],
            "graphics_id": "OBJ_EVENT_GFX_" + NPC_GRAPHICS[sprite, color],
            "x": int(x), "y": int(y), "elevation": 3,
            "movement_type": NPC_MOVEMENTS[movement],
            "movement_range_x": int(rx), "movement_range_y": int(ry),
            "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0",
            "script": event, "flag": "FLAG_TEMP_1" if scheduled else "0"})
        code += f"{event}::\n\tmsgbox {event}_Text, MSGBOX_NPC\n\tend\n\n"
        code += text_definition(event + "_Text", source_text(script, label))
        report.append({"local_id": objects[-1]["local_id"], "source_sprite": sprite,
            "source_palette": color, "source_text": label, "source_movement": movement,
            "source_time": time, "event": objects[-1]})
    assert len(objects) == len(selected), name
    return objects, code, report


def schedule_script(name, resident_info, files):
    scheduled = [npc for npc in resident_info if npc["source_time"] != "-1"]
    if not scheduled:
        return "", ""
    assert len(scheduled) == 1  # FLAG_TEMP_1 is local to each of these two maps.
    time = scheduled[0]["source_time"]
    assert time in ("(1 << MORN) | (1 << DAY)", "(1 << EVE) | (1 << NITE)")
    daytime = time == "(1 << MORN) | (1 << DAY)"
    constants = files["constants/misc_constants.asm"].decode()
    morning = int(re.search(r"DEF MORN_HOUR EQU\s+(\d+)", constants)[1])
    evening = int(re.search(r"DEF EVE_HOUR\s+EQU\s+(\d+)", constants)[1])
    prefix = "Polished" + name + "_ResidentSchedule"
    # Source LoadMapObjects -> LoadObjectMasks -> CheckObjectTime sets the
    # visibility mask when loading the map, not during an NPC conversation.
    # Read actual RTC hours rather than Expansion's configurable TOD cutoffs.
    code = f"""@ Map-local visibility, recomputed on entry and save loading.
{prefix}::
\tgettime
\tgoto_if_lt VAR_0x8000, {morning}, {prefix}_Evening
\tgoto_if_ge VAR_0x8000, {evening}, {prefix}_Evening
\t{'clearflag' if daytime else 'setflag'} FLAG_TEMP_1
\tend
{prefix}_Evening::
\t{'setflag' if daytime else 'clearflag'} FLAG_TEMP_1
\tend

@ Continue restores saved actors; reconcile only this scheduled resident with
@ the freshly computed mask. Ordinary menu returns retain the existing mask.
{prefix}_Restore::
\tgoto_if_set FLAG_TEMP_1, {prefix}_Hide
\taddobject {scheduled[0]['local_id']}
\tend
{prefix}_Hide::
\tremoveobject {scheduled[0]['local_id']}
\tend

"""
    return (f"\tmap_script MAP_SCRIPT_ON_LOAD, {prefix}\n"
            f"\tmap_script MAP_SCRIPT_ON_RETURN_TO_FIELD, {prefix}_Restore\n"), code


def resident_assets(files):
    products = {}
    colors = re.findall(r"^\s*ow_npc_pal_const (\w+)", files["constants/sprite_data_constants.asm"].decode(), re.M)
    rows = palette_rows(files["gfx/overworld/npc_sprites.pal"].decode())
    assert colors[:4] == ["RED", "BLUE", "GREEN", "BROWN"] and len(rows) == 80
    for color in ("BLUE", "GREEN", "BROWN", "POKE_BALL", "DECO_ITEM", "KEY_ITEM"):
        index = colors.index(color)
        base = "graphics/object_events/polished_npc_palettes/" + color.lower()
        products[base + ".bin"] = struct.pack("<16H", *rows[20 + index], *([0] * 12))
        if color in ("BLUE", "GREEN", "BROWN"):
            products[base + "_time.bin"] = b"".join(struct.pack("<16H", *rows[period * 20 + index],
                *([0] * 12)) for period in range(4))
    with tempfile.TemporaryDirectory() as tmp:
        inp, out = Path(tmp) / "npc.png", Path(tmp) / "walking.png"
        for sprite in ("fat_guy", "schoolboy", "battle_girl", "child", "rival", "elm", "lyra", "scientist", "mom", "matron", "teacher", "cool_dude", "officer", "camper"):
            inp.write_bytes(files[f"gfx/sprites/{sprite}.png"])
            convert_npc(inp, out)
            products[f"graphics/object_events/polished_{sprite}/walking.png"] = out.read_bytes()
        inp.write_bytes(files["gfx/sprites/ball_cut_fruit.png"])
        w, h, pixels = _read_png_2bit_grayscale(inp)
        assert (w, h) == (16, 48)
        # Standing-down is the first 16x16 source frame; cut tree and fruit
        # frames are not part of the starter-ball object. Keep exact pixels.
        frame = [[0] * 16 for _ in range(16)] + [[3 - p for p in row] for row in pixels[:16]]
        _write_indexed_4bit_png(out, frame)
        products["graphics/object_events/polished_starter_ball/ball.png"] = out.read_bytes()
        # Native bottom anchoring: retain the exact source Cut tiles and the
        # berry overlay (tile 8), without copying the apricorn/picked frames.
        for name, body in (("cut_tree", [[3-p for p in row] for row in pixels[16:32]]),
                           ("fruit", [[0]*3 + [3-p for p in row[:8]] + [0]*5 for row in pixels[32:40]]),
                           ("fruit_picked", [[3-p for p in row[8:16]] + [0]*8 for row in pixels[40:48]])):
            frame = [[0]*16 for _ in range(32-len(body))] + body
            _write_indexed_4bit_png(out, frame)
            products[f"graphics/object_events/polished_{name}/object.png"] = out.read_bytes()
    products["graphics/object_events/polished_npc_palettes/bg_green.bin"] = stage(load_source_palettes())[0]["standard.day"][64:96]
    return products


def rival_scene(script, files):
    fields = next([part.strip() for part in line.split(",")]
        for line in re.findall(r"^\s*object_event (.*)$", script, re.M)
        if ", NewBarkTownRivalScript," in line)
    assert fields == ["3", "2", "SPRITE_RIVAL", "SPRITEMOVEDATA_STANDING_RIGHT", "0", "0", "-1", "0",
                      "OBJECTTYPE_SCRIPT", "0", "NewBarkTownRivalScript", "EVENT_RIVAL_NEW_BARK_TOWN"]
    assert "RivalSpriteGFX, WALKING_SPRITE, PAL_OW_RED" in files["data/sprites/sprites.asm"].decode()
    prefix = "PolishedNewBarkTown_Rival"
    obj = {"local_id": "LOCALID_NEW_BARK_RIVAL", "graphics_id": "OBJ_EVENT_GFX_POLISHED_RIVAL",
        "x": int(fields[0]), "y": int(fields[1]), "elevation": 3,
        "movement_type": "MOVEMENT_TYPE_FACE_RIGHT", "movement_range_x": 0, "movement_range_y": 0,
        "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0",
        "script": prefix, "flag": "FLAG_HIDE_POLISHED_NEW_BARK_RIVAL"}
    code = f"""@ The west approach (2,2) is the only walkable adjacent tile.
@ Source follow queues one left step into the player's vacated cell; express
@ those two simultaneous moves directly without replacing any global follower.
@ Crystal jump_step_down travels TWO cells (Jump and Land), hence jump_2_down.
{prefix}::
\tlockall
\tmsgbox {prefix}_Text1, MSGBOX_DEFAULT
\tclosemessage
\tapplymovement LOCALID_NEW_BARK_RIVAL, {prefix}_FaceLeft
\twaitmovement LOCALID_NEW_BARK_RIVAL
\tmsgbox {prefix}_Text2, MSGBOX_DEFAULT
\tclosemessage
\tapplymovement OBJ_EVENT_ID_PLAYER, {prefix}_BackStep
\tapplymovement LOCALID_NEW_BARK_RIVAL, {prefix}_StepLeft
\twaitmovement OBJ_EVENT_ID_PLAYER
\twaitmovement LOCALID_NEW_BARK_RIVAL
\tdelay 5
\tapplymovement LOCALID_NEW_BARK_RIVAL, {prefix}_FaceDown
\twaitmovement LOCALID_NEW_BARK_RIVAL
\tdelay 5
@ Expansion's native Tackle impact (gBattleAnimMove_Tackle).
\tplayse SE_M_COMET_PUNCH
\tapplymovement OBJ_EVENT_ID_PLAYER, {prefix}_Shove
\twaitmovement OBJ_EVENT_ID_PLAYER
\tapplymovement LOCALID_NEW_BARK_RIVAL, {prefix}_Return
\twaitmovement LOCALID_NEW_BARK_RIVAL
\treleaseall
\tend

{prefix}_FaceLeft:
\tface_left
\tstep_end
{prefix}_FaceDown:
\tface_down
\tstep_end
{prefix}_BackStep:
\tface_up
\tlock_facing_direction
\twalk_down
\tstep_end
{prefix}_StepLeft:
\twalk_left
\tstep_end
{prefix}_Shove:
\tjump_2_down
\tunlock_facing_direction
\tstep_end
{prefix}_Return:
\twalk_right
\tstep_end

"""
    for number in (1, 2):
        code += text_definition(prefix + f"_Text{number}", source_text(script, f"NewBarkTownRivalText{number}"))
    report = {"source_script": "NewBarkTownRivalScript", "source_sprite": "SPRITE_RIVAL", "source_palette": "RED",
        "source_visibility_event": fields[-1], "visibility_flag": obj["flag"],
        "visibility_setter": "PolishedMrPokemonsHouse_CheckIntro (completed first visit)", "event": obj,
        "player_path": [[2, 2], [2, 3], [2, 5]], "rival_path": [[3, 2], [2, 2], [3, 2]],
        "sound_translation": {"SFX_TACKLE": "SE_M_COMET_PUNCH"}}
    return obj, code, report


def map_header(name, constant, width, height, script, files):
    prefix = "Polished" + name
    header = {
        "id": "MAP_POLISHED_" + constant, "name": prefix,
        "layout": "LAYOUT_POLISHED_" + constant,
        "music": "MUS_DUMMY", "region": "REGION_JOHTO", "region_map_section": "MAPSEC_NONE",
        "requires_flash": False, "weather": "WEATHER_NONE",
        "map_type": "MAP_TYPE_TOWN" if name in ("NewBarkTown", "CherrygroveCity") else "MAP_TYPE_ROUTE" if name in ("Route29", "Route30", "Route31") else "MAP_TYPE_INDOOR",
        "allow_cycling": name in ("NewBarkTown", "Route29", "Route30", "Route31", "CherrygroveCity"), "allow_escaping": False,
        "allow_running": True, "show_map_name": False, "battle_scene": "MAP_BATTLE_SCENE_NORMAL",
        "connections": None, "object_events": [], "warp_events": [], "coord_events": [], "bg_events": [],
    }
    objects, resident_code, resident_info = residents(name, script, files)
    schedule_header, schedule_code = schedule_script(name, resident_info, files)
    code = f"@ Generated from pinned {name}.asm: geography, static text and ambient residents.\n{prefix}_MapScripts::\n{schedule_header}\t.byte 0\n\n" + schedule_code
    for x, y, destination, warp in re.findall(r"^\s*warp_event\s+(\d+),\s*(\d+),\s*(\w+),\s*(\d+)", script, re.M):
        if destination == "POKECENTER_2F":
            continue  # Closed in the source first-visit story state.
        if name == "Route2946Gate" and destination == "ROUTE_46":
            continue  # North door is blocked until Route 46 is imported.
        if (int(x),int(y)) in polished_route31.CLOSED.get(name, ()):
            continue
        if destination == "ROUTE_31_VIOLET_GATE":
            warp = str(int(warp) - 2)  # Omit the two western exits to Violet.
        if destination == "ROUTE_29_46_GATE":
            warp = str(int(warp) - 2)  # Source warp 3 -> native warp 0.
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
            "movement_type": "MOVEMENT_TYPE_LOOK_AROUND", "movement_range_x": 0, "movement_range_y": 0,
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
\tgoto_if_eq VAR_POLISHED_STARTER, 0, {prefix}_CannotHeal
\tmsgbox {prefix}_HealText, MSGBOX_YESNO
\tgoto_if_eq VAR_RESULT, NO, {prefix}_HealDone
\tspecial HealPlayerParty
{prefix}_HealDone::
\treleaseall
\tend

{prefix}_CannotHeal::
\tmsgbox {prefix}_CannotHealText, MSGBOX_DEFAULT
\treleaseall
\tend

"""
        code += text_definition(prefix + "_HealText", source_text(script, "ElmsLabHealingMachineText2"))
        code += text_definition(prefix + "_CannotHealText", source_text(script, "ElmsLabHealingMachineText1"))
    header["object_events"].extend(objects)
    code += resident_code
    story = []
    if name == "NewBarkTown":
        obj, scene, report = rival_scene(script, files)
        header["object_events"].append(obj)  # Preserve all existing local IDs.
        code += scene
        story.append(report)
    if name in ("NewBarkTown", "ElmsLab"):
        hooks, scene, report = (new_bark_intro.town if name == "NewBarkTown" else new_bark_intro.lab)(
            script, header, source_text, text_definition)
        code = code.replace("\t.byte 0\n", hooks + "\t.byte 0\n", 1)
        code += scene
        story.append(report)
    if name == "ElmsLab":
        scene, report = new_bark_battle.lab(script, header, source_text, text_definition)
        code += scene
        story.append(report)
    if name == "PlayersHouse1F":
        hooks, scene, report = new_bark_mom.house(script, header, files, source_text, text_definition)
        code = code.replace("\t.byte 0\n", hooks + "\t.byte 0\n", 1)
        code += scene
        story.append(report)
    if name in ("NewBarkTown", "Route29", "Route2946Gate"):
        hooks, scene, report = polished_route29.events(name, script, header, files, source_text, text_definition)
        code = code.replace("\t.byte 0\n", hooks + "\t.byte 0\n", 1)
        code += scene
        story.append(report)
        if name == "Route29":
            # The GBC title-screen shortcuts are unavailable in this engine.
            code = code.replace(source_text(script, "Route29AdvancedTipsSignText"),
                "Advanced Tips!\\pSome POKéMON appear\\nonly at certain times.\\pLook during both the\\nday and the night!$")
    if name in polished_cherrygrove.MAPS:
        hooks, scene, report = polished_cherrygrove.events(name, script, header, files, source_text, text_definition)
        code = code.replace("\t.byte 0\n", hooks + "\t.byte 0\n", 1)
        code += scene
        story.append(report)
    if name in polished_route30.MAPS:
        hooks, scene, report = polished_route30.events(name, script, header, files, source_text, text_definition)
        code = code.replace("\t.byte 0\n", hooks + "\t.byte 0\n", 1)
        code += scene
        story.append(report)
        if name == "Route30":
            code = code.replace(source_text(script, "Route30AdvancedTipsText"),
                "Advanced Tips!\\pDuring battle, choose\\nPOKéMON to switch partners.\\pChoose BAG to use an item,\\nor RUN to leave a wild battle!$")
    if name in polished_route31.MAPS:
        hooks, scene, report = polished_route31.events(name, script, header, files, source_text, text_definition)
        code = code.replace("\t.byte 0\n", hooks + "\t.byte 0\n", 1)
        code += scene
        story.append(report)
    if name == "PlayersNeighborsHouse":
        npc = polished_route29.object_at("LOCALID_RETURNED_NEIGHBOR", "POLISHED_MATRON", 5, 3,
            "PolishedRoute30_NeighborHome", "FLAG_TEMP_2")
        npc["movement_type"] = "MOVEMENT_TYPE_FACE_LEFT"
        header["object_events"].append(npc)
        code = code.replace("PolishedPlayersNeighborsHouse_ResidentSchedule::\n",
            "PolishedPlayersNeighborsHouse_ResidentSchedule::\n\tsetflag FLAG_TEMP_2\n\tcall_if_eq VAR_POLISHED_MR_POKEMON_SCENE, 1, PolishedRoute30_ShowNeighbor\n")
        code += "PolishedRoute30_ShowNeighbor::\n\tclearflag FLAG_TEMP_2\n\treturn\n\nPolishedRoute30_NeighborHome::\n\tmsgbox PolishedRoute30_NeighborHomeText, MSGBOX_NPC\n\tend\n"
        code += text_definition("PolishedRoute30_NeighborHomeText", source_text(script, "PlayersNeighborText"))
    code = polished_return.adapt(name, script, header, files, code, source_text, text_definition)
    if name in polished_return.STORY:
        story.append(polished_return.STORY[name])
    return header, (code.rstrip() + "\n").encode(), resident_info, story


def generate():
    files = source_files()
    ids = behavior_ids()
    products, maps, layouts = {**resident_assets(files), **new_bark_battle.battle_assets(files), **polished_cherrygrove.assets(files), **polished_route30.assets(files), **polished_route31.assets(files), **polished_return.assets(files), **polished_departure.assets(files)}, {}, []
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
        "Gate": (*lab_geometry(files, "gate"), indoor_palette, collisions(files["data/tilesets/gate_collision.asm"].decode())),
    }
    for key in ("Mart", "PokeCenter"):
        name = key.lower()
        rows = polished_cherrygrove.service_palettes(files, key)
        palette = b"".join(struct.pack("<16H", *p, p[0], *([0] * 11)) for p in rows).ljust(512, b"\0")
        banks[key] = (*lab_geometry(files, name), palette, collisions(files[f"data/tilesets/{name}_collision.asm"].decode()))
    used_by_bank = {key: set() for key in banks}
    # Invert without collapsing the many source symbols sharing a permission.
    permissions = {symbol: permission for permission, symbol in re.findall(
        r"db (LAND_TILE|WALL_TILE|WATER_TILE)\s*; COLL_(\w+)", files["data/collision/collision_permissions.asm"].decode())}
    for name, constant in MAPS.items():
        bank = "Outdoor" if name in ("NewBarkTown", "Route29", "Route30", "Route31", "CherrygroveCity") else "Lab" if name == "ElmsLab" else "Gate" if name in ("Route2946Gate", "Route31VioletGate") else "Mart" if name == "CherrygroveMart" else "PokeCenter" if name == "CherrygrovePokeCenter1F" else "House"
        source_tileset = {"Outdoor": "JOHTO_TRADITIONAL", "Lab": "LAB", "House": "JOHTO_HOUSE", "Gate": "GATE", "Mart": "MART", "PokeCenter": "POKECENTER"}[bank]
        assert re.search(r"map " + name + r", TILESET_" + source_tileset + r",", files["data/maps/maps.asm"].decode())
        source_layout = "JohtoHouse4" if name in ("PlayersNeighborsHouse", "Route30BerrySpeechHouse") else name
        source_layout = polished_cherrygrove.BLOCKS.get(name, source_layout)
        if name == "Route2946Gate":
            source_layout = "NorthSouthGate"
        if name == "Route31VioletGate":
            source_layout = "EastWestGate"
        if name == "PlayersNeighborsHouse":
            assert re.search(r'PlayersNeighborsHouse_BlockData:(?:\s*\w+_BlockData:)*\s*INCBIN "maps/JohtoHouse4.ablk.lz"', files["data/maps/blocks.asm"].decode())
        width, height = map(int, re.search(r"map_const " + constant + r",\s*(\d+),\s*(\d+)", files["constants/map_constants.asm"].decode()).groups())
        blocks = files[f"maps/{source_layout}.ablk"]
        assert len(blocks) == width * height
        symbols = banks[bank][3]
        raw, used = map_cells(blocks, width, symbols, BEHAVIORS)
        if name == "CherrygrovePokeCenter1F":
            cells = list(struct.unpack(f"<{len(raw)//2}H", raw))
            cells[7 * width * 2] |= 0xC00  # Communication floor opens after the Mystery Egg quest.
            raw = struct.pack(f"<{len(cells)}H", *cells)
        if name == "Route2946Gate":
            cells = list(struct.unpack(f"<{len(raw)//2}H", raw))
            for x in (4, 5):
                cells[x] |= 0xC00  # Keep source art; no dangling Route 46 warp.
            raw = struct.pack(f"<{len(cells)}H", *cells)
        if name in polished_route31.CLOSED:
            cells = list(struct.unpack(f"<{len(raw)//2}H", raw))
            for x,y in polished_route31.CLOSED[name]:
                cells[y * width * 2 + x] |= 0xC00
            raw = struct.pack(f"<{len(cells)}H", *cells)
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
        header, script, resident_info, story = map_header(name, constant, width * 2, height * 2, files[f"maps/{name}.asm"].decode(), files)
        if name in (*polished_cherrygrove.MAPS, *polished_route30.MAPS):
            script += polished_cherrygrove.furniture(name, header, raw, width * 2, symbols, files, source_text, text_definition).encode()
            script = script.rstrip() + b"\n"
        products[f"data/maps/{prefix}/map.json"] = json_bytes(header)
        products[f"data/maps/{prefix}/scripts.inc"] = script
        tileset = "PolishedNewBark" + bank
        layouts.append({"id": header["layout"], "name": prefix + "_Layout", "width": width * 2, "height": height * 2,
            "primary_tileset": f"gTileset_{tileset}Primary", "secondary_tileset": f"gTileset_{tileset}Secondary",
            "border_filepath": f"data/layouts/{prefix}/border.bin", "blockdata_filepath": f"data/layouts/{prefix}/map.bin",
            "layout_version": "emerald"})
        maps[constant] = {"name": name, "width": width * 2, "height": height * 2, "tileset": bank,
            "source_blocks": source_layout, "used_metatiles": sorted(used | border_used),
            "warps": header["warp_events"], "static_interactions": len(header["bg_events"]),
            "residents": resident_info, "story_encounters": story}
        if name == "NewBarkTown":
            cells = struct.unpack(f"<{len(raw) // 2}H", raw)
            def walkable(x, y):
                return not cells[y * width * 2 + x] & 0xC00 and cells[y * width * 2 + x] >> 12 == 3
            assert walkable(2, 2) and not any(walkable(x, y) for x, y in ((3, 1), (4, 2), (3, 3)))
            assert all(walkable(x, y) for x, y in ((2, 3), (2, 4), (2, 5), (3, 2)))
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
    report = {"source_commit": PIN, "scope": "source geography, connected interiors, static interactions, six ambient residents, rival window encounter, Lyra/Elm introduction, all three starter choices, first Lyra battle, aide Potion and Mom's opening with weekday/DST setup, schedules and neighbor; Route 29, Cherrygrove services, Route 30 with Mulch, native encounters, Mystery Egg and Oak; Elm's disaster call, first rival battle, theft investigation, rival naming, Egg delivery, Joey, Mom's savings, the Route 29 catching tutorial and Tuscany; Route 31 with Wade, Finch, Mom’s worried call and the Violet gate",
        "maps": maps, "layouts": layouts, "output_sha256": {p: hashlib.sha256(raw).hexdigest() for p, raw in products.items()}}
    products["tools/polished_port/manifests/new_bark.json"] = json_bytes(report)
    products["tools/polished_port/manifests/route29_wild.json"] = json_bytes({
        "source_commit": PIN, "source_weights": [30, 30, 20, 10, 5, 3, 2],
        "native_weights": [20, 20, 10, 10, 10, 10, 5, 5, 4, 4, 1, 1],
        "rate_translation": "18 * 16 / 2880 = 10% base check; native step cooldown, abilities and other modifiers remain",
        "evening": "17-21 hours: source day table for 153/256 rolls; night for 103/256",
        "encounters": polished_route29.wild_entries(files)})
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
    print(f"PASS: {len(products)} deterministic Johto assets; {len(report['maps'])} maps, {warps} reciprocal warp events")


if __name__ == "__main__":
    main()
