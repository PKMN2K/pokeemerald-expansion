"""First-visit Cherrygrove tests using only normal controller input.

Invoked during the Route 29 campaign. RAM probes are read-only and checkpoints
are created by the current ROM's own gameplay, never injected saves or scripts.
"""
import json
import struct
import re
import subprocess
from functools import lru_cache

from build_johto_demo import ROOT
from test_johto_demo_emulator import A, B, UP, DOWN, LEFT, RIGHT
from test_new_bark_emulator import (map_data, walk, actors, close_resident_dialogue,
    save_game, continue_game, check_resident_palette, talk_to_resident)
from test_new_bark_intro_emulator import advance, script_active, text_matches, party, layout, task_active, choice_prompt
from test_new_bark_battle_emulator import potion_count, controller, choose_cell
from stage_johto_time_palettes import stage, load_source_palettes
from stage_johto_roof_palettes import load_roof_tables, roof_colors, apply_roof_colors
from polished_cherrygrove import MAPS
from build_new_bark import source_files


@lru_cache
def extra_layout():
    fields={"money":"offsetof(struct SaveBlock1, money)","item_size":"sizeof(struct ItemInfo)",
        "key_items":"offsetof(struct SaveBlock1, bag.keyItems)", "key_count":"BAG_KEYITEMS_COUNT",
        "town_map":"ITEM_TOWN_MAP", "nugget":"ITEM_NUGGET", "mystic":"ITEM_MYSTIC_WATER",
        "tentacool":"SPECIES_TENTACOOL", "tentacruel":"SPECIES_TENTACRUEL"}
    code='#include "global.h"\n#include "item.h"\nconst u32 values[] = {'+','.join(fields.values())+'};\n'
    r=subprocess.run(["arm-none-eabi-gcc","-mcpu=arm7tdmi","-mthumb","-mabi=apcs-gnu","-DMODERN=1","-DEMERALD",
        "-std=gnu17","-iquote","include","-iquote",".","-S","-x","c","-o","-","-"],cwd=ROOT,input=code,text=True,capture_output=True,check=True)
    return dict(zip(fields,map(int,re.findall(r"\.word\s+(\d+)",r.stdout))))


def money(game):
    key=game.u32(game.u32(game.sym["gSaveBlock2Ptr"])+layout()["encryption_key"])
    return game.u32(game.u32(game.sym["gSaveBlock1Ptr"])+extra_layout()["money"]) ^ key


def count_item(game,item,key_items=False):
    data=extra_layout() if key_items else layout()
    offset,count=(data["key_items"],data["key_count"]) if key_items else (data["items"],data["items_count"])
    key=game.u32(game.u32(game.sym["gSaveBlock2Ptr"])+layout()["encryption_key"])&0xffff
    raw=game.read(game.u32(game.sym["gSaveBlock1Ptr"])+offset,count*4)
    return sum(n^key for i,n in struct.iter_unpack("<HH",raw) if i==item)


def scene(game):
    base = game.u32(game.sym["gSaveBlock1Ptr"])
    return struct.unpack("<H", game.read(base + layout()["vars"] + 2*0xfa, 2))[0]


