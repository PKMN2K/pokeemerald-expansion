"""Dark Cave and Falkner through real controller input; RAM probes are read-only."""
from collections import deque
from functools import lru_cache
import json
import re
import struct
import subprocess

from build_new_bark import ROOT, source_files
from test_johto_demo_emulator import A, B, UP, DOWN, LEFT, RIGHT
from test_new_bark_emulator import map_data, actors, save_game, continue_game, walk as indoor_walk
from test_new_bark_intro_emulator import layout, advance, task_active, text_matches, party, script_active, choice_prompt
from test_new_bark_battle_emulator import controller, choose_cell, check_portrait
from test_cherrygrove_emulator import map_id, flag, leave, interact, money
from test_violet_houses_emulator import enter, walk_city
from test_violet_mart_emulator import count, select_bag_item, open_shop, purchase, close_shop, stock
from test_academy_emulator import wait_for
from test_route31_emulator import enter as transition
from test_sprout_emulator import constants as sprout_constants, heal

TRAINERS = ('Rod', 'Abe', 'Falkner')
DIRECTIONS = ((UP, 0, -1), (LEFT, -1, 0), (RIGHT, 1, 0), (DOWN, 0, 1))


@lru_cache
def constants():
    names = sorted(set(re.findall(r'\b(?:MOVE|TRAINER)_\w+', (ROOT / 'src/data/polished_falkner.party').read_text())))
    names += ['SPECIES_' + n for n in ('GEODUDE','ZUBAT','TEDDIURSA','DUNSPARCE','NATU','HOOTHOOT','PIDGEOTTO','PIDGEY','SPEAROW','TOGEPI')]
    for area in ('ROUTE_31', 'DARK_CAVE_VIOLET_ENTRANCE'):
        body = source_files()['data/wild/johto_grass.asm'].decode().split('def_grass_wildmons ' + area + '\n')[1].split('end_grass_wildmons')[0]
        names += ['SPECIES_' + sp for sp in re.findall(r'wildmon \d+, (\w+)', body)]
    names += ['ITEM_TM_ROOST','ITEM_PECHA_BERRY','FLAG_BADGE01_GET','FLAG_BADGE05_GET','FLAG_TEMP_1','FLAG_TEMP_2','FLAG_TEMP_3','FLAG_TEMP_4']
    names += re.findall(r'^#define (FLAG_POLISHED_\w+)', (ROOT/'include/constants/flags.h').read_text(), re.M)
    names += ['FLAG_POLISHED_' + n for n in ('CAVE_DONE','CAVE_POTION','CAVE_DUSK_STONE','CAVE_HYPER_POTION','CAVE_DIRE_HIT','ROD','ABE','FALKNER','ZEPHYR_BADGE','ROOST','EGG_CALL','EGG_GIVEN','EGG_REFUSED','AIDE_IN_VIOLET')]
    code = '#include "global.h"\n#include "constants/opponents.h"\nconst u32 values[] = {' + ','.join(names) + '};\n'
    r = subprocess.run(['arm-none-eabi-gcc','-mcpu=arm7tdmi','-mthumb','-mabi=apcs-gnu','-DMODERN=1','-DEMERALD','-std=gnu17','-iquote','include','-iquote','.','-S','-x','c','-o','-','-'],cwd=ROOT,input=code,text=True,capture_output=True,check=True)
    return {**sprout_constants(), **dict(zip(names, map(int,re.findall(r'\.word\s+(\d+)',r.stdout)),strict=True))}


def expected_party(name):
    f, c = source_files(), constants()
    src = f['data/trainers/parties.asm'].decode()
    group = src.split(('Falkner' if name == 'Falkner' else 'BirdKeeper') + 'Group:\n')[1]
    body = group.split('def_trainer ' + ('1' if name == 'Falkner' else name.upper()) + ', "' + name + '"\n')[1].split('end_trainer')[0]
    result = []
    for mon in re.split(r'\btr_mon ', body)[1:]:
        level, species = re.match(r'(\d+), (\w+)',mon).groups()
        if name == 'Falkner':
            moves = re.search(r'tr_moves (.*)',mon)[1].split(', ')
        else:
            learn = f['data/pokemon/evos_attacks.asm'].decode().split('evos_attacks ' + species.title() + '\n')[1].split('evos_attacks ')[0]
            moves = [m for lv,m in re.findall(r'learnset (\d+), (\w+)',learn) if int(lv)<=int(level)][-4:]
        result.append((c['SPECIES_'+species],int(level),[c['MOVE_'+m] for m in moves]+[0]*(4-len(moves))))
    return result


