"""Travel the actual Route 30/31 connection, fight its trainers and use both gate doors."""
import json
import re
import subprocess
from functools import lru_cache
from build_johto_demo import ROOT
from test_johto_demo_emulator import A, B, UP, DOWN, LEFT, RIGHT
from test_new_bark_emulator import map_data, walk, save_game, continue_game, close_resident_dialogue
from test_new_bark_intro_emulator import advance, text_matches, script_active, choice_prompt, party, task_active
from test_new_bark_battle_emulator import controller, choose_cell
from test_cherrygrove_emulator import flag, map_id
from test_post_egg_emulator import inventory, fields
from test_route29_emulator import route_walk
from test_route30_emulator import berries, pick
from test_return_emulator import battle


@lru_cache
def ids():
    names = ["SPECIES_"+n for n in ("RATTATA","PIDGEY","CATERPIE","WEEDLE","GEODUDE","MAREEP")]
    names += ["MOVE_"+n for n in ("TACKLE","LEER","STRING_SHOT","POISON_STING","DEFENSE_CURL","MUD_SLAP","ROLLOUT","THUNDER_WAVE","THUNDER_SHOCK")]
    names += ["ITEM_"+n for n in ("AIR_BALLOON","POTION","POKE_BALL","PERSIM_BERRY")]
    names += ["FLAG_POLISHED_ROUTE31_"+n for n in ("WADE","FINCH","AIR_BALLOON","POTION","POKE_BALL","MET_FINCH")]
    code = '#include "global.h"\n#include "constants/flags.h"\n#include "constants/items.h"\nconst u32 ids[] = {'+','.join(names)+'};'
    proc = subprocess.run(["arm-none-eabi-gcc","-mcpu=arm7tdmi","-mthumb","-mabi=apcs-gnu","-DMODERN=1","-DEMERALD","-std=gnu17","-iquote","include","-iquote",".","-S","-x","c","-o","-","-"],cwd=ROOT,input=code,text=True,capture_output=True,check=True)
    return dict(zip(names,map(int,re.findall(r'\.word\s+(\d+)',proc.stdout)),strict=True))


def team_row(species, level, moves, item=None):
    d=ids()
    actual=[d["MOVE_"+move] for move in moves]
    return (d["SPECIES_"+species],level,d["ITEM_"+item] if item else 0,actual+[0]*(4-len(actual)))


def count(game, name):
    return sum(quantity for item,quantity in inventory(game) if item == ids()["ITEM_"+name])


def handle_wild(game, observed, label):
    captured = False
    for _ in range(600):
        if task_active(game,"Task_BattleStart"):
            game.frames(20)
        elif game.u32(game.sym["gMain"]+4)==game.sym["CB2_Overworld"]|1:
            game.frames(30)
            if game.u32(game.sym["gMain"]+4)==game.sym["CB2_Overworld"]|1 and not task_active(game,"Task_BattleStart"):
                return
        elif controller(game,"HandleInputChooseAction"):
            if not captured:
                enemy=party(game,enemy=True,details=True)
                assert len(enemy)==1
                observed.append((enemy[0]["species"],enemy[0]["level"]))
                captured=True
            choose_cell(game,"gActionSelectionCursor",3)
        else:
            game.press(A,20)
    raise AssertionError(("Wild encounter stalled",label,game.position()))


def travel(game, data, goal, seen, avoid=()):
    walked = dict(data, warp_events=data["warp_events"]+[{"x":x,"y":y} for x,y in avoid])
    route_walk(game,walked,goal,seen,handle_wild)
    assert game.map_number()==map_id(game,data), (data["name"],goal,game.position())


def enter(game, dest, direction):
    # Side carpets warp on the outward step, including the step off the carpet.
    for _ in range(45):
        if game.map_number()==map_id(game,dest):
            game.frames(100)
            return
        game.frames(8,direction)
    raise AssertionError(("Missing connection",dest["name"],game.map_number(),game.position()))