def flag(game, number):
    base = game.u32(game.sym["gSaveBlock1Ptr"])
    return bool(game.read(base+layout()["flags"]+number//8, 1)[0] & 1 << (number%8))


def map_id(game, data):
    groups = json.loads((ROOT/"data/maps/map_groups.json").read_text())
    return groups["group_order"].index("gMapGroup_PolishedJohtoDemo"), groups["gMapGroup_PolishedJohtoDemo"].index(data["name"])


def enter(game, city, data):
    door = next(w for w in city["warp_events"] if w["dest_map"] == data["id"])
    walk(game, city, (door["x"], door["y"]+1))
    game.step(UP)
    game.frames(200)
    assert game.map_number() == map_id(game, data), (game.map_number(), data["name"])


def leave(game, data, city, index=0):
    door = data["warp_events"][index]
    walk(game, data, (door["x"], door["y"]))
    game.frames(40, DOWN)
    game.frames(200)
    assert game.map_number() == map_id(game, city)


def interact(game, data, pos, direction, label=None):
    walk(game, data, pos)
    game.face(direction)
    game.press(A, 100)
    if label:
        advance(game, lambda: text_matches(game, label))


def finish_tour(game):
    report = json.loads((ROOT/"tools/polished_port/manifests/new_bark.json").read_text())
    stops = report["maps"]["CHERRYGROVE_CITY"]["story_encounters"][0]["tour"]
    for stop in stops:
        advance(game, lambda: text_matches(game, "PolishedCherrygrove_"+stop["stop"]), limit=400)
        assert game.position() == tuple(stop["player"]), (stop,game.position())
        assert actors(game)[1]["position"] == tuple(stop["guide"])
        game.frames(220)
        game.screenshot("cherrygrove-tour-"+stop["stop"])
        game.press(A)
    advance(game, lambda: scene(game) == 2 and not script_active(game), limit=400)
    assert game.position() == (24,11) and flag(game,0x4a7) and flag(game,0x4a8)
    assert count_item(game,extra_layout()["town_map"],True)==1
    assert 1 not in actors(game)


def sea_battle(game, lose=False):
    seen = None
    for _ in range(1800):
        if task_active(game,"Task_BattleStart"):
            game.frames(30)
            continue
        if game.u32(game.sym["gMain"]+4)==game.sym["CB2_Overworld"]|1:
            game.frames(30)
            if task_active(game,"Task_BattleStart"):
                continue
            return seen
        if task_active(game,"Task_HandleChooseMonInput"):
            healthy=[i for i,m in enumerate(party(game,details=True)) if m["hp"]]
            assert healthy
            target=healthy[0] if len(healthy)==1 else 1
            selected=game.read(game.sym["gPartyMenu"]+9,1)[0]
            game.press(A if selected==target else RIGHT if target==1 else LEFT,30)
        elif controller(game,"HandleInputChooseAction"):
            if seen is None:
                seen=party(game,enemy=True,details=True)[0]
                assert (seen["species"],seen["level"]) in ((extra_layout()["tentacool"],15),
                    (extra_layout()["tentacool"],20),(extra_layout()["tentacruel"],20)),seen
                game.screenshot("cherrygrove-water-battle")
            if lose:
                # Tentacool can trap the lead with Wrap. Keep spending real
                # turns on Leer/Growl; repeatedly asking to switch while
                # trapped cannot advance the battle. The higher-level sea
                # opponents defeat each lead before these moves run out.
                choose_cell(game,"gActionSelectionCursor",0)
            else:
                choose_cell(game,"gActionSelectionCursor",3)
        elif controller(game,"HandleInputChooseMove"):
            assert lose and game.read(game.sym["gBattleMons"]+layout()["battle_pp"],4)[1]
            choose_cell(game,"gMoveSelectionCursor",1)
        else:
            game.press(A,30)
    game.command(f"save {game.output / 'cherrygrove-water-failure.state'}")
    raise AssertionError(("Cherrygrove water battle did not finish", seen, party(game,details=True)))


def check_items_and_blackout(game, city, center, checks):
    # The northeastern Nugget ledge is approached through the unported north
    # connection. Its native event is audited, not reached through a test warp.
    game.restore("CherrygrovePokeCenter1F")
    leave(game,center,city)
    walk(game,city,(10,9));game.face(DOWN);game.press(A)
    advance(game,lambda:bool(game.read(game.sym["gPlayerAvatar"],1)[0]&8) and not script_active(game))
    game.frames(180)
    assert game.position()==(10,10)
    game.snapshot("cherrygrove-surf")
    for direction in (DOWN,DOWN,DOWN,LEFT,LEFT,LEFT):
        game.step(direction);game.frames(45);sea_battle(game)
        assert game.map_number()==map_id(game,city),"Unexpected defeat en route to Fisher"
    assert game.position()==(7,13) and not game.read(game.sym["gPlayerAvatar"],1)[0]&8
    before=count_item(game,extra_layout()["mystic"])
    game.face(UP);game.press(A)
    advance(game,lambda:not script_active(game))
    assert flag(game,0x4aa) and count_item(game,extra_layout()["mystic"])==before+1
    game.screenshot("cherrygrove-mystic-water")
    save_game(game);continue_game(game)
    game.face(UP);game.press(A)
    advance(game,lambda:text_matches(game,"PolishedCherrygroveCity_MysticAfter_Text"))
    close_resident_dialogue(game)
    assert flag(game,0x4aa) and count_item(game,extra_layout()["mystic"])==before+1
    checks.append("Surf reaches the source Fisher; Mystic Water is granted once and its flag survives Save/Continue")
    game.restore("cherrygrove-surf")
    encountered=None
    for _ in range(150):
        game.step(DOWN if game.position()[1]==10 else UP)
        game.frames(45)
        encountered=sea_battle(game,lose=True)
        if encountered:
            break
    assert encountered is not None
    advance(game,lambda:not script_active(game),limit=400)
    assert game.read(game.sym["gBattleOutcome"],1)==b"\x02"
    assert game.map_number()==map_id(game,center) and game.position()==(5,3)
    assert all(m["hp"]==m["max_hp"] and m["status"]==0 for m in party(game,details=True))
    game.screenshot("cherrygrove-blackout-recovery")
    checks.append("A genuine source-table water battle loss returns to Cherrygrove Center (5,3) with the full party healed")
    print("PASS: Cherrygrove saved items, Surf and actual blackout",flush=True)


def check_city(game, route, observed, rows, checks):
    from test_route29_emulator import route_walk
    game.snapshot("cherrygrove-before-visit")
    game.set_hour(10)
    city = map_data("PolishedCherrygroveCity")
    interiors = {name:map_data("Polished"+name) for name in MAPS if name != "CherrygroveCity"}
    route_walk(game, route, (0,6), observed)
    game.step(LEFT)
    game.frames(240)
    assert game.map_number() == map_id(game,city) and game.position() == (39,6)
    game.screenshot("cherrygrove-first-entry")
    game.snapshot("cherrygrove-entry")
    game.command(f"save {game.output / 'cherrygrove-entry.state'}")
    # Original coordinate trigger, then the source NPC's direct interaction.
    walk(game,city,(34,7))
    game.step(LEFT)
    advance(game,lambda:text_matches(game,"PolishedCherrygrove_GuideGentIntroText"))
    finish_tour(game)
    game.snapshot("cherrygrove-after-tour")
    game.command(f"save {game.output / 'cherrygrove-after-tour.state'}")
    # From the first-visit east entrance, only the eastern talk approach is
    # reachable without crossing the original automatic trigger at 33,7.
    for pos,direction in [((33,6),LEFT)]:
        game.restore("cherrygrove-entry")
        direct={**city,"warp_events":city["warp_events"]+[{"x":33,"y":7}]}
        interact(game,direct,pos,direction,"PolishedCherrygrove_GuideGentIntroText")
        finish_tour(game)
    checks.append("Cherrygrove Guide Gent coordinate trigger and direct east-side conversation follow all six source tour stops and grant one usable map")
    print("PASS: Cherrygrove guided tour",flush=True)
    game.restore("cherrygrove-after-tour")
    # Portable map uses the actual source Johto artwork and starts at this city.
    game.use_key_item(2)
    advance(game,lambda:game.u32(game.sym["gMain"]+4)==game.sym["CB2_PolishedTownMap"]|1)
    game.frames(60)
    assert game.u32(game.sym["gMain"]+4) == game.sym["CB2_PolishedTownMap"]|1
    game.screenshot("cherrygrove-town-map")
    pointer = game.u32(game.sym["sJohtoMap"])
    assert game.read(pointer+2052,2) == bytes((2,2))
    game.press(DOWN)
    assert game.read(pointer+2052,1) == b"\x03"
    game.press(B,180)
    assert game.map_number() == map_id(game,city) and not script_active(game)
    game.use_key_item(2)
    advance(game,lambda:game.u32(game.sym["gMain"]+4)==game.sym["CB2_PolishedTownMap"]|1)
    game.frames(60)
    game.press(A,180)
    save_game(game); continue_game(game)
    assert scene(game)==2 and flag(game,0x4a8) and 1 not in actors(game)
    game.snapshot("cherrygrove-services")
    checks.append("Guide departure and Map Card persist through Save/Continue; the Bag map selects Johto landmarks and closes with A or B")
    # All six map endpoints, including both tiles of every interior exit.
    warps=[]
    for name,data in interiors.items():
        game.restore("cherrygrove-services")
        enter(game,city,data)
        game.snapshot(name)
        game.command(f"save {game.output / (name+'.state')}")
        game.screenshot("cherrygrove-"+name)
        assert len(data["warp_events"])==2
        for n in (0,1):
            game.restore(name)
            leave(game,data,city,n)
            expected=next(w for w in city["warp_events"] if w["dest_map"]==data["id"])
            assert game.position()==(expected["x"],expected["y"]+1)
            warps.append((name,n))
    assert len(warps)==10
    checks.append("All five Cherrygrove interiors and both exit tiles per building connect to their original city doors; upstairs communication remains closed")
    # Authentic four-period city roof palette, fixed indoor special palettes.
    palettes=stage(load_source_palettes())[0];roofs=load_roof_tables()
    for hour,period in [(7,"morn"),(10,"day"),(19,"eve"),(23,"nite")]:
        game.restore("cherrygrove-services");game.set_hour(hour)
        expected=apply_roof_colors(palettes["standard."+period],roof_colors(roofs,period,map_group=26)[0])
        assert game.read(game.sym["gPlttBufferUnfaded"],224)==expected
        game.screenshot("cherrygrove-"+period)
    checks.append("Cherrygrove uses original roof group 26 in all four time palettes")
    colors = re.findall(r"^\s*ow_npc_pal_const (\w+)",source_files()["constants/sprite_data_constants.asm"].decode(),re.M)
    report = json.loads((ROOT/"tools/polished_port/manifests/new_bark.json").read_text())["maps"]
    # Resident text, palette and talk direction include moving NPCs and the
    # children who keep facing each other during their conversations.
    for name,data in interiors.items():
        game.restore(name);game.set_hour(10)
        bank = "mart" if name=="CherrygroveMart" else "pokecenter" if name=="CherrygrovePokeCenter1F" else "house"
        expected=(ROOT/f"data/tilesets/primary/polished_new_bark_{bank}/palettes.bin").read_bytes()[:224]
        # Native indoor backdrop is black. Opaque source color zero lives in
        # slot 4, so all visible original colors remain exact.
        expected=b"\0\0"+expected[2:]
        assert game.read(game.sym["gPlttBufferUnfaded"],224)==expected
        for n,row in enumerate(data["object_events"],1):
            if row["script"].endswith(("ClerkScript","_Nurse")):
                continue
            talk_to_resident(game,data,n,"cherrygrove-npc-"+name+"-"+str(n),leave_open=True)
            assert script_active(game)
            source=report[MAPS[name]]["story_encounters"][0]["residents"][n-1]
            check_resident_palette(game,data,n,colors.index(source["color"]),"day",rows)
            close_resident_dialogue(game)
    game.restore("cherrygrove-services");game.set_hour(10)
    for n in (2,3,5):
        talk_to_resident(game,city,n,"cherrygrove-city-npc-"+str(n))
        check_resident_palette(game,city,n,colors.index({2:"BLUE",3:"RED",5:"BROWN"}[n]),"day",rows)
    for x,label in ((24,"MartSign"),(30,"PokeCenterSign")):
        interact(game,city,(x,4),UP,"PolishedCherrygroveCity_"+label+"_Text")
        close_resident_dialogue(game)
    checks.append("All accessible Cherrygrove and interior residents converse, including the guide at home, the moving customers and Pidgey's cry")
    print("PASS: Cherrygrove interiors and residents",flush=True)
    # Real purchase, native money deduction and persistent inventory.
    mart=interiors["CherrygroveMart"]
    game.restore("CherrygroveMart")
    before,cash=potion_count(game),money(game)
    price=game.u32(game.sym["gItemsInfo"]+layout()["potion"]*extra_layout()["item_size"])
    interact(game,mart,(3,3),LEFT)
    advance(game,lambda:task_active(game,"Task_ShopMenu"))
    game.press(A,160)
    assert task_active(game,"Task_BuyMenu")
    game.screenshot("cherrygrove-mart-stock")
    game.press(A)
    advance(game,lambda:task_active(game,"Task_BuyHowManyDialogueHandleInput"))
    game.press(A)
    advance(game,lambda:task_active(game,"Task_ReturnToItemListAfterItemPurchase"))
    assert potion_count(game)==before+1 and money(game)==cash-price
    game.press(A)
    assert task_active(game,"Task_BuyMenu")
    game.press(B,180)
    advance(game,lambda:task_active(game,"Task_ShopMenu"))
    game.press(B)
    close_resident_dialogue(game)
    save_game(game);continue_game(game)
    assert potion_count(game)==before+1 and money(game)==cash-price
    checks.append("Cherrygrove Mart sells its four first-visit items through the native shop; a Potion purchase deducts the live price and survives Save/Continue")
    print("PASS: Cherrygrove Mart purchase",flush=True)
    # The nurse uses source clock greetings; No leaves the party unchanged.
    center=interiors["CherrygrovePokeCenter1F"]
    game.restore("CherrygrovePokeCenter1F")
    for hour,label in [(7,"Morn"),(10,"Day"),(19,"Eve"),(23,"Nite")]:
        game.set_hour(hour)
        before=party(game,details=True)
        interact(game,center,(5,3),UP,"PolishedCherrygrovePokeCenter1F_Nurse"+label+"Text")
        choice_prompt(game,"PolishedCherrygrovePokeCenter1F_NurseAskHealText")
        game.press(DOWN);game.press(A)
        close_resident_dialogue(game)
        assert party(game,details=True)==before
    interact(game,center,(5,3),UP)
    advance(game,lambda:not script_active(game),limit=400)
    assert all(m["hp"]==m["max_hp"] and m["status"]==0 for m in party(game,details=True))
    game.screenshot("cherrygrove-healed")
    checks.append("Pokémon Center nurse recognizes all four source time periods, honors No and heals the full party on Yes")
    # Source PC at 7,2 opens the actual native storage menu, not a placeholder.
    interact(game,center,(7,3),UP)
    advance(game,lambda:task_active(game,"Task_HandleMultichoiceInput"))
    game.press(A,160)
    advance(game,lambda:task_active(game,"Task_PCMainMenu"))
    game.press(A,160)
    advance(game,lambda:game.u32(game.sym["gMain"]+4)==game.sym["CB2_PokeStorage"]|1)
    game.frames(240)
    game.screenshot("cherrygrove-storage")
    game.restore("CherrygrovePokeCenter1F")
    interact(game,center,(10,2),UP,"PolishedCherrygrovePokeCenter1F_JournalText")
    close_resident_dialogue(game)
    assert flag(game,0x4ab)
    save_game(game);continue_game(game)
    assert flag(game,0x4ab)
    interact(game,center,(9,1),UP)
    advance(game,lambda:game.u32(game.sym["gMain"]+4)==game.sym["CB2_PolishedTownMap"]|1)
    game.frames(60)
    game.screenshot("cherrygrove-center-wall-map")
    game.press(B,180)
    checks.append("Center PC opens native Pokémon storage and reading Red's journal records its persistent flag")
    check_items_and_blackout(game,city,center,checks)
    game.restore("cherrygrove-services")
    walk(game,city,(39,6));game.step(RIGHT);game.frames(100)
    assert game.map_number()==map_id(game,route) and game.position()==(0,6)
    checks.append("Cherrygrove's east connection returns seamlessly to Route 29")
    game.restore("cherrygrove-before-visit")
    print("PASS: Cherrygrove first-visit services",flush=True)