def settle(game, report):
    observed, portrait_seen, name = None, False, None
    attempted, last_slot, unavailable = None, None, set()
    for _ in range(3500):
        on_field = game.u32(game.sym["gMain"] + 4) == game.sym["CB2_Overworld"] | 1
        if (on_field and not task_active(game, "Task_BattleStart") and not script_active(game)
                and not game.read(game.sym["sLockFieldControls"], 1)[0]):
            if observed is not None:
                outcome = game.read(game.sym["gBattleOutcome"], 1)[0]
                assert outcome == (1 if name else 4), (name, outcome, party(game, details=True))
                if name:
                    assert portrait_seen, name
                    assert flag(game, constants()["FLAG_POLISHED_" + name.upper()])
                    report["trainers"][name] = observed
                    game.snapshot("falkner-after-" + name.lower())
                    print("PASS: Falkner phase " + name + " source party, portrait and victory", flush=True)
                    if name != "Falkner" and count(game, constants()["ITEM_POTION"]):
                        heal(game)
                else:
                    report["wild"].append(observed[0])
            return
        if not on_field:
            trainer_id = int.from_bytes(game.read(game.sym["gTrainerBattleParameter"] + constants()["opponent_offset"], 2), "little")
            trainer = bool(game.u32(game.sym["gBattleTypeFlags"]) & 8)
            name = next((n for n in TRAINERS if constants()["TRAINER_POLISHED_" + n.upper()] == trainer_id), None) if trainer else None
            if name and not portrait_seen and observed is None:
                portrait_seen = check_portrait(game, "falkner-" + name.lower(), "falkner" if name == "Falkner" else "bird_keeper")
        opcode = game.read(game.u32(game.sym["gBattlescriptCurrInstr"]), 1)[0] if not on_field else -1
        if opcode == constants()["B_SCR_OP_YESNOBOXLEARNMOVE"]:
            game.press(B, 20)  # Keep the existing moves and their PP.
        elif opcode == constants()["B_SCR_OP_YESNOBOXSTOPLEARNINGMOVE"]:
            game.press(A, 20)
        elif game.u32(game.sym["gMain"] + 4) == game.sym["CB2_UpdatePartyMenu"] | 1:
            mons = party(game, details=True)
            active = int.from_bytes(game.read(game.sym["gBattlerPartyIndexes"], 2), "little")
            if mons[active]["hp"]:
                game.press(B, 30)  # Cancel an optional shift prompt.
            else:
                healthy = next(i for i, m in enumerate(mons) if m["hp"])
                selected = game.read(game.sym["gPartyMenu"] + 9, 1)[0]
                game.press(A if selected == healthy else RIGHT if healthy else LEFT, 30)
        elif not on_field and controller(game, "HandleInputChooseAction"):
            if name and observed is not None:
                active = game.read(game.sym["gBattlerPartyIndexes"], 2)[0]
                mon = party(game, details=True)[active]
                if 0 < mon["hp"] <= 20 and count(game, constants()["ITEM_POTION"]):
                    battle_item(game, constants()["ITEM_POTION"], active)
                    continue
            if observed is None:
                observed = party(game, enemy=True, details=True)
                if name:
                    assert [(m["species"], m["level"], m["moves"]) for m in observed] == expected_party(name), (name, observed)
                    dv = 22 if name == "Falkner" else 16
                    evs = [32, 0, 0, 0, 0, 0] if name == "Falkner" else [5] * 6
                    assert all(m["ivs"] == [dv] * 6 and m["evs"] == evs for m in observed), observed
                    assert [m["item"] for m in observed] == ([0, 0, constants()["ITEM_PECHA_BERRY"]] if name == "Falkner" else [0] * len(observed))
                    game.screenshot("falkner-" + name.lower() + "-battle")
                else:
                    c = constants()
                    area = 'DARK_CAVE_VIOLET_ENTRANCE' if game.map_number() == map_id(game, map_data('PolishedDarkCaveVioletEntrance')) else 'ROUTE_31'
                    body = source_files()['data/wild/johto_grass.asm'].decode().split('def_grass_wildmons ' + area + '\n')[1].split('end_grass_wildmons')[0]
                    valid = {(c['SPECIES_' + sp], int(lv)) for lv, sp in re.findall(r'wildmon (\d+), (\w+)', body)}
                    assert len(observed) == 1 and (observed[0]['species'], observed[0]['level']) in valid, (area, observed)
                    game.screenshot("falkner-wild-" + str(len(report["wild"])))
            choose_cell(game, "gActionSelectionCursor", 0 if name else 3)
        elif not on_field and controller(game, "HandleInputChooseMove"):
            mon = party(game, details=True)[game.read(game.sym["gBattlerPartyIndexes"], 2)[0]]
            pp = game.read(game.sym["gBattleMons"] + layout()["battle_pp"], 4)
            enemy_index = int.from_bytes(game.read(game.sym["gBattlerPartyIndexes"] + 2, 2), "little")
            enemy = party(game, enemy=True, details=True)[enemy_index]
            signature = (tuple(pp), mon["hp"], mon["status"], enemy["hp"], enemy_index)
            if signature == attempted:
                unavailable.add(last_slot)  # A rejected choice, e.g. Gastly's Disable.
            else:
                unavailable.clear()
            strong_bite = enemy["species"] in (constants()["SPECIES_BELLSPROUT"], constants()["SPECIES_GASTLY"], constants()["SPECIES_NATU"])
            preferred = [constants()["MOVE_" + m] for m in (("BITE", "EMBER", "SURF", "WATER_GUN", "RAZOR_LEAF", "VINE_WHIP", "TACKLE") if strong_bite else ("SURF", "WATER_GUN", "BITE", "EMBER", "RAZOR_LEAF", "VINE_WHIP", "TACKLE"))]
            slot = next((i for m in preferred for i, move in enumerate(mon["moves"]) if move == m and pp[i] and i not in unavailable), None)
            assert slot is not None, (name, mon)
            attempted, last_slot = signature, slot
            choose_cell(game, "gMoveSelectionCursor", slot)
        elif not on_field and controller(game, "PlayerHandleYesNoInput"):
            game.press(B, 20)
        else:
            game.press(A, 12)
    game.screenshot("falkner-stalled")
    game.snapshot("falkner-stalled")
    pointers = {key: game.u32(game.sym[key]) for key in ("gBattlerControllerFuncs", "gBattleMainFunc")}
    pointers["callback"] = game.u32(game.sym["gMain"] + 4)
    resolved = {key: [n for n, value in game.sym.items() if isinstance(value, int) and value | 1 == ptr] for key, ptr in pointers.items()}
    raise AssertionError(("Falkner phase interaction stalled", name, game.position(), resolved, party(game, details=True)))



