"""The Violet City first visit, from the Route 31 gate to the Pokémon Center."""
import re
import tempfile
from pathlib import Path

from convert_player_walking import convert
from polished_academy import outdoor
from polished_cherrygrove import center, standing_sheet
from polished_route29 import object_at

MAPS = {"VioletCity": "VIOLET_CITY", "VioletPokeCenter1F": "VIOLET_POKECENTER_1F"}
# Keep source artwork at entrances whose destination has a later campaign phase.
CLOSED = {"VioletCity": ((9, 17), (18, 17), (3, 15), (21, 29),
                        (23, 1), (2, 8), (2, 9))}
GRAPHICS = {
    ("CUTE_GIRL", "GREEN"): "CUTE_GIRL_GREEN", ("ACE_TRAINER_M", "RED"): "ACE_TRAINER_M",
    ("ACE_TRAINER_F", "RED"): "ACE_TRAINER_F", ("GRAMPS", "BROWN"): "GRAMPS",
    ("YOUNGSTER", "GREEN"): "YOUNGSTER_GREEN", ("FAT_GUY", "BLUE"): "FAT_GUY_BLUE",
    ("COOL_DUDE", "PURPLE"): "COOL_DUDE_PURPLE", ("GAMEBOY_KID", "GREEN"): "GAMEBOY_KID",
    ("GENTLEMAN", "BLUE"): "GENTLEMAN", ("SAGE", "BLUE"): "SAGE",
    ("SCHOOLBOY", "RED"): "SCHOOLBOY_RED",
}
MOVEMENTS = {"WANDER": "WANDER_AROUND", "STANDING_UP": "FACE_UP", "STANDING_DOWN": "FACE_DOWN",
             "STANDING_RIGHT": "FACE_RIGHT", "STANDING_LEFT": "FACE_LEFT",
             "SPINRANDOM_SLOW": "LOOK_AROUND", "WALK_LEFT_RIGHT": "WANDER_LEFT_AND_RIGHT"}


def assets(files):
    result = {}
    with tempfile.TemporaryDirectory() as tmp:
        inp, out = Path(tmp)/"sprite.png", Path(tmp)/"walking.png"
        for name in ("ace_trainer_f", "sage", "gameboy_kid"):
            inp.write_bytes(files[f"gfx/sprites/{name}.png"])
            if name == "gameboy_kid":
                standing_sheet(inp, out, 48)
            else:
                convert(inp, out)
            result[f"graphics/object_events/polished_{name}/walking.png"] = out.read_bytes()
    return result


