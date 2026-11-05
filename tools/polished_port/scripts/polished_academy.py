"""Earl's tour, Academy lessons and the original illustrated Type Chart."""
from itertools import groupby
import re
import struct
import tempfile
from pathlib import Path

from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png
from new_bark_intro import movement, obj
from polished_cherrygrove import plain_text, standing_sheet

MAPS = {"EarlsPokemonAcademy": "EARLS_POKEMON_ACADEMY"}
ACADEMY = "PolishedEarlsPokemonAcademy"
EARL_LOCAL_ID = "LOCALID_VIOLET_EARL"
DIRECTIONS = {"up": (0,-1), "down": (0,1), "left": (-1,0), "right": (1,0)}


def chart_assets(files, temp):
    raw = files["gfx/type_chart/type_chart.pal"].decode().split("else")[0]
    colors = [int(r) | int(g)<<5 | int(b)<<10 for r,g,b in
              re.findall(r"^\s*RGB\s+(\d+),\s*(\d+),\s*(\d+)", raw, re.M)]
    assert len(colors) == 48
    sheets = []
    for name, dims in (("bg",(160,144)), ("ob",(128,16))):
        temp.write_bytes(files[f"gfx/type_chart/{name}.png"])
        w,h,rows = _read_png_2bit_grayscale(temp)
        assert (w,h)==dims
        sheets.append([[3-pixel for pixel in row] for row in rows])
    bg, sprites = sheets
    attrs = files["gfx/type_chart/type_chart.attrmap"]
    tilemap = files["gfx/type_chart/type_chart.tilemap"]
    assert len(attrs)==len(tilemap)==20*18
    # The source PNG is the complete background before RGBDS deduplicates
    # and flips tiles. Check the tile numbers and flips against that image.
    tiles = {}
    for i,(number,attr) in enumerate(zip(tilemap,attrs)):
        tx,ty = (i%20)*8,(i//20)*8
        tile = tuple(bg[ty+(7-y if attr&64 else y)][tx+(7-x if attr&32 else x)]
                     for y in range(8) for x in range(8))
        assert tiles.setdefault((attr&8,number),tile)==tile
    assert len(tiles)==285

    source = files["engine/items/type_chart.asm"].decode()
    objects = [(int(x),int(y),int(tile,16),int(attr)) for x,y,tile,attr in
               re.findall(r"^\s*oamdata\s+(\d+),\s*(\d+),\s*\$(\w+),\s*(\d+)",source,re.M)]
    assert len(objects)==40 and "and PAD_A | PAD_B" in source
    pixels = [[colors[4*(attrs[(y//8)*20+x//8]&7)+bg[y][x]]
               for x in range(160)] for y in range(144)]
    for y,row in enumerate(pixels):
        # First ten sprites on the GBC scanline. Earlier OAM entries win.
        active = [o for o in objects if o[1]<=y<o[1]+16][:10]
        for x in range(160):
            for left,top,tile,attr in active:
                if not left<=x<left+8:
                    continue
                sx = 7-(x-left) if attr&32 else x-left
                sy = 15-(y-top) if attr&64 else y-top
                color = sprites[sy][(tile//2)*8+sx]
                if color:
                    if not (bg[y][x] and (attr|attrs[(y//8)*20+x//8])&128):
                        row[x]=colors[32+(attr&7)*4+color]
                    break

    # Palette zero is the GBA backdrop. Keep the full 160x144 chart centered.
    palette = sorted({color for row in pixels for color in row})
    indices = {color:i+1 for i,color in enumerate(palette)}
    gfx = [bytes(64)]
    tile_ids = {gfx[0]:0}
    screen = [0]*1024
    for ty in range(18):
        for tx in range(20):
            tile = bytes(indices[pixels[ty*8+y][tx*8+x]] for y in range(8) for x in range(8))
            if tile not in tile_ids:
                tile_ids[tile]=len(gfx)
                gfx.append(tile)
            screen[(ty+1)*32+tx+5]=tile_ids[tile]
    stem = "graphics/polished_type_chart/"
    return {stem+"tiles.bin":b"".join(gfx), stem+"map.bin":struct.pack("<1024H",*screen),
            stem+"palette.bin":struct.pack("<"+"H"*(len(palette)+1),0,*palette)}


def assets(files):
    with tempfile.TemporaryDirectory() as tmp:
        src,out = Path(tmp)/"src.png",Path(tmp)/"out.png"
        products=chart_assets(files,src)
        src.write_bytes(files["gfx/sprites/gamer_girl.png"])
        standing_sheet(src,out,48)
        products["graphics/object_events/polished_gamer_girl/walking.png"]=out.read_bytes()
        src.write_bytes(files["gfx/sprites/book_paper_pokedex.png"])
        w,h,rows=_read_png_2bit_grayscale(src)
        assert (w,h)==(16,48)
        _write_indexed_4bit_png(out,[[0]*16 for _ in range(16)]+[[3-p for p in r] for r in rows[:16]])
        products["graphics/object_events/polished_academy_book/book.png"]=out.read_bytes()
    return products


def tour_movement(script):
    label = "VioletCityFollowEarl_MovementData"
    body = re.search(r"^"+label+r":\n(.*?)^\s*step_end",script,re.M|re.S)[1]
    commands = [line.strip() for line in body.splitlines() if line.strip()]
    assert all(re.fullmatch(r"(run_step|turn_head)_(up|down|left|right)",cmd) for cmd in commands)
    # During a follow, the player occupies Earl's previous square at each step.
    # The first step depends on which side of Earl the player talks from.
    assert commands.pop(0)=="run_step_down"
    x,y,previous = 13,17,"down"
    blocks = []
    code = """    getobjectfacingdirection OBJ_EVENT_ID_PLAYER, VAR_0x8004
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_FirstStep
    switch VAR_0x8004
"""
    for direction, facing in (("up","NORTH"),("down","SOUTH"),("left","WEST"),("right","EAST")):
        code += f"    case DIR_{facing}, PolishedEarl_From_{direction}\n"
    for direction in DIRECTIONS:
        code += (f"PolishedEarl_From_{direction}::\n    applymovement OBJ_EVENT_ID_PLAYER, PolishedEarl_First_{direction}\n"
                 "    goto PolishedEarl_Following\n")
        blocks.append(f"PolishedEarl_First_{direction}:\n    walk_fast_{direction}\n    step_end")
    blocks.append("PolishedEarl_FirstStep:\n    walk_fast_down\n    step_end")
    code += "PolishedEarl_Following::\n    waitmovement OBJ_EVENT_ID_PLAYER\n    waitmovement LOCALID_VIOLET_EARL\n"
    for n,(running,group) in enumerate(groupby(commands,lambda cmd:cmd.startswith("run_step_"))):
        prefix=f"PolishedEarl_Path{n}"
        leader, follower = [], []
        for cmd in group:
            direction=cmd.rsplit("_",1)[1]
            if running:
                leader.append("walk_fast_"+direction)
                follower.append("walk_fast_"+previous)
                dx,dy=DIRECTIONS[direction]
                x,y,previous=x+dx,y+dy,direction
            else:
                leader.append("face_"+direction)
        code+=f"    applymovement {EARL_LOCAL_ID}, {prefix}\n"
        blocks.append(prefix+":\n    "+"\n    ".join(leader+["step_end"]))
        if follower:
            code+=f"    applymovement OBJ_EVENT_ID_PLAYER, {prefix}_Follow\n    waitmovement OBJ_EVENT_ID_PLAYER\n"
            blocks.append(prefix+"_Follow:\n    "+"\n    ".join(follower+["step_end"]))
        code+=f"    waitmovement {EARL_LOCAL_ID}\n"
    assert (x,y,previous)==(30,18,"up")
    return code,"\n\n".join(blocks)+"\n"


def outdoor(script,header,files,text,define):
    earl=obj(script,"VioletCityEarlScript",EARL_LOCAL_ID,"FAT_GUY","FLAG_POLISHED_EARL_IN_ACADEMY")
    earl.update(script="PolishedEarl_Talk",movement_type="MOVEMENT_TYPE_LOOK_AROUND")
    header["object_events"].append(earl)  # Existing Violet residents keep their local IDs.
    tour,moves=tour_movement(script)
    code="""PolishedEarl_Talk::
    lockall
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Spin
    waitmovement LOCALID_VIOLET_EARL
    faceplayer
    msgbox PolishedEarl_Text_EarlAsksIfYouBeatFalkner, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, YES, PolishedEarl_AlreadyWon
    msgbox PolishedEarl_Text_FollowEarl, MSGBOX_DEFAULT
    closemessage
""" + tour + """    turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Spin
    waitmovement LOCALID_VIOLET_EARL
    msgbox PolishedEarl_Text_HereTeacherIAm, MSGBOX_DEFAULT
    call PolishedEarl_GiveChart
    goto_if_eq VAR_RESULT, FALSE, PolishedEarl_EnterAcademy
    msgbox PolishedEarl_Text_EarlsTypeChart, MSGBOX_DEFAULT
PolishedEarl_EnterAcademy::
    closemessage
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Spin
    waitmovement LOCALID_VIOLET_EARL
    applymovement LOCALID_VIOLET_EARL, PolishedEarl_Door
    waitmovement LOCALID_VIOLET_EARL
    playse SE_DOOR
    removeobject LOCALID_VIOLET_EARL
    setflag FLAG_POLISHED_EARL_IN_ACADEMY
    waitse
    releaseall
    end

PolishedEarl_AlreadyWon::
    msgbox PolishedEarl_Text_VeryNiceIndeed, MSGBOX_DEFAULT
    releaseall
    end

PolishedEarl_GiveChart::
    checkitem ITEM_POLISHED_TYPE_CHART
    goto_if_eq VAR_RESULT, TRUE, PolishedEarl_ChartDone
    giveitem ITEM_POLISHED_TYPE_CHART
    goto_if_eq VAR_RESULT, TRUE, PolishedEarl_ChartDone
    msgbox PolishedEarl_FullText, MSGBOX_DEFAULT
    setvar VAR_RESULT, FALSE
PolishedEarl_ChartDone::
    return

PolishedEarl_FullText:
    .string "Make room in your Key Items pocket,\\nthen speak to me at the Academy.$"

PolishedEarl_Door:
    walk_up
    step_end

""" + movement(script,"VioletCitySpinningEarl_MovementData","PolishedEarl_Spin") + moves
    for label in ("Text_EarlAsksIfYouBeatFalkner","Text_FollowEarl","Text_HereTeacherIAm","Text_EarlsTypeChart","Text_VeryNiceIndeed"):
        code+=define("PolishedEarl_"+label,text(script,label))
    blocks=files["maps/VioletCity.ablk"]
    door=blocks[(17//2)*20+30//2]*4+2
    return code,door


def events(script,header,files,text,define):
    prefix=ACADEMY
    code=""
    people=(
        ("AcademyEarl","EARL","FAT_GUY","DOWN","FLAG_TEMP_1"),
        ("AcademyNotebook","NOTEBOOK","ACADEMY_BOOK","DOWN","0"),
        ("EarlsPokemonAcademyYoungster1Text","STUDENT1","SCHOOLBOY_RED","UP","0"),
        ("EarlsPokemonAcademyYoungster2Text","STUDENT2","CHILD","UP","0"),
        ("EarlsPokemonAcademyGameboyKid1Script","GAMER","GAMER_GIRL","DOWN","0"),
        ("EarlsPokemonAcademyGameboyKid2Script","KID","GAMEBOY_KID_RED","LEFT","0"),
    )
    for label,local,gfx,facing,flag in people:
        npc=obj(script,label,"LOCALID_ACADEMY_"+local,gfx,flag,facing)
        npc["script"]=prefix+"_"+label
        header["object_events"].append(npc)
    for x,y,label in re.findall(r"^\s*bg_event\s+(\d+),\s*(\d+), BGEVENT_READ, (\w+)",script,re.M):
        header["bg_events"].append({"type":"sign","x":int(x),"y":int(y),"elevation":0,
            "player_facing_dir":"BG_EVENT_PLAYER_FACING_ANY","script":prefix+"_"+label})
    hooks=f"\tmap_script MAP_SCRIPT_ON_LOAD, {prefix}_Setup\n"
    code+=f"""{prefix}_Setup::
    setflag FLAG_TEMP_1
    goto_if_unset FLAG_POLISHED_EARL_IN_ACADEMY, {prefix}_End
    clearflag FLAG_TEMP_1
{prefix}_End::
    end

{prefix}_AcademyEarl::
    lock
    call PolishedEarl_GiveChart
    applymovement LOCALID_ACADEMY_EARL, {prefix}_Spin
    waitmovement LOCALID_ACADEMY_EARL
    faceplayer
    msgbox {prefix}_AcademyEarlIntroText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, {prefix}_More
    msgbox {prefix}_AcademyEarlTeachHowToWinText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, {prefix}_DoneTeaching
{prefix}_More::
    msgbox {prefix}_AcademyEarlTeachMoreText, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, {prefix}_DoneTeaching
    msgbox {prefix}_AcademyEarlTeachHowToRaiseWellText, MSGBOX_DEFAULT
    goto {prefix}_Release
{prefix}_DoneTeaching::
    msgbox {prefix}_AcademyEarlNoMoreToTeachText, MSGBOX_DEFAULT
{prefix}_Release::
    release
    end

{prefix}_PokemonJournalWalkerScript::
    setflag FLAG_POLISHED_READ_WALKER_JOURNAL
    msgbox {prefix}_JournalText, MSGBOX_SIGN
    end

{prefix}_AcademyBlackboard::
    lockall
    msgbox {prefix}_AcademyBlackboardText, MSGBOX_DEFAULT
{prefix}_BoardMenu::
    multichoicegrid 0, 0, MULTI_POLISHED_BLACKBOARD, 2, FALSE
    switch VAR_RESULT
"""
    subjects=("Poison","Paralysis","Sleep","Burn","Freeze")
    for n,subject in enumerate(subjects):
        code+=f"    case {n}, {prefix}_Board{subject}\n"
    code+="    releaseall\n    end\n\n"
    for subject in subjects:
        code+=f"{prefix}_Board{subject}::\n    msgbox {prefix}_Academy{subject}Text, MSGBOX_DEFAULT\n    goto {prefix}_BoardMenu\n"
    code+=f"\n{prefix}_AcademyNotebook::\n    lockall\n"
    for suffix in ("","1","2"):
        code+=f"    msgbox {prefix}_AcademyNotebookText{suffix}, MSGBOX_YESNO\n    goto_if_eq VAR_RESULT, NO, {prefix}_CloseBook\n"
    code+=f"    msgbox {prefix}_AcademyNotebookText3, MSGBOX_DEFAULT\n{prefix}_CloseBook::\n    releaseall\n    end\n"
    for n in (1,2):
        label=f"EarlsPokemonAcademyYoungster{n}Text"
        code+=f"\n{prefix}_{label}::\n    msgbox {prefix}_{label}_Text, MSGBOX_NPC\n    end\n"
        code+=define(prefix+"_"+label+"_Text",text(script,label))
        label=f"EarlsPokemonAcademyGameboyKid{n}"
        code+=f"\n{prefix}_{label}Script::\n    lock\n    faceplayer\n    msgbox {prefix}_{label}Text, MSGBOX_DEFAULT\n    turnobject VAR_LAST_TALKED, DIR_{'SOUTH' if n==1 else 'WEST'}\n    release\n    end\n"
        code+=define(prefix+"_"+label+"Text",text(script,label+"Text"))
    code+=movement(script,"AcademyEarlSpinMovement",prefix+"_Spin")
    labels=re.findall(r"^(Academy(?:Earl\w+|Blackboard|Poison|Paralysis|Sleep|Burn|Freeze|Notebook)Text\d*):",script,re.M)
    for label in labels:
        code+=define(prefix+"_"+label,text(script,label))
    code+=define(prefix+"_JournalText",plain_text(script,"PokemonJournalWalkerScript",text))
    return hooks, code, {"activities":["Earl's lessons","status blackboard","student notebook","Walker's journal"]}