def battle_item(game, item, active):
    before=count(game,item)
    choose_cell(game,'gActionSelectionCursor',1)
    wait_for(game,lambda:task_active(game,'Task_BagMenu_HandleInput'),'battle Bag')
    game.frames(60);select_bag_item(game,item);game.press(A,90)
    for _ in range(140):
        if count(game,item)==before-1 and game.u32(game.sym['gMain']+4)==game.sym['BattleMainCB2']|1:
            return
        if task_active(game,'Task_HandleChooseMonInput'):
            selected=game.read(game.sym['gPartyMenu']+9,1)[0]
            game.press(A if selected==active else LEFT if active==0 else RIGHT,30)
        else:
            game.press(A,20)
    raise AssertionError(('Battle item stalled',item,count(game,item),before))

def walk(game, data, goal, report, avoid=()):
    for _ in range(600):
        settle(game,report)
        if game.position()==goal:
            return
        blocked = set(avoid) | {(w['x'],w['y']) for w in data['warp_events']}
        live=actors(game)
        for number,obj in enumerate(data['object_events'],1):
            if obj['flag']!='0' and flag(game,constants().get(obj['flag'], 0)):
                continue
            blocked.add(live[number]['position'] if number in live else (obj['x'],obj['y']))
        blocked.discard(goal)
        start=game.position();queue=deque([start]);paths={start:[]}
        while queue and goal not in paths:
            x,y=queue.popleft()
            for key,dx,dy in DIRECTIONS:
                pos=x+dx,y+dy
                if pos in paths or pos in blocked or not(0<=pos[0]<data['width'] and 0<=pos[1]<data['height']):continue
                cell=data['cells'][pos[1]*data['width']+pos[0]]
                if cell&0xc00 or cell>>12!=3:continue
                paths[pos]=paths[x,y]+[key];queue.append(pos)
        assert goal in paths,('No path',data['name'],start,goal,blocked)
        try:game.step(paths[goal][0])
        except AssertionError as e:
            if not str(e).startswith('Blocked at'):raise
        game.frames(30)
    raise AssertionError(('Walk timeout',data['name'],game.position(),goal))



