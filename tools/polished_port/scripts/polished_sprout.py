"""Pinned Sprout Tower first visit: three floors, seven trainers and the summit."""
import re
import tempfile
from pathlib import Path

from build_johto_demo import gba_tile, parse_grayscale2_png
from convert_player_walking import convert, _read_png_2bit_grayscale, _write_indexed_4bit_png
from polished_route29 import object_at
from polished_route30 import palette
from new_bark_intro import coord

MAPS = {f"SproutTower{n}F": f"SPROUT_TOWER_{n}F" for n in (1, 2, 3)}
TRAINERS = ("Chow", "Nico", "Edmond", "Jin", "Troy", "Neal", "Li")
PILLAR_TILES = (0x2d, 0x2f, 0x3d, 0x3f, 0x3c, 0x2c, 0x4d, 0x4f, 0x5d, 0x5f)
# Preserve all seven distinct source slots, including the 3% and 2% levels.
SLOT_ASSIGNMENT = (0, 1, 0, 1, 2, 2, 3, 3, 4, 5, 6, 6)
SLOT_WEIGHTS = (20, 20, 10, 10, 10, 10, 5, 5, 5, 3, 1, 1)


def trainers(files):
    parties = files["data/trainers/parties.asm"].decode()
    attacks = files["data/pokemon/evos_attacks.asm"].decode()
    result = "/* Generated from Polished Crystal v3.2.3; modern EV conversion. */\n"
    for name in TRAINERS:
        elder = name == "Li"
        group = parties.split(('Elder' if elder else 'Sage') + 'Group:\n', 1)[1]
        body = re.search(r'def_trainer ' + name.upper() + r', "' + name + r'"\n(.*?)end_trainer', group, re.S)[1]
        kind = "Elder" if elder else "Sage"
        result += f"\n=== TRAINER_POLISHED_SPROUT_{name.upper()} ===\nName: {name}\nClass: {kind}\nPic: Polished {kind}\nGender: Male\nMusic: Suspicious\nDouble Battle: No\n"
        if elder:
            assert "; Elder\n\tdb SUPER_POTION, 0" in files["data/trainers/attributes.asm"].decode()
            result += "Items: Super Potion\n"
        # Source class DVs are 12/13; modern EVs divide class EVs by three.
        values = re.search(r"db\s+(\d+), \$([0-9A-F]{2}), \$([0-9A-F]{2}), \$([0-9A-F]{2}), MALE\s*; " + kind.lower() + r"\b", files["data/trainers/dvs.asm"].decode()).groups()
        assert len(set("".join(values[1:]))) == 1
        dv, ev = int(values[1][0], 16) * 2, int(values[0]) // 3
        for level, species in re.findall(r"tr_mon (\d+), (\w+)", body):
            learnset = attacks.split(f"evos_attacks {species.title()}\n", 1)[1].split("evos_attacks ", 1)[0]
            moves = [m for lv, m in re.findall(r"learnset (\d+), (\w+)", learnset) if int(lv) <= int(level)][-4:]
            result += f"\n{species.title()} (M)\nLevel: {level}\nNature: Hardy\n"
            for label, value in (("IVs", dv), ("EVs", ev)):
                result += label + ": " + " / ".join(f"{value} {s}" for s in ("HP", "Atk", "Def", "SpA", "SpD", "Spe")) + "\n"
            result += "".join(f"- MOVE_{m}\n" for m in moves)
    return result


def assets(files):
    result = {"src/data/polished_sprout.party": trainers(files).encode()}
    with tempfile.TemporaryDirectory() as tmp:
        src, out = Path(tmp) / "source.png", Path(tmp) / "out.png"
        for name in ("elder", "kimono_girl"):
            src.write_bytes(files[f"gfx/sprites/{name}.png"])
            convert(src, out)
            result[f"graphics/object_events/polished_{name}/walking.png"] = out.read_bytes()
        for name in ("sage", "elder"):
            src.write_bytes(files[f"gfx/trainers/{name}.png"])
            w, h, pixels = _read_png_2bit_grayscale(src)
            assert (w, h) == (56, 56)
            canvas = [[0] * 64 for _ in range(64)]
            for y, row in enumerate(pixels):
                canvas[y + 8][4:60] = [3 - v for v in row]
            _write_indexed_4bit_png(out, canvas)
            result[f"graphics/trainers/front_pics/polished_{name}.png"] = out.read_bytes()
            colors = [tuple(map(int, row)) for row in re.findall(r"RGB\s+(\d+),\s*(\d+),\s*(\d+)", files[f"gfx/trainers/{name}.pal"].decode())]
            assert len(colors) == 2
            result[f"graphics/trainers/palettes/polished_{name}.pal"] = palette(colors)
    animation = files["engine/tilesets/tileset_anims.asm"].decode()
    frames = bytearray()
    for n, tile in enumerate(PILLAR_TILES, 1):
        assert re.search(rf"TowerPillarTilePointer{n}:\s+dw vTiles2 tile \${tile:02x}, TowerPillarTile{n}\b", animation)
        pixels = parse_grayscale2_png(files[f"gfx/tilesets/tower-pillar/{n}.png"], expected_width=8, expected_height=40)
        frames.extend(b"".join(gba_tile(pixels[y:y + 8]) for y in range(0, 40, 8)))
    result["graphics/polished_sprout/pillar.bin"] = bytes(frames)
    return result