def talk(game, direction, label):
    game.face(direction)
    game.press(A,80)
    advance(game,lambda:text_matches(game,label))


def check_route31(game, route30, checks):
    route, gate = map_data("PolishedRoute31"),map_data("PolishedRoute31VioletGate")
    # Three absent first-visit actors would otherwise block Mikey's only approach.
    route30=dict(route30,object_events=[o for o in route30["object_events"] if o["local_id"] not in
                  ("LOCALID_ROUTE30_JOEY","LOCALID_ROUTE30_PIDGEY","LOCALID_ROUTE30_RATTATA")])
    seen, teams = [], {}
    travel(game,route30,(5,25),seen,[(5,24)])
    game.step(UP)
    advance(game,lambda:text_matches(game,"PolishedRoute30_MikeySeen"))
    teams["Mikey"]=battle(game,"route31-mikey",[team_row("RATTATA",2,["TACKLE","LEER"]),team_row("PIDGEY",4,["TACKLE"])],iv=12)
    advance(game,lambda:not script_active(game))
    print("PASS: Route 30 Mikey",flush=True)
    travel(game,route30,(1,11),seen,[(1,y) for y in (8,9,10)])
    game.step(UP)
    advance(game,lambda:text_matches(game,"PolishedRoute30_DonSeen"))
    cat=team_row("CATERPIE",3,["TACKLE","STRING_SHOT"])
    teams["Don"]=battle(game,"route31-don",[cat,cat],iv=12)
    advance(game,lambda:not script_active(game))
    print("PASS: Route 30 Don",flush=True)
    travel(game,route30,(4,0),seen)
    if not flag(game,fields()["open"]):
        game.snapshot("before-moms-call")
        for answer in (False,True):
            game.restore("before-moms-call")
            enter(game,route,UP)
            choice_prompt(game,"PolishedRoute31_MomLecture")
            game.screenshot("route31-mom-worried")
            game.press(A if answer else B,60)
            advance(game,lambda:not script_active(game))
            assert flag(game,fields()["open"]) and flag(game,fields()["save"])==answer
            save_game(game);continue_game(game)
            assert flag(game,fields()["open"]) and flag(game,fields()["save"])==answer
        checks.append("Mom calls when her account is unopened; Yes and No both persist through native Save/Continue")
    else:
        enter(game,route,UP)
    assert game.position()==(24,17),game.position()
    game.screenshot("route31-entry")
    game.command("save "+str(game.output/"route31-entered.state"))
    checks.append("Mikey and Don use their source teams; the source north connection lands at Route 31 (24,17)")
    guarded=[(28,8),(28,9)]+[(x,13) for x in range(16,21)]
    travel(game,route,(29,8),seen,guarded)
    game.step(LEFT)
    advance(game,lambda:text_matches(game,"PolishedRoute31_FinchIntroText"))
    game.screenshot("route31-finch-introduction")
    close_resident_dialogue(game)
    assert flag(game,ids()["FLAG_POLISHED_ROUTE31_MET_FINCH"])
    guarded = [(x,13) for x in range(16,21)]  # The introduction is complete.
    travel(game,route,(29,6),seen)
    before=count(game,"POTION")
    game.face(UP);game.press(A,80);close_resident_dialogue(game)
    assert count(game,"POTION")==before+1 and flag(game,ids()["FLAG_POLISHED_ROUTE31_POTION"])
    travel(game,route,(16,8),seen,guarded)
    item=ids()["ITEM_PERSIM_BERRY"];before=berries(game,item)
    game.face(UP);game.press(A,80)
    advance(game,lambda:any(text_matches(game,"PolishedRoute29_FruitObtained"+s) for s in ("One","Two","Three")))
    close_resident_dialogue(game);assert 1<=berries(game,item)-before<=3
    travel(game,route,(17,8),seen);talk(game,UP,"PolishedRoute31_SleepyText")
    close_resident_dialogue(game)
    checks.append("Finch explains his challenge before Wade is beaten; the Potion, Persim tree and sleepy resident work at their source coordinates")
    travel(game,route,(20,12),seen,guarded)
    game.step(DOWN)
    advance(game,lambda:text_matches(game,"PolishedRoute31_WadeSeen"))
    cat=team_row("CATERPIE",2,["TACKLE","STRING_SHOT"])
    weedle=team_row("WEEDLE",3,["POISON_STING","STRING_SHOT"])
    teams["Wade"]=battle(game,"route31-wade",[cat,cat,weedle,cat],iv=12,portrait="bug_catcher")
    advance(game,lambda:not script_active(game))
    assert flag(game,ids()["FLAG_POLISHED_ROUTE31_WADE"])
    print("PASS: Route 31 Wade and his source portrait",flush=True)
    travel(game,route,(19,14),seen); before=count(game,"POKE_BALL")
    game.face(DOWN);game.press(A,80);close_resident_dialogue(game)
    assert count(game,"POKE_BALL")==before+1
    travel(game,route,(29,7),seen);game.face(LEFT);game.press(A,80)
    choice_prompt(game,"PolishedRoute31_FinchQuestionText");game.press(B,60)
    advance(game,lambda:text_matches(game,"PolishedRoute31_FinchRefusedText"));close_resident_dialogue(game)
    before=count(game,"AIR_BALLOON")
    game.face(LEFT);game.press(A,80);choice_prompt(game,"PolishedRoute31_FinchQuestionText");game.press(A,60)
    advance(game,lambda:text_matches(game,"PolishedRoute31_FinchSeenText"))
    teams["Finch"]=battle(game,"route31-finch",[
        team_row("GEODUDE",8,["TACKLE","DEFENSE_CURL","MUD_SLAP","ROLLOUT"],"AIR_BALLOON"),
        team_row("MAREEP",9,["TACKLE","THUNDER_WAVE","THUNDER_SHOCK","MUD_SLAP"],"AIR_BALLOON")],
        portrait="cooltrainer_m",iv=26,hp_ev=12)
    advance(game,lambda:text_matches(game,"PolishedRoute31_FinchAfterText2"))
    close_resident_dialogue(game);assert count(game,"AIR_BALLOON")==before+1
    save_game(game);continue_game(game)
    assert all(flag(game,ids()["FLAG_POLISHED_ROUTE31_"+n]) for n in ("WADE","FINCH","POTION","POKE_BALL","AIR_BALLOON"))
    talk(game,LEFT,"PolishedRoute31_FinchAfterText2");close_resident_dialogue(game)
    assert count(game,"AIR_BALLOON")==before+1
    checks.append("Wade’s four bugs and Finch’s two Air Balloon holders match the source in battle; both portraits, declining Finch, winning, one reward and native Save/Continue work")
    for row in (6,7):
        travel(game,route,(5,row),seen);enter(game,gate,LEFT)
        assert game.position()==(9,row-2),(row,game.position())
        game.screenshot("route31-violet-gate-"+str(row))
        if row==6:
            walk(game,gate,(5,4));talk(game,UP,"PolishedRoute31VioletGate_Route31VioletGateOfficerText_Text");close_resident_dialogue(game)
            walk(game,gate,(1,3));talk(game,UP,"PolishedRoute31VioletGate_Route31VioletGateCooltrainerFText_Text");close_resident_dialogue(game)
        walk(game,gate,(8,row-2));enter(game,route,RIGHT)
        assert game.position()==(4,row),(row,game.position())
    travel(game,route,(24,17),seen);enter(game,route30,DOWN)
    assert game.position()==(4,0),game.position()
    checks.append("Both rows of the Violet gate lead back to the correct route tiles, both gate residents speak, and the south connection returns to Route 30")
    (game.output/"route31.json").write_text(json.dumps({"teams":teams,"wild":seen,"return":game.position()},indent=2)+"\n")
    print("PASS: Route 31 Finch, rewards, both gate doors and return",flush=True)
