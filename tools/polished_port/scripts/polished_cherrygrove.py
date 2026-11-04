"""Pinned Cherrygrove first visit: geography, guide, residents and services."""
import re
import struct
import tempfile
from pathlib import Path

from new_bark_intro import coord, movement
from polished_route29 import object_at

MAPS = {
    "CherrygroveCity": "CHERRYGROVE_CITY",
    "CherrygroveMart": "CHERRYGROVE_MART",
    "CherrygrovePokeCenter1F": "CHERRYGROVE_POKECENTER_1F",
    "CherrygroveGymSpeechHouse": "CHERRYGROVE_GYM_SPEECH_HOUSE",
    "GuideGentsHouse": "GUIDE_GENTS_HOUSE",
    "CherrygroveEvolutionSpeechHouse": "CHERRYGROVE_EVOLUTION_SPEECH_HOUSE",
}
BLOCKS = {"CherrygroveMart": "Mart", "CherrygrovePokeCenter1F": "JohtoPokeCenter1F",
    "CherrygroveGymSpeechHouse": "JohtoHouse1", "CherrygroveEvolutionSpeechHouse": "JohtoHouse1",
    "GuideGentsHouse": "House2"}
# Existing IDs remain stable; these variants are appended to the graphics enum.
GRAPHICS = {
    "GRAMPS": ("gramps", "BROWN"), "YOUNGSTER_RED": ("youngster", "RED"),
    "POKEFAN_BLUE": ("pokefan_f", "BLUE"), "FISHER": ("fisher", "BLUE"),
    "FAT_GUY_RED": ("fat_guy", "RED"), "GENTLEMAN": ("gentleman", "BLUE"),
    "LADY_GREEN": ("lady", "GREEN"), "CLERK": ("clerk", "GREEN"),
    "SCHOOLBOY_RED": ("schoolboy", "RED"), "CUTE_GIRL_GREEN": ("cute_girl", "GREEN"),
    "POKEFAN_M_BROWN": ("pokefan_m", "BROWN"), "CHILD_RED": ("child", "RED"),
    "BOWING_NURSE": ("bowing_nurse", "PINK"), "PIDGEY": ("pidgey", "BROWN"),
    "COOL_DUDE_BLUE": ("cool_dude", "BLUE"),
}
SPRITES = {("SPRITE_" + sprite.upper(), color): key for key, (sprite, color) in GRAPHICS.items()}
SPRITES.update({("SPRITE_TEACHER", "GREEN"): "TEACHER", ("SPRITE_COOL_DUDE", "RED"): "COOL_DUDE"})


def service_palettes(files, key):
    from build_johto_demo import palette_rows
    rows = palette_rows(files[f"gfx/tilesets/{key.lower()}.pal"].decode())[:7]
    if key == "PokeCenter":
        # PokeCenterSpecialCase copies PAL_BG_RED into PAL_BG_ROOF in Johto.
        rows[6] = rows[1]
    else:
        assert key == "Mart"
        # Cherrygrove aliases GenericMart_BlockData: its green bank is blue.
        body = files["data/maps/palettes.asm"].decode().split("MartBluePalette:\n")[1].split("else")[0]
        rgb = re.findall(r"RGB\s+(\d+),\s*(\d+),\s*(\d+)", body)
        assert len(rgb) == 4
        rows[2] = [int(r) | int(g) << 5 | int(b) << 10 for r,g,b in rgb]
    return rows


def furniture(name, header, raw, width, symbols, files, text, define):
    """Source collision interactions, overridden by explicit map events."""
    labels = {"BOOKSHELF":"MagazineBookshelfText", "TRASH_CAN":"TrashCanText",
        "MART_SHELF":"MerchandiseShelfText", "TV":"TVText", "WINDOW":"WindowText",
        "FRIDGE":"RefrigeratorText", "SINK":"SinkText", "STOVE":"StoveText",
        "POKECENTER_SIGN":"PokeCenterSignText", "MART_SIGN":"MartSignText"}
    occupied = {(e["x"],e["y"]) for e in header["bg_events"]}
    code, emitted = "", set()
    for i, cell in enumerate(struct.unpack(f"<{len(raw)//2}H", raw)):
        symbol = symbols[cell & 0x3ff]
        x,y = i%width,i//width
        if symbol not in labels or (x,y) in occupied:
            continue
        label = labels[symbol]
        event = "Polished"+name+"_"+label.removesuffix("Text")
        header["bg_events"].append({"type":"sign", "x":x, "y":y, "elevation":0,
            "player_facing_dir":"BG_EVENT_PLAYER_FACING_ANY", "script":event})
        if event not in emitted:
            code += f"{event}::\n"
            if symbol == "TV":
                code += f"\tgetobjectfacingdirection OBJ_EVENT_ID_PLAYER, VAR_0x8004\n\tgoto_if_ne VAR_0x8004, DIR_NORTH, {event}_WrongSide\n"
            code += f"\tmsgbox {event}_Text, MSGBOX_SIGN\n\tend\n\n"
            code += define(event+"_Text", text(files["data/text/std_text.asm"].decode(),label))
            if symbol == "TV":
                code += f"{event}_WrongSide::\n\tmsgbox {event}_WrongText, MSGBOX_SIGN\n\tend\n\n"
                code += define(event+"_WrongText", text(files["data/text/std_text.asm"].decode(),"WrongSideText"))
            emitted.add(event)
    return code


