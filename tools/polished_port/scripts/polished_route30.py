"""Pinned Route 30 first visit, Mulch and the Mystery Egg/Oak handoff."""
import re
import struct
import tempfile
from pathlib import Path

from polished_route29 import object_at, wild_entries as route29_wild
from new_bark_intro import movement

MAPS = {"Route30": "ROUTE_30", "Route30BerrySpeechHouse": "ROUTE_30_BERRY_SPEECH_HOUSE",
        "MrPokemonsHouse": "MR_POKEMONS_HOUSE"}
GRAPHICS = {
    "YOUNGSTER_BLUE": ("YoungsterBlue", "Youngster", "BLUE"),
    "YOUNGSTER_BROWN": ("YoungsterBrown", "Youngster", "BROWN"),
    "LASS_GREEN": ("LassGreen", "Lass", "GREEN"),
    "BUG_CATCHER": ("BugCatcher", "BugCatcher", "BROWN"),
    "OAK": ("Oak", "Oak", "BROWN"),
    "RATTATA_BACK": ("RattataBack", "RattataBack", "PURPLE"),
    "POKEDEX": ("Pokedex", "Pokedex", "RED"),
    "FRUIT_PINK": ("FruitPink", "Fruit", "PINK"),
    "FRUIT_PICKED_PINK": ("FruitPickedPink", "FruitPicked", "PINK"),
}


def palette(colors):
    colors = [(31,31,31), *colors, (0,0,0)] + [(0,0,0)]*12
    return ("JASC-PAL\n0100\n16\n" + "".join(" ".join(str((c<<3)|(c>>2)) for c in row)+"\n" for row in colors)).replace("\n","\r\n").encode()


def assets(files):
    from build_johto_demo import palette_rows
    from convert_player_walking import convert, _read_png_2bit_grayscale, _write_indexed_4bit_png
    result = {}
    with tempfile.TemporaryDirectory() as tmp:
        src, out = Path(tmp)/"src.png", Path(tmp)/"out.png"
        for sprite in ("oak", "lass", "bug_catcher", "rattata_back", "book_paper_pokedex"):
            src.write_bytes(files[f"gfx/sprites/{sprite}.png"])
            if sprite in ("rattata_back", "book_paper_pokedex"):
                w,h,pixels = _read_png_2bit_grayscale(src)
                assert (w,h) == (16,48)
                # Rattata's back is source down; the Dex is source left.
                y = 0 if sprite == "rattata_back" else 32
                _write_indexed_4bit_png(out, [[0]*16 for _ in range(16)] + [[3-v for v in row] for row in pixels[y:y+16]])
            else:
                convert(src,out)
            result[f"graphics/object_events/polished_{sprite}/walking.png"] = out.read_bytes()
        for item, bank, label in (("mulch","items","MULCH"),("mystery_egg","key_items","MYSTERY EGG")):
            src.write_bytes(files[f"gfx/items/{item}.png"])
            w,h,pixels = _read_png_2bit_grayscale(src)
            assert (w,h) == (24,24)
            _write_indexed_4bit_png(out, [[3-v for v in row] for row in pixels])
            result[f"graphics/items/icons/polished_{item}.png"] = out.read_bytes()
            body = files[f"gfx/items/{bank}.pal"].decode().split("; "+label+"\n")[1]
            colors = [tuple(map(int,v)) for v in re.findall(r"RGB\s+(\d+),\s*(\d+),\s*(\d+)",body)[:2]]
            result[f"graphics/items/icon_palettes/polished_{item}.pal"] = palette(colors)
        for sprite in ("youngster","bug_catcher"):
            src.write_bytes(files[f"gfx/trainers/{sprite}.png"])
            w,h,pixels = _read_png_2bit_grayscale(src)
            assert (w,h) == (56,56)
            canvas = [[0]*64 for _ in range(64)]
            for y,row in enumerate(pixels):
                canvas[y+8][4:60] = [3-v for v in row]
            _write_indexed_4bit_png(out,canvas)
            result[f"graphics/trainers/front_pics/polished_{sprite}.png"] = out.read_bytes()
            colors = [tuple(map(int,v)) for v in re.findall(r"RGB\s+(\d+),\s*(\d+),\s*(\d+)",files[f"gfx/trainers/{sprite}.pal"].decode())]
            assert len(colors)==2
            result[f"graphics/trainers/palettes/polished_{sprite}.pal"] = palette(colors)
    colors = re.findall(r"^\s*ow_npc_pal_const (\w+)",files["constants/sprite_data_constants.asm"].decode(),re.M)
    rows = palette_rows(files["gfx/overworld/npc_sprites.pal"].decode())
    for color in ("PURPLE","PINK"):
        index=colors.index(color);base="graphics/object_events/polished_npc_palettes/"+color.lower()
        result[base+".bin"] = struct.pack("<16H",*rows[20+index],*([0]*12))
        result[base+"_time.bin"] = b"".join(struct.pack("<16H",*rows[t*20+index],*([0]*12)) for t in range(4))
    result["src/data/polished_route30.party"] = trainers(files).encode()
    return result