def enter_building(game, city, dest, report):
    door=next(w for w in city['warp_events'] if w['dest_map']==dest['id'])
    walk(game,city,(door['x'],door['y']+1),report)
    transition(game,dest,UP)

def cave_entry(game, city, cave, report):
    gate=map_data('PolishedRoute31VioletGate');route=map_data('PolishedRoute31')
    walk(game,city,(38,24),report);transition(game,gate,RIGHT)
    assert game.map_number()==map_id(game,gate)
    indoor_walk(game,gate,(8,4));transition(game,route,RIGHT)
    assert game.map_number()==map_id(game,route)
    walk(game,route,(34,6),report)
    transition(game,cave,UP)
    assert game.map_number()==map_id(game,cave)
    game.snapshot('falkner-cave-entry')


def cave_exit(game, cave, city, report):
    route=map_data('PolishedRoute31');gate=map_data('PolishedRoute31VioletGate')
    walk(game,cave,(3,14),report)
    transition(game,route,DOWN)
    assert game.map_number()==map_id(game,route)
    walk(game,route,(5,6),report)
    transition(game,gate,LEFT)
    assert game.map_number()==map_id(game,gate)
    indoor_walk(game,gate,(1,4));transition(game,city,LEFT)
    assert game.map_number()==map_id(game,city)


def check_falkner(game, city, checks):
    c=constants();gym=map_data('PolishedVioletGym');cave=map_data('PolishedDarkCaveVioletEntrance')
    center=map_data('PolishedVioletPokeCenter1F')
    report={'trainers':{},'wild':[]}
    game.snapshot('falkner-after-sprout')
    enter_building(game,city,gym,report)
    assert not flag(game,c['FLAG_POLISHED_CAVE_DONE'])
    advance(game,lambda:text_matches(game,'PolishedVioletGym_AwayText'))
    assert set(actors(game))=={1}
    game.screenshot('falkner-gym-away')
    advance(game,lambda:game.map_number()==map_id(game,city) and not script_active(game))
    checks.append('Violet Gym turns the player away while Falkner is in Dark Cave')

    # Visit before obtaining Flash, then return to the naturally earned summit checkpoint.
    game.restore('violet-day')
    assert not flag(game,c['FLAG_POLISHED_SPROUT_FLASH'])
    cave_entry(game,city,cave,report)
    walk(game,cave,(5,2),report,avoid=((6,2),))
    game.step(RIGHT)
    advance(game,lambda:text_matches(game,'PolishedDarkCave_Darkness'))
    game.screenshot('dark-cave-warning')
    advance(game,lambda:not script_active(game))
    assert game.position()==(5,2) and not flag(game,c['FLAG_POLISHED_CAVE_DONE'])
    checks.append('Dark Cave warns and steps the player back before Flash acquisition')

    game.restore('falkner-after-sprout')
    enter_building(game,city,center,report);interact(game,center,(5,3),UP)
    advance(game,lambda:not script_active(game),limit=400);leave(game,center,city)
    cave_entry(game,city,cave,report)
    walk(game,cave,(5,2),report,avoid=((6,2),));game.step(RIGHT)
    advance(game,lambda:text_matches(game,'PolishedDarkCave_Attack'))
    assert all(i in actors(game) for i in (1,2,3))
    game.screenshot('dark-cave-falkner-rescue')
    advance(game,lambda:text_matches(game,'PolishedDarkCave_Return'))
    assert 1 not in actors(game)
    advance(game,lambda:not script_active(game),limit=500)
    assert game.position()==(3,2) and flag(game,c['FLAG_POLISHED_CAVE_DONE'])
    assert not any(i in actors(game) for i in (1,2,3))
    save_game(game);continue_game(game)
    assert flag(game,c['FLAG_POLISHED_CAVE_DONE']) and not any(i in actors(game) for i in (1,2,3))
    walk(game,cave,(6,2),report);game.frames(120)
    assert not script_active(game)
    game.snapshot('falkner-cave-cleared')
    cave_exit(game,cave,city,report)
    checks.append('Falkner defeats Ursaring, recalls Pidgeotto, leaves by the source path and remains gone after Save/Continue')

    check_gym(game,city,checks,report)