def wild_entries(files):
    result = []
    for floor in (2, 3):
        constant = f"SPROUT_TOWER_{floor}F"
        body = re.search(rf"def_grass_wildmons {constant}\n(.*?)end_grass_wildmons", files["data/wild/johto_grass.asm"].decode(), re.S)[1]
        assert "2 percent, 2 percent, 2 percent" in body
        slots = [(int(lv), sp) for lv, sp in re.findall(r"wildmon (\d+), (\w+)", body)]
        assert len(slots) == 21
        for n, period in enumerate(("Morning", "Day", "Night")):
            group = slots[n * 7:n * 7 + 7]
            result.append({"map": "MAP_POLISHED_" + constant, "base_label": f"gPolishedSproutTower{floor}F_{period}",
                "always_time_of_day": True, "land_mons": {"encounter_rate": 4,
                    "mons": [{"min_level": group[i][0], "max_level": group[i][0], "species": "SPECIES_" + group[i][1]} for i in SLOT_ASSIGNMENT]}})
    return result


def inline_text(script, label, text):
    """Skip the generictrainer or conditional command before an inline body."""
    body = script.split(label + ":\n", 1)[1]
    body = body[body.index('\ttext "'):]
    return text("Body:\n" + body, "Body")


def events(name, script, header, files, text, define):
    prefix = "Polished" + name
    floor = int(name[-2])
    code = ""
    gfx = {"KIMONO_GIRL": "KIMONO_GIRL_GREEN", "SAGE": "SAGE", "ELDER": "ELDER",
           "GRANNY": "GRANNY", "TEACHER": "TEACHER", "RIVAL": "RIVAL"}
    moves = {"STANDING_UP": "FACE_UP", "STANDING_DOWN": "FACE_DOWN", "STANDING_LEFT": "FACE_LEFT",
             "STANDING_RIGHT": "FACE_RIGHT", "WANDER": "WANDER_AROUND", "SPINRANDOM_FAST": "LOOK_AROUND"}
    for number, row in enumerate(re.findall(r"^\s*object_event (.*)$", script, re.M), 1):
        x, y, sprite, movement, ry, rx, time, pal, kind, sight, label, flag = [v.strip() for v in row.split(",")]
        local = f"LOCALID_SPROUT_{floor}F_{number}"
        action = prefix + "_" + label
        npc = object_at(local, "POLISHED_" + gfx[sprite.removeprefix("SPRITE_")], int(x), int(y), action,
                        "FLAG_POLISHED_SPROUT_RIVAL_LEFT" if sprite == "SPRITE_RIVAL" else "0")
        npc.update(movement_type="MOVEMENT_TYPE_" + moves[movement.removeprefix("SPRITEMOVEDATA_")],
                   movement_range_x=int(rx), movement_range_y=int(ry))
        if kind == "OBJECTTYPE_GENERICTRAINER":
            trainer = label.removeprefix("GenericTrainerSage")
            npc.update(trainer_type="TRAINER_TYPE_NORMAL", trainer_sight_or_berry_tree_id=sight)
            code += f"{action}::\n\ttrainerbattle_single TRAINER_POLISHED_SPROUT_{trainer.upper()}, {action}_Seen, {action}_Beaten\n\tmsgbox {action}_After, MSGBOX_AUTOCLOSE\n\tend\n"
            code += define(action + "_Seen", text(script, f"Sage{trainer}SeenText"))
            code += define(action + "_Beaten", text(script, f"Sage{trainer}BeatenText"))
            code += define(action + "_After", inline_text(script, label, text))
        elif label == "ElderLiScript":
            code += elder(script, text, define)
        elif label == "ObjectEvent":
            code += f"{action}::\n\tend\n"
        else:
            body = text(script, label) if kind == "OBJECTTYPE_COMMAND" else text(script, ".IntroText") if label == "KimonoGirlMakoScript" else inline_text(script, label, text)
            code += f"{action}::\n\tmsgbox {action}_Text, MSGBOX_NPC\n\tend\n" + define(action + "_Text", body)
        header["object_events"].append(npc)
    for x, y, item, quantity, source_flag in re.findall(r"itemball_event\s+(\d+),\s*(\d+), (\w+), (\d+), (\w+)", script):
        item = {"PARALYZEHEAL": "PARALYZE_HEAL"}.get(item, item)
        action = prefix + "_" + item
        header["object_events"].append(object_at(f"LOCALID_SPROUT_{floor}F_{item}", "POLISHED_BALL_RED", int(x), int(y), action, f"FLAG_POLISHED_SPROUT_{floor}F_{item}"))
        # The port's Mart already supplies Crystal's consumable Escape Rope.
        native_item = "POLISHED_ESCAPE_ROPE" if item == "ESCAPE_ROPE" else item
        code += f"{action}::\n\tfinditem ITEM_{native_item}, {quantity}\n\tend\n"
    hooks = ""
    if floor == 3:
        hooks = f"\tmap_script MAP_SCRIPT_ON_LOAD, {prefix}_Setup\n"
        coord(header, 9, 9, "VAR_TEMP_4", 0, prefix + "_RivalScene")
        code += summit(script, text, define)
    return hooks, code, {"trainers": [n for n in TRAINERS if f"Sage{n}SeenText:" in script],
        "items": [o["flag"] for o in header["object_events"] if "_ITEM" in o["script"] or "BALL" in o["graphics_id"]],
        "reward_translation": "Source reusable TM70 Flash uses ITEM_TM70; older HM05 saves retain field access. Dark Cave uses source Flash eligibility and lighting rules.",
        "deferred": ["Mako's post-Ecruteak challenge and Rare Candy", "source music"],
        "granny_units": "Imperial source wording, matching the engine's configured default."}


