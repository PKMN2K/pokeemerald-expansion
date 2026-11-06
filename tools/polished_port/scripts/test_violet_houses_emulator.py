"""Visit Violet's houses and trade through normal input; no injected game state."""
import re
from build_new_bark import source_files
from build_johto_demo import palette_rows
from test_johto_demo_emulator import A, B, UP, DOWN, LEFT, RIGHT, START
from test_new_bark_emulator import (map_data, walk, actors, talk_to_resident,
    close_resident_dialogue, save_game, continue_game, check_resident_palette)
from test_new_bark_intro_emulator import (advance, task_active, text_matches,
    choice_prompt, party, script_active, layout)
from test_cherrygrove_emulator import enter as enter_building, leave, interact, flag
from test_academy_emulator import wait_for, cb
from test_new_bark_battle_emulator import controller, choose_cell
from test_cherrygrove_emulator import extra_layout, sea_battle
from test_violet_mart_emulator import count, open_shop, purchase, close_shop, stock, select_bag_item

P = "PolishedVioletOnixTradeHouse_"


def walk_city(game,city,goal):
    # Replan around live patrol positions. Reserving their whole range would
    # seal Violet's narrow east-west passage; a long path can become occupied.
    for _ in range(20):
        try:
            walk(game,city,goal,avoid=((5,18),(13,16)))
            return
        except AssertionError as error:
            if not str(error).startswith(("Blocked at", "No path in")):
                raise
            assert not script_active(game)
            game.frames(30)
    raise AssertionError(("Violet patrol blocked route",game.position(),goal))


def enter(game, city, house):
    door=next(w for w in city["warp_events"] if w["dest_map"]==house["id"])
    # The west-side youngster may be off-camera when planning across town.
    walk_city(game,city,(door["x"],door["y"]+1))
    enter_building(game,city,house)


def prompt(game, house):
    talk_to_resident(game,house,2,"violet-kyle-offer",leave_open=True)
    choice_prompt(game,P+"IntroText")


def select_party(game, slot):
    # The native combined selector starts in box 1. Open its party tray.
    for _ in range(5):
        game.press(DOWN,30)
    game.press(A,120)
    assert game.read(game.sym["sCursorArea"],1)==b"\x01"
    for _ in range(slot):
        game.press(DOWN,30)
    game.press(A,90)
    game.press(A,180)


def catch_poliwag(game,city):
    mart=map_data("PolishedVioletMart")
    enter(game,city,mart)
    open_shop(game,mart)
    ball=stock(game)[0]
    for _ in range(5):
        purchase(game,0,ball)
    close_shop(game);leave(game,mart,city)
    center=map_data("PolishedVioletPokeCenter1F")
    enter(game,city,center)
    interact(game,center,(5,3),UP)
    advance(game,lambda:not script_active(game),limit=400)
    leave(game,center,city)
    walk_city(game,city,(24,21));walk_city(game,city,(21,10))
    game.face(DOWN);game.press(A)
    advance(game,lambda:bool(game.read(game.sym["gPlayerAvatar"],1)[0]&8) and not script_active(game))
    game.frames(180)
    before=len(party(game))
    for _ in range(30):
        for _ in range(500):
            direction=DOWN if game.position()[1]==11 else UP
            try:
                game.step(direction)
            except AssertionError:
                assert task_active(game,"Task_BattleStart") or not cb(game,"CB2_Overworld")
            game.frames(45)
            if task_active(game,"Task_BattleStart") or not cb(game,"CB2_Overworld"):
                break
        else:
            raise AssertionError("No Violet pond encounter")
        for _ in range(200):
            if controller(game,"HandleInputChooseAction"):
                break
            game.press(B,30)
        offered=party(game,enemy=True,details=True)[0]
        if offered["species"]==extra_layout()["poliwag"]:
            break
        sea_battle(game,water_table=[(extra_layout()["poliwhirl"],20)],area="violet-poliwhirl")
        game.frames(180)
    else:
        raise AssertionError("No Poliwag after 30 pond encounters")
    game.snapshot("kyle-wild-encounter")
    finish_poliwag_capture(game,before,ball)
    game.frames(180)
    while game.position()[1]>10:
        game.step(UP)
    game.frames(90)
    return before


def finish_poliwag_capture(game,before,ball):
    offered=None
    for _ in range(1800):
        if cb(game,"CB2_Overworld") and not task_active(game,"Task_BattleStart"):
            if len(party(game))==before+1:
                break
        opcode=game.read(game.u32(game.sym["gBattlescriptCurrInstr"]),1)[0] if cb(game,"BattleMainCB2") else -1
        # Catch EXP can reach a source learn level. Repeated B would alternate
        # between declining the move and declining to stop learning it.
        if opcode==layout()["learn_move_opcode"]:
            game.press(B,20)
        elif opcode==layout()["stop_learning_opcode"]:
            game.press(A,20)
        elif controller(game,"HandleInputChooseAction"):
            offered=party(game,enemy=True,details=True)[0]
            assert offered["species"]==extra_layout()["poliwag"],offered
            assert count(game,ball)>0,"Ran out of Poké Balls"
            choose_cell(game,"gActionSelectionCursor",1)
            wait_for(game,lambda:task_active(game,"Task_BagMenu_HandleInput"),"battle Bag")
            game.frames(90)
            # The pocket retains its cursor; select the real ball stack.
            select_bag_item(game,ball,pocket=1)
            game.press(A,180)
        elif cb(game,"CB2_UpdatePartyMenu"):
            # A fainted lead opens both the party selector and a separate
            # Send Out menu. Confirm both; B would cancel the second forever.
            healthy=[i for i,m in enumerate(party(game,details=True)) if m["hp"]]
            assert healthy,"Party lost before catching Poliwag"
            target=healthy[0]
            selected=game.read(game.sym["gPartyMenu"]+9,1)[0]
            game.press(A if selected==target else RIGHT if target else LEFT,30)
        else:
            # B advances battle text and declines the captured-mon nickname.
            game.press(B,30)
    else:
        raise AssertionError(("Poliwag capture did not finish", opcode,
            int.from_bytes(game.read(game.sym["gMoveToLearn"],2),"little"),
            count(game,ball),party(game,details=True)))
    assert party(game)[-1]["species"]==extra_layout()["poliwag"]
    assert party(game)[-1]["level"]==offered["level"]
    game.screenshot("violet-poliwag-caught")