def check_gym(game, city, checks, report):
    c=constants();gym=map_data('PolishedVioletGym');center=map_data('PolishedVioletPokeCenter1F')
    enter_building(game,city,gym,report)
    assert 1 not in actors(game) and all(i in actors(game) for i in (4,5)) and not flag(game,c["FLAG_TEMP_1"])
    game.screenshot('violet-gym-open')
    # Follow the source's zigzag walkway through both three-tile sightlines.
    for pos in ((4,10),(6,8),(3,8),(3,6),(4,6)):
        walk(game,gym,pos,report)
    walk(game,gym,(5,2),report)
    leave(game,gym,city)
    # Source learnsets change the climb's damage and item use. Replenish the
    # boss supplies with earned money instead of assuming Sprout left Potions.
    mart=map_data('PolishedVioletMart')
    enter_building(game,city,mart,report);open_shop(game,mart)
    potion=stock(game)[2]
    for n in range(min(max(0,8-count(game,potion)),money(game)//300)):
        purchase(game,2 if n==0 else 0,potion)
    close_shop(game);leave(game,mart,city)
    enter_building(game,city,center,report);interact(game,center,(5,3),UP)
    advance(game,lambda:not script_active(game),limit=400);leave(game,center,city)
    enter_building(game,city,gym,report);walk(game,gym,(5,2),report)
    game.snapshot('falkner-ready')
    game.face(UP);game.press(A,60);settle(game,report)
    assert set(report['trainers'])==set(TRAINERS),report
    assert flag(game,c['FLAG_POLISHED_ZEPHYR_BADGE']) and flag(game,c['FLAG_BADGE01_GET'])
    assert not flag(game,c['FLAG_BADGE05_GET'])
    assert flag(game,c['FLAG_POLISHED_ROOST']) and count(game,c['ITEM_TM_ROOST'])==1
    game.screenshot('falkner-badge-received')
    save_game(game);continue_game(game)
    game.face(UP);game.press(A,60);settle(game,report)
    assert count(game,c['ITEM_TM_ROOST'])==1
    game.snapshot('falkner-gym-cleared')
    checks.append('Abe, Rod and Falkner use original portraits and parties; the first badge and one reusable Roost survive Save/Continue without repeat battles')
    leave(game,gym,city)
    advance(game,lambda:text_matches(game,'PolishedEgg_CallText'))
    game.screenshot('elm-togepi-call')
    advance(game,lambda:not script_active(game))
    assert flag(game,c['FLAG_POLISHED_EGG_CALL']) and flag(game,c['FLAG_POLISHED_AIDE_IN_VIOLET'])
    enter_building(game,city,center,report)
    assert 6 in actors(game)
    interact(game,center,(10,3),UP)
    choice_prompt(game,'PolishedEgg_Intro')
    game.press(DOWN,12);game.press(A,60)
    advance(game,lambda:not script_active(game))
    assert flag(game,c['FLAG_POLISHED_EGG_REFUSED']) and not flag(game,c['FLAG_POLISHED_EGG_GIVEN'])
    before=len(party(game))
    interact(game,center,(10,3),UP);choice_prompt(game,'PolishedEgg_Question')
    game.press(A,60)
    advance(game,lambda:text_matches(game,'PolishedEgg_Received'))
    game.screenshot('togepi-egg-received')
    advance(game,lambda:not script_active(game),limit=500)
    assert len(party(game))==before+1 and party(game,details=True)[-1]['species']==c['SPECIES_TOGEPI']
    assert party(game,details=True)[-1]['is_egg']
    assert flag(game,c['FLAG_POLISHED_EGG_GIVEN']) and not flag(game,c['FLAG_POLISHED_AIDE_IN_VIOLET'])
    assert 6 not in actors(game)
    save_game(game);continue_game(game)
    assert 6 not in actors(game) and len(party(game))==before+1
    leave(game,center,city);enter_building(game,city,center,report)
    assert 6 not in actors(game) and len(party(game))==before+1
    game.snapshot('falkner-egg-complete')
    checks.append('Elm calls once after the badge; the aide accepts refusal, gives one Togepi Egg, walks out and remains absent after Save/Continue and reentry')
    (game.output/'falkner-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: Dark Cave rescue, Violet Gym, Zephyr Badge, Roost and Togepi Egg',flush=True)