def trainers(files):
    code="/* Pinned first-visit western Route 30 trainers. */\n"
    dvs=files["data/trainers/dvs.asm"].decode()
    for trainer,cls in (("MIKEY","Youngster"),("DON","Bug Catcher")):
        assert re.search(r"db\s+0, \$66, \$66, \$66, MALE\s+; "+cls.lower(),dvs)
        body=re.search(r'def_trainer '+trainer+r', "(\w+)"(.*?)end_trainer',files["data/trainers/parties.asm"].decode(),re.S)
        code+=f"\n=== TRAINER_POLISHED_{trainer} ===\nName: {body[1]}\nClass: {cls}\nPic: Polished {cls}\nGender: Male\nMusic: Male\nDouble Battle: No\n"
        for level,species in re.findall(r"tr_mon (\d+), (\w+)",body[2]):
            learnset=re.search(r"evos_attacks "+("RattataPlain" if species=="RATTATA" else species.title())+r"\n(.*?)(?=\n\s*evos_attacks |\Z)",files["data/pokemon/evos_attacks.asm"].decode(),re.S)[1]
            moves=[move for lv,move in re.findall(r"learnset (\d+), (\w+)",learnset) if int(lv)<=int(level)][-4:]
            assert moves
            code+=f"\n{species.title()} (M)\nLevel: {level}\nIVs: 12 HP / 12 Atk / 12 Def / 12 SpA / 12 SpD / 12 Spe\n"+"".join("- "+move.replace("_"," ").title()+"\n" for move in moves)
    return code


def wild_entries(files):
    # Both routes have the same seven-slot weights and identical final slots.
    adapted=dict(files)
    adapted["data/wild/johto_grass.asm"] = re.sub(rb"def_grass_wildmons ROUTE_29\n.*?end_grass_wildmons",b"",files["data/wild/johto_grass.asm"],flags=re.S).replace(b"def_grass_wildmons ROUTE_30\n",b"def_grass_wildmons ROUTE_29\n")
    entries=route29_wild(adapted)
    water=re.search(r"def_water_wildmons ROUTE_30\n(.*?)end_water_wildmons",files["data/wild/johto_water.asm"].decode(),re.S)[1]
    assert "2 percent" in water
    slots=[{"min_level":int(lv),"max_level":int(lv),"species":"SPECIES_"+s} for lv,s in re.findall(r"wildmon (\d+), (\w+)",water)]
    for entry in entries:
        entry["map"]="MAP_POLISHED_ROUTE_30"
        entry["base_label"]=entry["base_label"].replace("Route29","Route30")
        entry["water_mons"]={"encounter_rate":4,"mons":[slots[i] for i in (0,1,2,2,2)]}
    return entries