def check_houses(game,city,checks):
    nickname=map_data("PolishedVioletNicknameSpeechHouse")
    trade=map_data("PolishedVioletOnixTradeHouse")
    game.set_hour(12)
    game.snapshot("houses-city")
    for house,landing in ((nickname,(3,16)),(trade,(21,30))):
        enter(game,city,house)
        assert game.position()==(3,7),game.position()
        game.snapshot(house["name"])
        game.screenshot(house["name"])
        for index in (0,1):
            game.restore(house["name"])
            leave(game,house,city,index)
            assert game.position()==landing,game.position()
    checks.append("Both source houses open at their Violet doorways and all four exit tiles return correctly")
    for hour in (4,5,16,17,23,0):
        game.restore("houses-city")
        game.set_hour(hour)
        enter(game,city,nickname)
        assert (4 in actors(game))==(hour<5 or hour>=17),(hour,actors(game))
        if hour==17:
            game.snapshot("nickname-evening")
    game.restore("nickname-evening")
    files=source_files()
    colors=re.findall(r"^\s*ow_npc_pal_const (\w+)",files["constants/sprite_data_constants.asm"].decode(),re.M)
    rows=palette_rows(files["gfx/overworld/npc_sprites.pal"].decode())
    for actor,color in ((1,"RED"),(2,"GREEN"),(3,"BROWN"),(4,"PURPLE")):
        check_resident_palette(game,nickname,actor,colors.index(color),"day",rows)
        talk_to_resident(game,nickname,actor,"violet-nickname-resident-"+str(actor))
    save_game(game)
    game.set_hour(12)
    continue_game(game)
    assert 4 not in actors(game)
    game.set_hour(17)
    # A menu return preserves the source entry schedule until a new entry.
    game.press(START);game.press(B)
    assert 4 not in actors(game)
    save_game(game);continue_game(game)
    assert 4 in actors(game)
    checks.append("Strawberry and all three residents converse; Gramps follows 05:00/17:00 boundaries and reconciles on Continue while menu returns retain the current visit")
    # RTC overrides are external to snapshots. Enter afresh in daytime so
    # Gramps is away from the incense burner we are about to inspect.
    game.restore("houses-city")
    game.set_hour(12)
    enter(game,city,nickname)
    for x,label in ((0,"IncenseBurner"),(1,"MagazineBookshelf")):
        interact(game,nickname,(x,2),UP,nickname["name"]+"_"+label+"_Text")
        close_resident_dialogue(game)
    game.restore(trade["name"])
    talk_to_resident(game,trade,1,"violet-trade-father")
    before=party(game)
    prompt(game,trade)
    game.press(B)
    advance(game,lambda:text_matches(game,P+"CancelText"))
    close_resident_dialogue(game)
    assert party(game)==before and not flag(game,0x4d8)
    prompt(game,trade)
    game.press(A,240)
    wait_for(game,lambda:cb(game,"CB2_PokeStorage"),"Kyle selection")
    game.frames(180)
    game.screenshot("violet-kyle-selection")
    game.snapshot("kyle-selection")
    game.press(B,180)
    game.press(B,180)  # Decline the native "Continue Box operations?" prompt.
    advance(game,lambda:text_matches(game,P+"CancelText"))
    close_resident_dialogue(game)
    assert party(game)==before and not flag(game,0x4d8)
    checks.append("Kyle's No and selection-cancel branches preserve the party and leave the trade available")
    game.restore("kyle-selection")
    select_party(game,0)
    advance(game,lambda:text_matches(game,P+"WrongText"))
    close_resident_dialogue(game)
    assert party(game)==before and not flag(game,0x4d8)
    checks.append("Offering the wrong species produces Kyle's source refusal without changing the party or consuming the trade")
    game.restore("houses-city")
    slot=catch_poliwag(game,city)
    enter(game,city,trade)
    game.snapshot("kyle-ready")
    before=party(game)
    prompt(game,trade);game.press(A,240)
    wait_for(game,lambda:cb(game,"CB2_PokeStorage"),"Kyle selection")
    game.frames(180)
    select_party(game,slot)
    advance(game,lambda:text_matches(game,P+"CompleteText") and cb(game,"CB2_Overworld"),limit=600)
    game.screenshot("violet-kyle-complete")
    close_resident_dialogue(game)
    after=party(game)
    assert flag(game,0x4d8) and len(after)==len(before)
    assert after[:slot]==before[:slot]
    assert after[slot]["species"]==100 and after[slot]["level"]==before[slot]["level"]
    assert after[slot]["trainer"]==48926
    save_game(game);continue_game(game)
    assert party(game)==after and flag(game,0x4d8)
    talk_to_resident(game,trade,2,"violet-kyle-after",leave_open=True)
    advance(game,lambda:text_matches(game,P+"AfterText"))
    close_resident_dialogue(game)
    assert party(game)==after and not task_active(game,"Task_HandleYesNoInput")
    checks.append("A Poliwag caught from Violet's pond trades through the native animation for same-level Mimic; party replacement and the one-time guard survive Save/Continue")
    game.restore("houses-city")