def assets(files):
    from build_johto_demo import palette_rows
    from convert_player_walking import convert, _read_png_2bit_grayscale, _write_indexed_4bit_png
    result = {}
    with tempfile.TemporaryDirectory() as tmp:
        inp, out = Path(tmp) / "in.png", Path(tmp) / "out.png"
        for sprite in dict.fromkeys(row[0] for row in GRAPHICS.values()):
            if sprite in ("fat_guy", "schoolboy", "child"):
                continue
            path = f"gfx/icons/{sprite}.png" if sprite == "pidgey" else f"gfx/sprites/{sprite}.png"
            inp.write_bytes(files[path])
            if sprite in ("bowing_nurse", "pidgey"):
                w, h, pixels = _read_png_2bit_grayscale(inp)
                assert w == 16 and h == (48 if sprite == "bowing_nurse" else 32)
                # Pad each exact source 16x16 frame to native bottom anchoring.
                frames = [[[0]*16 for _ in range(16)] + [[3-v for v in row] for row in pixels[y:y+16]]
                          for y in range(0, h, 16)]
                _write_indexed_4bit_png(out, [sum((f[y] for f in frames), []) for y in range(32)])
            else:
                convert(inp, out)
            result[f"graphics/object_events/polished_{sprite}/walking.png"] = out.read_bytes()
    colors = re.findall(r"^\s*ow_npc_pal_const (\w+)", files["constants/sprite_data_constants.asm"].decode(), re.M)
    rows = palette_rows(files["gfx/overworld/npc_sprites.pal"].decode())
    pink = rows[20 + colors.index("PINK")]
    result["graphics/object_events/polished_npc_palettes/pink.bin"] = struct.pack("<16H", *pink, *([0]*12))
    result.update(map_assets(files))
    return result