def events(name, script, header, files, text, define):
    p="Polished"+name
    if name=="Route30BerrySpeechHouse":
        header["object_events"].append(object_at("LOCALID_ROUTE30_BERRY_MASTER","POLISHED_POKEFAN_M_BROWN",2,3,p+"_Talk"))
        code=f"""{p}_Talk::
\tlock
\tfaceplayer
\tgoto_if_set FLAG_POLISHED_ROUTE30_MULCH, {p}_After
\tmsgbox {p}_BeforeText, MSGBOX_DEFAULT
\tgiveitem ITEM_POLISHED_MULCH
\tgoto_if_eq VAR_RESULT, FALSE, {p}_Done
\tsetflag FLAG_POLISHED_ROUTE30_MULCH
{p}_After::
\tmsgbox {p}_AfterText, MSGBOX_DEFAULT
{p}_Done::
\trelease
\tend

"""
        code+=define(p+"_BeforeText",text(script,"Route30BerrySpeechHouseMonEatBerriesText"))
        body=script[script.index('\ttext "I buy top-quality"'):].split("\tdone")[0]+"\tdone\n"
        code+=define(p+"_AfterText",text("Selected:\n"+body,"Selected"))
        return "",code,{"gift":"Mulch, committed only after Bag insertion; replenishes one picked tree"}
    if name=="MrPokemonsHouse":
        return mr_pokemon(script,header,text,define)
    header["connections"]=[{"map":"MAP_POLISHED_CHERRYGROVE_CITY","offset":-10,"direction":"down"}]
    code=""
    for local,gfx,x,y,label,facing in (
        ("JOEY","YOUNGSTER_BLUE",5,26,"Joey","UP"),
        ("PIDGEY","PIDGEY",5,24,"Pidgey","DOWN"),
        ("RATTATA","RATTATA_BACK",5,25,"Silent","DOWN"),
        ("MIKEY","YOUNGSTER_BLUE",5,23,"Mikey","DOWN"),
        ("DON","BUG_CATCHER",1,7,"Don","DOWN"),
        ("YOUNGSTER","YOUNGSTER_BROWN",7,30,"Youngster","DOWN"),
        ("LASS","LASS_GREEN",2,13,"Lass","DOWN")):
        o=object_at("LOCALID_ROUTE30_"+local,"POLISHED_"+gfx,x,y,p+"_"+label)
        o["movement_type"]="MOVEMENT_TYPE_FACE_"+facing
        if local=="YOUNGSTER":
            o["movement_type"]="MOVEMENT_TYPE_WANDER_LEFT_AND_RIGHT";o["movement_range_x"]=1
        if local in ("MIKEY","DON"):
            o["trainer_type"]="TRAINER_TYPE_NORMAL";o["trainer_sight_or_berry_tree_id"]="1" if local=="MIKEY" else "3"
        header["object_events"].append(o)
    for local,gfx,x,y,label,flag in (
        ("CUT","POLISHED_CUT_TREE",8,6,"EventScript_CutTree","FLAG_POLISHED_ROUTE30_CUT"),
        ("ORAN","VAR_E",10,39,p+"_Oran","0"),("PECHA","VAR_D",11,5,p+"_Pecha","0"),
        ("ANTIDOTE","POLISHED_BALL_RED",8,35,p+"_Antidote","FLAG_POLISHED_ROUTE30_ANTIDOTE")):
        header["object_events"].append(object_at("LOCALID_ROUTE30_"+local,gfx,x,y,label,flag))
    header["bg_events"].append({"type":"hidden_item","x":14,"y":9,"elevation":0,"item":"ITEM_POTION","flag":"FLAG_POLISHED_ROUTE30_HIDDEN_POTION"})
    code+=f"""{p}_Setup::
\tspecial PolishedFruitRefreshAll
\tend
{p}_Silent::
\tend
{p}_Pidgey::
\tend
{p}_Antidote::
\tfinditem ITEM_ANTIDOTE
\tend
{p}_Oran::
\tsetvar VAR_0x8005, 1
\tgoto PolishedFruit_Interact
{p}_Pecha::
\tsetvar VAR_0x8005, 2
\tgoto PolishedFruit_Interact
{p}_Joey::
\tlockall
\tmsgbox {p}_UseTackleText, MSGBOX_DEFAULT
\tclosemessage
\tplayse SE_M_COMET_PUNCH
\tapplymovement LOCALID_ROUTE30_RATTATA, {p}_RattataAttack
\twaitmovement 0
\tfaceplayer
\tmsgbox {p}_BigBattleText, MSGBOX_DEFAULT
\tclosemessage
\tturnobject LOCALID_ROUTE30_JOEY, DIR_NORTH
\tplayse SE_M_COMET_PUNCH
\tapplymovement LOCALID_ROUTE30_PIDGEY, {p}_PidgeyAttack
\twaitmovement 0
\treleaseall
\tend
{p}_RattataAttack:
\twalk_fast_up
\twalk_fast_down
\tstep_end
{p}_PidgeyAttack:
\twalk_fast_down
\twalk_fast_up
\tstep_end
"""
    for key,label in (("UseTackle","Text_UseTackle"),("BigBattle","Text_ThisIsABigBattle"),("Youngster","Route30YoungsterText_DirectionsToMrPokemonsHouse"),("Lass","Route30CooltrainerFText")):
        if key in ("Youngster","Lass"):
            code+=f"{p}_{key}::\n\tmsgbox {p}_{key}Text, MSGBOX_NPC\n\tend\n"
        code+=define(p+"_"+key+"Text",text(script,label))
    from polished_cherrygrove import plain_text
    for who,source in (("Mikey","YoungsterMikey"),("Don","Bug_catcherDon")):
        code+=f"{p}_{who}::\n\ttrainerbattle_single TRAINER_POLISHED_{who.upper()}, {p}_{who}Seen, {p}_{who}Beaten\n\tmsgbox {p}_{who}After, MSGBOX_AUTOCLOSE\n\tend\n"
        for suffix in ("Seen","Beaten"):
            code+=define(p+"_"+who+suffix,text(script,source+suffix+"Text"))
        code+=define(p+"_"+who+"After",plain_text(script,"GenericTrainer"+source,text))
    return (f"\tmap_script MAP_SCRIPT_ON_LOAD, {p}_Setup\n\tmap_script MAP_SCRIPT_ON_RETURN_TO_FIELD, {p}_Setup\n",code,
        {"first_visit":"Joey's battle barrier, original residents, Mikey/Don, six signs, Antidote, hidden Potion, Cut and two daily fruit trees",
         "deferred":["Route31 north connection","post-errand Joey battle and phone rematches","fishing","source music"]})