def elder(script, text, define):
    p = "PolishedSproutTower3F_ElderLiScript"
    code = f"""{p}::
    lock
    faceplayer
    goto_if_set FLAG_POLISHED_SPROUT_FLASH, {p}_After
    goto_if_defeated TRAINER_POLISHED_SPROUT_LI, {p}_Gift
    msgbox {p}_Seen, MSGBOX_DEFAULT
    trainerbattle_no_intro TRAINER_POLISHED_SPROUT_LI, {p}_Beaten
{p}_Gift::
    msgbox {p}_Take, MSGBOX_DEFAULT
    giveitem ITEM_TM70
    goto_if_eq VAR_RESULT, FALSE, {p}_Done
    setflag FLAG_POLISHED_SPROUT_FLASH
    msgbox {p}_Explain, MSGBOX_DEFAULT
    goto {p}_Done
{p}_After::
    msgbox {p}_AfterText, MSGBOX_DEFAULT
{p}_Done::
    releaseall
    end
"""
    for suffix, label in (("Seen", "SageLiSeenText"), ("Beaten", "SageLiBeatenText"), ("Take", "SageLiTakeThisFlashText"), ("AfterText", "SageLiAfterBattleText")):
        code += define(p + "_" + suffix, text(script, label))
    explanation = inline_text(script, "ElderLiScript", text)
    code += define(p + "_Explain", explanation)
    return code


def summit(script, text, define):
    p = "PolishedSproutTower3F"
    rival = "LOCALID_SPROUT_3F_1"
    code = f"""{p}_Setup::
    setvar VAR_TEMP_4, 0
    goto_if_unset FLAG_POLISHED_SPROUT_RIVAL_LEFT, {p}_End
    setvar VAR_TEMP_4, 1
{p}_End::
    end
{p}_RivalScene::
    lockall
    turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
    playse SE_M_COMET_PUNCH
    setvar VAR_0x8004, 2
    setvar VAR_0x8005, 2
    setvar VAR_0x8006, 40
    setvar VAR_0x8007, 2
    special ShakeCamera
    waitstate
    delay 15
    playse SE_M_COMET_PUNCH
    special ShakeCamera
    waitstate
    applymovement OBJ_EVENT_ID_PLAYER, {p}_Approach
    waitmovement OBJ_EVENT_ID_PLAYER
    applymovement {rival}, {p}_Up
    waitmovement {rival}
    msgbox {p}_Lecture, MSGBOX_DEFAULT
    closemessage
    turnobject {rival}, DIR_SOUTH
    delay 15
    applymovement {rival}, {p}_Leaves
    waitmovement {rival}
    msgbox {p}_RivalText, MSGBOX_DEFAULT
    closemessage
    turnobject {rival}, DIR_NORTH
    special PolishedErrandBufferRivalName
    msgbox {p}_Escape, MSGBOX_DEFAULT
    closemessage
    playse SE_WARP_IN
    fadescreen FADE_TO_BLACK
    setflag FLAG_POLISHED_SPROUT_RIVAL_LEFT
    removeobject {rival}
    setvar VAR_TEMP_4, 1
    fadescreen FADE_FROM_BLACK
    releaseall
    end
{p}_Approach:
    walk_up
    walk_up
    walk_up
    walk_up
    step_end
{p}_Up:
    walk_up
    step_end
{p}_Leaves:
    walk_right
    walk_down
    step_end
"""
    for suffix, label in (("Lecture", "SproutTowerElderLecturesRivalText"), ("RivalText", "SproutTowerRivalOnlyCareAboutStrongText"), ("Escape", "SproutTowerRivalUsedEscapeRopeText")):
        code += define(p + "_" + suffix, text(script.replace("<RIVAL>", "{STR_VAR_1}"), label))
    return code