def events(name, script, header, files, text, define):
    prefix = "Polished" + name
    code = ""
    for raw in re.findall(r"^\s*object_event (.*)$", script, re.M):
        x, y, sprite, move, ry, rx, schedule, color, kind, command, label, flag = (
            field.strip() for field in raw.split(","))
        if kind == "OBJECTTYPE_SCRIPT":
            # Earl is appended after the existing local IDs. The aide arrives after Falkner.
            assert label in ("VioletCityEarlScript", "VioletPokeCenter1FElmsAideScript")
            continue
        assert kind == "OBJECTTYPE_COMMAND" and command == "jumptextfaceplayer" and flag == "-1"
        sprite = sprite.removeprefix("SPRITE_")
        if color == "0":
            source_name = "".join(part.title() for part in sprite.split("_"))
            color = re.search(source_name + r"SpriteGFX, \w+_SPRITE, PAL_OW_(\w+)",
                              files["data/sprites/sprites.asm"].decode())[1]
        gfx = GRAPHICS[sprite, color.removeprefix("PAL_NPC_")]
        local = "LOCALID_" + MAPS[name] + "_" + sprite
        visibility = {"-1": "0", "(1 << MORN) | (1 << DAY)": "FLAG_TEMP_1",
                      "(1 << EVE) | (1 << NITE)": "FLAG_TEMP_2"}[schedule]
        event = prefix + "_" + label
        obj = object_at(local, "POLISHED_" + gfx, int(x), int(y), event, visibility)
        obj.update(movement_type="MOVEMENT_TYPE_"+MOVEMENTS[move.removeprefix("SPRITEMOVEDATA_")],
                   movement_range_y=int(ry), movement_range_x=int(rx))
        header["object_events"].append(obj)
        code += f"{event}::\n\tmsgbox {event}_Text, MSGBOX_NPC\n\tend\n\n"
        code += define(event+"_Text", text(script, label))

    if name == "VioletPokeCenter1F":
        header["object_events"].append(object_at("LOCALID_VIOLET_NURSE", "POLISHED_BOWING_NURSE",
                                                 5, 1, prefix+"_Nurse"))
        for x, y, handler in ((7, 2, prefix+"_PC"), (9, 0, "EventScript_RegionMap"), (10, 1, prefix+"_Journal")):
            header["bg_events"].append({"type": "sign", "x": x, "y": y, "elevation": 0,
                "player_facing_dir": "BG_EVENT_PLAYER_FACING_NORTH", "script": handler})
        code += center(script, files, text, define, city="Violet", journal="Falkner")
        return f"\tmap_script MAP_SCRIPT_ON_TRANSITION, {prefix}_Respawn\n", code, {
            "services": ["heal", "blackout recovery", "storage", "wall map", "Falkner journal"],
            "deferred": ["post-badge Togepi Egg", "communication floor"]}

    header["connections"] = [{"map": "MAP_POLISHED_ROUTE_31", "offset": 18, "direction": "right"}]
    for x, y, item in re.findall(r"^\s*bg_event\s+(\d+),\s*(\d+), BGEVENT_ITEM \+ (\w+),", script, re.M):
        header["bg_events"].append({"type": "hidden_item", "x": int(x), "y": int(y), "elevation": 0,
            "item": "ITEM_"+item, "flag": "FLAG_POLISHED_VIOLET_HIDDEN_"+item})
    for x, y, item in re.findall(r"^\s*itemball_event\s+(\d+),\s*(\d+), (\w+), 1,", script, re.M):
        event = prefix + "_" + item
        header["object_events"].append(object_at("LOCALID_VIOLET_"+item, "POLISHED_BALL_RED",
                                                 int(x), int(y), event, "FLAG_POLISHED_VIOLET_"+item))
        code += f"{event}::\n\tfinditem ITEM_{item}\n\tend\n\n"
    header["object_events"].append(object_at("LOCALID_VIOLET_CUT", "POLISHED_CUT_TREE", 36, 19,
                                             "EventScript_CutTree", "FLAG_POLISHED_VIOLET_CUT"))
    header["object_events"].append(object_at("LOCALID_VIOLET_CHERI", "VAR_B", 14, 29, prefix+"_Cheri"))
    hooks = ("\tmap_script MAP_SCRIPT_ON_LOAD, PolishedVioletCity_Setup\n"
             "\tmap_script MAP_SCRIPT_ON_RETURN_TO_FIELD, PolishedVioletCity_Restore\n")
    earl, door = outdoor(script, header, files, text, define)
    code += earl
    code += f"""PolishedVioletCity_Cheri::
    setvar VAR_0x8005, 4
    goto PolishedFruit_Interact

PolishedVioletCity_Setup::
    setmetatile 30, 17, {door}, FALSE
    setflag FLAG_POLISHED_VISITED_VIOLET
    special PolishedFruitRefreshAll
    gettime
    goto_if_lt VAR_0x8000, 5, PolishedVioletCity_Night
    goto_if_ge VAR_0x8000, 17, PolishedVioletCity_Night
    clearflag FLAG_TEMP_1
    setflag FLAG_TEMP_2
    end

PolishedVioletCity_Night::
    setflag FLAG_TEMP_1
    clearflag FLAG_TEMP_2
    end

PolishedVioletCity_Restore::
    goto_if_set FLAG_POLISHED_EARL_IN_ACADEMY, PolishedVioletCity_RestoreResidents
    addobject LOCALID_VIOLET_EARL
PolishedVioletCity_RestoreResidents::
    goto_if_set FLAG_TEMP_1, PolishedVioletCity_ShowNight
    removeobject LOCALID_VIOLET_CITY_ACE_TRAINER_F
    addobject LOCALID_VIOLET_CITY_ACE_TRAINER_M
    end

PolishedVioletCity_ShowNight::
    removeobject LOCALID_VIOLET_CITY_ACE_TRAINER_M
    addobject LOCALID_VIOLET_CITY_ACE_TRAINER_F
    end
"""
    return hooks, code, {"roof_group": 10, "palette": "VioletEcruteakPalette",
                         "deferred": ["houses and Mart", "Gym",
                                      "Sprout Tower", "Route 36 gate and adjoining routes"]}