def mr_pokemon(script,header,text,define):
    p="PolishedMrPokemonsHouse"
    for local,gfx,x,y,face,flag in (("MR_POKEMON","GENTLEMAN",3,5,"RIGHT","0"),("OAK","OAK",6,5,"UP","FLAG_TEMP_2"),("POKEDEX","POKEDEX",4,4,"LEFT","FLAG_TEMP_3")):
        o=object_at("LOCALID_"+local,"POLISHED_"+gfx,x,y,p+"_Talk" if local=="MR_POKEMON" else p+"_Silent",flag)
        o["movement_type"]="MOVEMENT_TYPE_FACE_"+face;header["object_events"].append(o)
    header["bg_events"].append({"type":"sign","x":3,"y":1,"elevation":0,"player_facing_dir":"BG_EVENT_PLAYER_FACING_ANY","script":p+"_Cabinet"})
    hooks=f"\tmap_script MAP_SCRIPT_ON_LOAD, {p}_Setup\n\tmap_script MAP_SCRIPT_ON_RETURN_TO_FIELD, {p}_Restore\n\tmap_script MAP_SCRIPT_ON_FRAME_TABLE, {p}_Frame\n"
    code=f"""{p}_Setup::
\tclearflag FLAG_TEMP_2
\tclearflag FLAG_TEMP_3
\tgoto_if_ne VAR_POLISHED_MR_POKEMON_SCENE, 1, {p}_End
\tsetflag FLAG_TEMP_2
\tsetflag FLAG_TEMP_3
{p}_End::
\tend
{p}_Restore::
\tgoto_if_ne VAR_POLISHED_MR_POKEMON_SCENE, 1, {p}_End
\tremoveobject LOCALID_OAK
\tremoveobject LOCALID_POKEDEX
\tend
{p}_Frame::
\tmap_script_2 VAR_TEMP_1, 0, {p}_CheckIntro
\t.2byte 0
{p}_CheckIntro::
\tsetvar VAR_TEMP_1, 1
\tgoto_if_ne VAR_POLISHED_MR_POKEMON_SCENE, 0, {p}_End
\tlockall
\tapplymovement LOCALID_MR_POKEMON, PolishedIntro_Exclamation
\twaitmovement 0
\tturnobject LOCALID_MR_POKEMON, DIR_SOUTH
\tmsgbox {p}_MrPokemonIntroText1, MSGBOX_DEFAULT
\tclosemessage
\tapplymovement OBJ_EVENT_ID_PLAYER, {p}_PlayerApproach
\twaitmovement 0
\tturnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
\tmsgbox {p}_MrPokemonIntroText2, MSGBOX_DEFAULT
\tgiveitem ITEM_POLISHED_MYSTERY_EGG
\tgoto_if_eq VAR_RESULT, FALSE, {p}_Full
\tsetrespawn HEAL_LOCATION_POLISHED_CHERRYGROVE
\tmsgbox {p}_MrPokemonIntroText3, MSGBOX_DEFAULT
\tturnobject LOCALID_MR_POKEMON, DIR_EAST
\tmsgbox {p}_MrPokemonIntroText4, MSGBOX_DEFAULT
\tturnobject LOCALID_MR_POKEMON, DIR_SOUTH
\tturnobject LOCALID_OAK, DIR_WEST
\tmsgbox {p}_MrPokemonIntroText5, MSGBOX_DEFAULT
\tclosemessage
\tapplymovement LOCALID_OAK, {p}_OakApproach
\twaitmovement 0
\tturnobject OBJ_EVENT_ID_PLAYER, DIR_EAST
\tmsgbox {p}_MrPokemonsHouse_OakText1, MSGBOX_DEFAULT
\tclosemessage
\tturnobject LOCALID_OAK, DIR_NORTH
\tdelay 10
\tapplymovement LOCALID_POKEDEX, {p}_TakeDex
\twaitmovement 0
\tremoveobject LOCALID_POKEDEX
\tsetflag FLAG_TEMP_3
\tdelay 10
\tturnobject LOCALID_OAK, DIR_WEST
\tdelay 10
\tsetflag FLAG_SYS_POKEDEX_GET
\tspecial EnableNationalPokedex
\tplayfanfare MUS_OBTAIN_ITEM
\tmsgbox {p}_ReceivedDex, MSGBOX_DEFAULT
\twaitfanfare
\tmsgbox {p}_MrPokemonsHouse_OakText2, MSGBOX_DEFAULT
\tclosemessage
\tturnobject OBJ_EVENT_ID_PLAYER, DIR_SOUTH
\tapplymovement LOCALID_OAK, {p}_OakExit
\twaitmovement 0
\tplayse SE_DOOR
\tremoveobject LOCALID_OAK
\tsetflag FLAG_TEMP_2
\tdelay 15
\tturnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
\tmsgbox {p}_MrPokemonsHouse_MrPokemonHealText, MSGBOX_DEFAULT
\tclosemessage
\tfadescreen FADE_TO_BLACK
\tplayfanfare MUS_HEAL
\tspecial HealPlayerParty
\twaitfanfare
\tfadescreen FADE_FROM_BLACK
\tmsgbox {p}_MrPokemonText_ImDependingOnYou, MSGBOX_DEFAULT
\tsetflag FLAG_HIDE_POLISHED_NEW_BARK_RIVAL
\tsetvar VAR_POLISHED_MR_POKEMON_SCENE, 1
@ Stage 1 records the stolen-starter/Elm-call handoff for the return phase.
\tsetvar VAR_POLISHED_ERRAND_STATE, 1
\treleaseall
\tend
{p}_Full::
\tmsgbox {p}_MakeRoom, MSGBOX_DEFAULT
\treleaseall
\tend
{p}_Talk::
\tlock
\tfaceplayer
\tgoto_if_eq VAR_POLISHED_MR_POKEMON_SCENE, 0, {p}_FullTalk
\tmsgbox {p}_MrPokemonText_ImDependingOnYou, MSGBOX_DEFAULT
\trelease
\tend
{p}_FullTalk::
\tmsgbox {p}_MakeRoom, MSGBOX_DEFAULT
\trelease
\tend
{p}_Silent::
\tend
{p}_Cabinet::
\tmsgbox {p}_MrPokemonsHouse_CabinetText, MSGBOX_SIGN
\tend
{p}_PlayerApproach:
@ The first-frame scene runs before the native doorway step.
\twalk_right
\twalk_up
\tstep_end
{p}_TakeDex:
\tlock_facing_direction
\twalk_down
\tunlock_facing_direction
\tstep_end
{p}_OakExit:
\twalk_down
\twalk_left
\tface_down
\tdelay_2
\tstep_end
"""
    code+=movement(script,"MrPokemonsHouse_OakWalksToPlayer",p+"_OakApproach")
    for label in ["MrPokemonIntroText"+str(i) for i in range(1,6)]+["MrPokemonsHouse_OakText1","MrPokemonsHouse_OakText2","MrPokemonsHouse_MrPokemonHealText","MrPokemonText_ImDependingOnYou","MrPokemonsHouse_CabinetText"]:
        code+=define(p+"_"+label,text(script,label))
    code+=define(p+"_ReceivedDex","{PLAYER} received the POKéDEX!$")
    code+=define(p+"_MakeRoom","Please make room for the Egg\\nand visit me again.$")
    return hooks,code,{"source_scene":0,"next_scene":1,"gift":"Mystery Egg key item",
        "pokedex":"native National Pokédex adapter; original Johto ordering/UI pending",
        "return_handoff":"VAR_POLISHED_ERRAND_STATE=1; return rival, Elm call/theft investigation and Egg delivery are next"}