def map_assets(files):
    from stage_johto_tile_graphics import parse_grayscale2_png
    png = files["gfx/town_map/town_map.png"]
    w, h = struct.unpack_from(">II", png, 16)
    pixels = parse_grayscale2_png(png, expected_width=w, expected_height=h)
    def tile(rows):
        return bytes((3-row[x]) | (3-row[x+1]) << 4 for row in rows for x in range(0,8,2))
    graphics = b"".join(tile([r[x:x+8] for r in pixels[y:y+8]]) for y in range(0,h,8) for x in range(0,w,8))
    pal_text = files["gfx/pokegear/pokegear.pal"].decode().split("else")[0]
    colors = [tuple(map(int,m)) for m in re.findall(r"RGB\s+(\d+),\s*(\d+),\s*(\d+)",pal_text)]
    assert len(colors) == 32
    colors = [r | g << 5 | b << 10 for r,g,b in colors]
    palettes = b"".join(struct.pack("<16H", *colors[i:i+4], *([0]*12)) for i in range(0,32,4))
    pal_map = [int(n) for row in re.findall(r"^\s*townmappals ([\d, ]+)$", files["engine/pokegear/pokegear.asm"].decode(), re.M) for n in row.split(",")]
    raw = files["gfx/town_map/johto.bin"]
    assert len(raw) == 361 and raw[-1] == 255 and len(pal_map) == 64
    tilemap = [0]*1024
    for i,v in enumerate(raw[:-1]):
        tilemap[(i//20+1)*32+i%20] = (v&63) | ((v>>6)&1)*0x400 | ((v>>7)&1)*0x800 | pal_map[v&63]<<12
    landmarks = files["data/maps/landmarks.asm"].decode()
    entries = re.findall(r"^\s*landmark\s+(\d+),\s*(\d+),\s*(\w+)", landmarks.split("assert_table_length KANTO_LANDMARK")[0], re.M)
    data = "// Generated from the pinned Johto landmark names and pixel coordinates.\nstatic const struct PolishedLandmark sJohtoLandmarks[] = {\n"
    for x,y,label in entries:
        name = re.search(r"^"+label+r':\s+rawchar "([^"]+)@"',landmarks,re.M)[1]
        name = name.replace("¯", "\\n").replace("#", "POKé")
        data += f'    {{{x}, {y}, COMPOUND_STRING("{name}")}},\n'
    data += "};\n"
    png = files["gfx/town_map/arrow.png"]
    w,h = struct.unpack_from(">II",png,16)
    pixels = parse_grayscale2_png(png,expected_width=w,expected_height=h)
    cursor = b"".join(tile([r[x:x+8] for r in pixels[y:y+8]]) for y in (0,8) for x in (0,8))
    return {"graphics/polished_town_map/tiles.bin": graphics,
        "graphics/polished_town_map/map.bin": struct.pack("<1024H",*tilemap),
        "graphics/polished_town_map/palettes.bin":palettes,
        "graphics/polished_town_map/cursor.bin":cursor,
        "src/data/polished_johto_landmarks.h":data.encode()}


def plain_text(script, label, text):
    """Select the first-visit branch's complete text, rejecting empty extracts."""
    match = re.search(r"^" + re.escape(label) + r":\n(.*?)(?=^\w+:|\Z)", script, re.M | re.S)
    assert match, label
    body = match[1]
    body = body[re.search(r'^\s*text "', body, re.M).start():]
    body = body[:re.search(r"^\s*done\s*$", body, re.M).end()]
    body = body.replace("<PK><MN>", "POKéMON")
    return text("Selected:\n" + body, "Selected")


def events(name, script, header, files, text, define):
    p = "Polished" + name
    code, report = "", {"first_visit": True, "residents": []}
    for raw in re.findall(r"^\s*object_event (.*)$", script, re.M):
        f = [s.strip() for s in raw.split(",")]
        x, y, sprite, move, ry, rx, time, color, kind, command, label, flag = f
        if sprite == "SPRITE_RIVAL":
            continue  # The return encounter appends him after existing local IDs.
        if color == "0":
            camel = "".join(s.title() for s in sprite.removeprefix("SPRITE_").split("_"))
            color = "PAL_NPC_" + re.search(camel + r"SpriteGFX, WALKING_SPRITE, PAL_OW_(\w+)", files["data/sprites/sprites.asm"].decode())[1]
        color = color.removeprefix("PAL_NPC_")
        graphics = SPRITES[sprite, color]
        local = "LOCALID_" + MAPS[name] + "_" + str(len(header["object_events"]) + 1)
        if name == "CherrygroveCity" and label == "CherrygroveCityGuideGent":
            local, flag = "LOCALID_CHERRYGROVE_GUIDE", "FLAG_POLISHED_GUIDE_HOME"
        elif name == "GuideGentsHouse":
            flag = "FLAG_TEMP_2"
        else:
            flag = "0"
        event = p + "_" + label
        obj = object_at(local, "POLISHED_" + graphics, int(x), int(y), event, flag)
        obj["movement_type"] = {"SPRITEMOVEDATA_STANDING_DOWN": "MOVEMENT_TYPE_FACE_DOWN",
            "SPRITEMOVEDATA_STANDING_UP": "MOVEMENT_TYPE_FACE_UP", "SPRITEMOVEDATA_STANDING_LEFT": "MOVEMENT_TYPE_FACE_LEFT",
            "SPRITEMOVEDATA_STANDING_RIGHT": "MOVEMENT_TYPE_FACE_RIGHT", "SPRITEMOVEDATA_WALK_LEFT_RIGHT": "MOVEMENT_TYPE_WANDER_LEFT_AND_RIGHT"}[move]
        obj["movement_range_x"], obj["movement_range_y"] = int(rx), int(ry)
        header["object_events"].append(obj)
        report["residents"].append({"source_label": label, "color": color, "event": obj})
        if label in ("CherrygroveCityGuideGent", "MysticWaterGuy", "CherrygroveMartClerkScript", "GuideGentsHouseGuideGentText"):
            continue
        # jumptext preserves the facing of the two children speaking together.
        box = "MSGBOX_NPC" if command != "jumptext" else "MSGBOX_SIGN"
        code += f"{event}::\n\tmsgbox {event}_Text, {box}\n\tend\n\n"
        code += define(event + "_Text", plain_text(script, label, text))
    hooks = ""
    if name == "CherrygroveCity":
        header["connections"] = [{"map": "MAP_POLISHED_ROUTE_29", "offset": 0, "direction": "right"},
            {"map": "MAP_POLISHED_ROUTE_30", "offset": 10, "direction": "up"}]
        header["bg_events"].append({"type": "hidden_item", "x": 35, "y": 2, "elevation": 0,
            "item": "ITEM_NUGGET", "flag": "FLAG_POLISHED_CHERRYGROVE_NUGGET"})
        bird = object_at("LOCALID_CHERRYGROVE_PIDGEY", "POLISHED_PIDGEY", 26, 13, p + "_Pidgey")
        header["object_events"].append(bird)
        code += f"{p}_Pidgey::\n\tlock\n\tplaymoncry SPECIES_PIDGEY, CRY_MODE_NORMAL\n\tmsgbox {p}_Pidgey_Text, MSGBOX_DEFAULT\n\twaitmoncry\n\trelease\n\tend\n\n"
        code += define(p + "_Pidgey_Text", text(script, "CherrygrovePidgeyText"))
        code += f"""{p}_MysticWaterGuy::
\tlock
\tfaceplayer
\tgoto_if_set FLAG_POLISHED_CHERRYGROVE_MYSTIC_WATER, {p}_MysticAfter
\tmsgbox {p}_MysticBefore_Text, MSGBOX_DEFAULT
\tgiveitem ITEM_MYSTIC_WATER
\tgoto_if_eq VAR_RESULT, FALSE, {p}_MysticDone
\tsetflag FLAG_POLISHED_CHERRYGROVE_MYSTIC_WATER
{p}_MysticAfter::
\tmsgbox {p}_MysticAfter_Text, MSGBOX_DEFAULT
{p}_MysticDone::
\trelease
\tend

"""
        for short, label in (("Before", "MysticWaterGuyTextBefore"), ("After", "MysticWaterGuyTextAfter")):
            code += define(p + "_Mystic" + short + "_Text", text(script, label))
        tour, tour_report = guide(script, header, text, define)
        code += tour
        report["tour"] = tour_report
    elif name == "GuideGentsHouse":
        hooks = f"\tmap_script MAP_SCRIPT_ON_LOAD, {p}_Setup\n"
        code += f"""{p}_Setup::
\tsetflag FLAG_TEMP_2
\tgoto_if_unset FLAG_POLISHED_GUIDE_HOME, {p}_SetupDone
\tclearflag FLAG_TEMP_2
{p}_SetupDone::
\tend

{p}_GuideGentsHouseGuideGentText::
\tlock
\tfaceplayer
\tcall_if_unset FLAG_POLISHED_MAP_CARD, PolishedCherrygrove_GiveMap
\tmsgbox {p}_GuideText, MSGBOX_DEFAULT
\trelease
\tend

"""
        code += define(p + "_GuideText", text(script, "GuideGentsHouseGuideGentText"))
    elif name == "CherrygroveMart":
        stock = re.search(r"^CherrygroveMart:\n(.*?)^\s*db -1", files["data/items/marts.asm"].decode(), re.M | re.S)[1]
        items = re.findall(r"^\s*db ([A-Z_]+)\s*$", stock, re.M)
        assert items == ["POTION", "ANTIDOTE", "PARALYZEHEAL", "AWAKENING"]
        code += f"""{p}_CherrygroveMartClerkScript::
\tlock
\tfaceplayer
\tmessage gText_HowMayIServeYou
\twaitmessage
\tpokemart {p}_Stock
\tmsgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
\trelease
\tend

\t.align 2
{p}_Stock:
""" + "".join("\t.2byte ITEM_" + i.replace("PARALYZEHEAL", "PARALYZE_HEAL") + "\n" for i in items) + "\tpokemartlistend\n\n"
        report["stock"] = items
    elif name == "CherrygrovePokeCenter1F":
        hooks = f"\tmap_script MAP_SCRIPT_ON_TRANSITION, {p}_Respawn\n"
        nurse = object_at("LOCALID_CHERRYGROVE_NURSE", "POLISHED_BOWING_NURSE", 5, 1, p + "_Nurse")
        header["object_events"].append(nurse)
        # Source PC and wall map are collision interactions, not bg_events.
        for x, y, event in ((7, 2, p + "_PC"), (9, 0, "EventScript_RegionMap"), (10, 1, p + "_Journal")):
            header["bg_events"].append({"type": "sign", "x": x, "y": y, "elevation": 0,
                "player_facing_dir": "BG_EVENT_PLAYER_FACING_NORTH", "script": event})
        code += center(script, files, text, define)
    return hooks, code, report


def guide(script, header, text, define):
    p = "PolishedCherrygrove"
    coord(header, 33, 7, "VAR_POLISHED_CHERRYGROVE_SCENE", 0, p + "_Trigger")
    code = f"""{p}_Trigger::
\tlockall
\tapplymovement OBJ_EVENT_ID_PLAYER, {p}_Enter
\twaitmovement 0
\tgoto {p}_Tour

PolishedCherrygroveCity_CherrygroveCityGuideGent::
\tlockall
\tgetplayerxy VAR_0x8004, VAR_0x8005
\tgoto_if_eq VAR_0x8004, 31, {p}_FromWest
\tgoto_if_eq VAR_0x8004, 33, {p}_FromEast
\tgoto {p}_Tour
{p}_FromWest::
\tapplymovement OBJ_EVENT_ID_PLAYER, {p}_WestApproach
\twaitmovement 0
\tgoto {p}_Tour
{p}_FromEast::
\tapplymovement OBJ_EVENT_ID_PLAYER, {p}_EastApproach
\twaitmovement 0
{p}_Tour::
\tturnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
\tmsgbox {p}_GuideGentIntroText, MSGBOX_DEFAULT
"""
    segments = [("1", "GuideGentPokeCenterText", "UP"), ("2", "GuideGentMartText", "UP"),
        ("3", "GuideGentRoute30Text", "UP"), ("3_5", "GuideGentAdvancedTipsText", "LEFT"),
        ("4", "GuideGentSeaText", "LEFT"), ("5", "GuideGentGiftText", "RIGHT")]
    moves, report = "", []
    guide_pos, player_pos = (32, 6), (32, 7)
    delta = {"left": (-1, 0), "right": (1, 0), "up": (0, -1), "down": (0, 1)}
    for n, label, facing in segments:
        g = f"{p}_Guide{n}"; f = f"{p}_Follow{n}"
        translated = movement(script, "GuideGentMovement" + n, g)
        follow = []
        for direction in re.findall(r"\twalk_(\w+)", translated):
            dx, dy = guide_pos[0]-player_pos[0], guide_pos[1]-player_pos[1]
            assert (dx, dy) in delta.values(), (n, guide_pos, player_pos)
            follow.append(next(d for d, value in delta.items() if value == (dx,dy)))
            player_pos = guide_pos
            dx, dy = delta[direction]
            guide_pos = guide_pos[0]+dx, guide_pos[1]+dy
        moves += translated + f + ":\n" + "".join("\twalk_"+d+"\n" for d in follow) + "\tstep_end\n\n"
        # Concurrent native movement reproduces the source follower's previous-cell path.
        code += f"\tclosemessage\n\tapplymovement LOCALID_CHERRYGROVE_GUIDE, {g}\n\tapplymovement OBJ_EVENT_ID_PLAYER, {f}\n\twaitmovement OBJ_EVENT_ID_PLAYER\n\twaitmovement LOCALID_CHERRYGROVE_GUIDE\n\tturnobject OBJ_EVENT_ID_PLAYER, DIR_{ {'UP':'NORTH','LEFT':'WEST','RIGHT':'EAST'}[facing]}\n"
        if n == "5":
            code += "\tturnobject LOCALID_CHERRYGROVE_GUIDE, DIR_WEST\n"
        code += f"\tmsgbox {p}_{label}, MSGBOX_DEFAULT\n"
        report.append({"stop": label, "guide": guide_pos, "player": player_pos})
    assert (guide_pos, player_pos) == ((25, 11), (24, 11))
    code += f"""\tcall {p}_GiveMap
\tclosemessage
\tturnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
\tapplymovement LOCALID_CHERRYGROVE_GUIDE, {p}_Leave
\twaitmovement 0
\tplayse SE_DOOR
\tremoveobject LOCALID_CHERRYGROVE_GUIDE
\tsetflag FLAG_POLISHED_GUIDE_HOME
\tsetvar VAR_POLISHED_CHERRYGROVE_SCENE, 2
\treleaseall
\tend

{p}_GiveMap::
\tgoto_if_set FLAG_POLISHED_MAP_CARD, {p}_MapDone
\tgiveitem ITEM_TOWN_MAP
\tgoto_if_eq VAR_RESULT, FALSE, {p}_MapFull
\tsetflag FLAG_POLISHED_MAP_CARD
\tmsgbox {p}_MapAccess, MSGBOX_DEFAULT
{p}_MapDone::
\treturn
{p}_MapFull::
\tmsgbox {p}_MapFullText, MSGBOX_DEFAULT
\treturn

{p}_WestApproach:
\twalk_down
\twalk_right
\tstep_end
{p}_EastApproach:
\twalk_down
\twalk_left
\tstep_end

"""
    code += movement(script, "GuideGentPlayerMovement", p + "_Enter")
    code += movement(script, "GuideGentMovement6", p + "_Leave") + moves
    for label in ["GuideGentIntroText"] + [s[1] for s in segments]:
        code += define(p+"_"+label, text(script, label))
    code += define(p+"_MapAccess", "Open the TOWN MAP from\\nyour BAG's KEY ITEMS.\\pI wish you luck on\\nyour journey!$")
    code += define(p+"_MapFullText", "Your BAG is full.\\pCome to my house when\\nyou have room for the map.$")
    return code, report


def center(script, files, text, define):
    p = "PolishedCherrygrovePokeCenter1F"
    code = f"""{p}_Respawn::
\tsetrespawn HEAL_LOCATION_POLISHED_CHERRYGROVE
\tend

{p}_Nurse::
\tlockall
\tgettime
\tgoto_if_lt VAR_0x8000, 5, {p}_Nite
\tgoto_if_lt VAR_0x8000, 9, {p}_Morn
\tgoto_if_lt VAR_0x8000, 17, {p}_Day
\tgoto_if_lt VAR_0x8000, 21, {p}_Eve
{p}_Nite::
\tmsgbox {p}_NurseNiteText, MSGBOX_DEFAULT
\tgoto {p}_Ask
{p}_Morn::
\tmsgbox {p}_NurseMornText, MSGBOX_DEFAULT
\tgoto {p}_Ask
{p}_Day::
\tmsgbox {p}_NurseDayText, MSGBOX_DEFAULT
\tgoto {p}_Ask
{p}_Eve::
\tmsgbox {p}_NurseEveText, MSGBOX_DEFAULT
{p}_Ask::
\tmsgbox {p}_NurseAskHealText, MSGBOX_YESNO
\tgoto_if_eq VAR_RESULT, NO, {p}_Goodbye
\tmsgbox {p}_NurseTakePokemonText, MSGBOX_DEFAULT
\tclosemessage
\tturnobject LOCALID_CHERRYGROVE_NURSE, DIR_WEST
\tplayfanfare MUS_HEAL
\twaitfanfare
\tspecial HealPlayerParty
\tturnobject LOCALID_CHERRYGROVE_NURSE, DIR_SOUTH
\tmsgbox {p}_NurseReturnPokemonText, MSGBOX_DEFAULT
{p}_Goodbye::
\tturnobject LOCALID_CHERRYGROVE_NURSE, DIR_NORTH
\tdelay 10
\tturnobject LOCALID_CHERRYGROVE_NURSE, DIR_SOUTH
\tmsgbox {p}_NurseGoodbyeText, MSGBOX_DEFAULT
\treleaseall
\tend

@ Native storage/Bag PC menus, without Emerald metatile substitutions.
{p}_PC::
\tlockall
\tplayse SE_PC_ON
\tmsgbox Text_BootUpPC, MSGBOX_DEFAULT
\tgoto EventScript_PCMainMenu

{p}_Journal::
\tsetflag FLAG_POLISHED_READ_RED_JOURNAL
\tmsgbox {p}_JournalText, MSGBOX_SIGN
\tend

"""
    for label in ("NurseMornText", "NurseDayText", "NurseEveText", "NurseNiteText", "NurseAskHealText",
                  "NurseTakePokemonText", "NurseReturnPokemonText", "NurseGoodbyeText"):
        code += define(p+"_"+label, text(files["data/text/std_text.asm"].decode(), label))
    code += define(p+"_JournalText", plain_text(script, "PokemonJournalRedScript", text))
    return code
