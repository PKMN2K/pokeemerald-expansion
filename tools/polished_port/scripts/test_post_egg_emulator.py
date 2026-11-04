"""Real-input post-Egg tests. All state comes from ordinary story events and saves."""
import json
import struct
import subprocess
import re
from functools import lru_cache

from build_johto_demo import ROOT
from test_johto_demo_emulator import A,B,UP,DOWN,LEFT,RIGHT
from test_new_bark_emulator import (map_data,walk,travel,actors,resident_id,save_game,
    continue_game,close_resident_dialogue,talk_to_resident)
from test_new_bark_intro_emulator import layout,party,advance,script_active,text_matches,task_active,choice_prompt
from test_new_bark_mom_emulator import weekday
from test_cherrygrove_emulator import flag,money,count_item,map_id
from test_route29_emulator import route_walk


@lru_cache
def fields():
    keys={'lo':'VAR_POLISHED_MOM_BALANCE_LO','hi':'VAR_POLISHED_MOM_BALANCE_HI',
        'open':'FLAG_POLISHED_MOM_BANK_OPEN','save':'FLAG_POLISHED_MOM_SAVES_MONEY',
        'done':'FLAG_POLISHED_CAUGHT_TUTORIAL','decline':'FLAG_POLISHED_DECLINED_TUTORIAL',
        'dst':'FLAG_POLISHED_DAYLIGHT_SAVING','balls':'offsetof(struct SaveBlock1, bag.pokeBalls)',
        'bag':'offsetof(struct SaveBlock1, bag)','bag_size':'sizeof(struct Bag)',
        'slots':'BAG_POKEBALLS_COUNT','ball':'ITEM_POKE_BALL','premier':'ITEM_PREMIER_BALL',
        'scarf':'ITEM_SILK_SCARF','pidgey':'SPECIES_PIDGEY','caught':'B_OUTCOME_CAUGHT'}
    code='#include "global.h"\n#include "constants/items.h"\n#include "constants/battle.h"\n#include "constants/flags.h"\n#include "constants/vars.h"\nconst u32 fields[]={'+','.join(keys.values())+'};'
    r=subprocess.run(['arm-none-eabi-gcc','-mcpu=arm7tdmi','-mthumb','-mabi=apcs-gnu','-DMODERN=1','-DEMERALD',
        '-std=gnu17','-iquote','include','-iquote','.','-S','-x','c','-o','-','-'],cwd=ROOT,input=code,text=True,capture_output=True,check=True)
    return dict(zip(keys,map(int,re.findall(r'\.word\s+(\d+)',r.stdout))))


def balance(game):
    base=game.u32(game.sym['gSaveBlock1Ptr'])+layout()['vars']
    return sum(int.from_bytes(game.read(base+2*(fields()[name]-0x4000),2),'little')<<bits
               for name,bits in (('lo',0),('hi',16)))


def balls(game,item):
    raw=game.read(game.u32(game.sym['gSaveBlock1Ptr'])+fields()['balls'],4*fields()['slots'])
    key=game.u32(game.u32(game.sym['gSaveBlock2Ptr'])+layout()['encryption_key'])&0xffff
    return sum(n^key for i,n in struct.iter_unpack('<HH',raw) if i==item)


def inventory(game):
    start = game.u32(game.sym['gSaveBlock1Ptr']) + fields()['bag']
    key = game.u32(game.u32(game.sym['gSaveBlock2Ptr']) + layout()['encryption_key']) & 0xffff
    slots = struct.iter_unpack('<HH', game.read(start, fields()['bag_size']))
    return [(item, count ^ key if item else 0) for item, count in slots]


def say(game,house):
    local=next(i for i in actors(game) if house['object_events'][i-1]['local_id'].startswith('LOCALID_HOME_')
               and house['object_events'][i-1]['local_id']!='LOCALID_HOME_NEIGHBOR')
    talk_to_resident(game,house,local,'departure-mom',leave_open=True)


def yesno(game,label,yes):
    choice_prompt(game,label)
    game.press(A if yes else B,60)


def bank_menu(game,house,row):
    say(game,house)
    yesno(game,'PolishedMom_IsThisAboutYourMoneyText',True)
    advance(game,lambda:task_active(game,'Task_HandleMultichoiceInput'))
    game.frames(24)
    for _ in range(row):
        game.press(DOWN,12)
    game.press(A,45)


def amount(game,units,tens=0,cancel=False):
    advance(game,lambda:task_active(game,'Task_PolishedMomTransfer'))
    game.frames(12)
    for _ in range(units):
        game.press(UP,10)
    if tens:
        game.press(LEFT,10)
        for _ in range(tens):
            game.press(UP,10)
    game.screenshot('departure-bank-amount')
    game.press(B if cancel else A,45)


def check_bank(game,town):
    house=map_data('PolishedPlayersHouse1F')
    game.set_hour(10);travel(game,town,house)
    game.snapshot('departure-first-bank')
    for accept in (False,True):
        game.restore('departure-first-bank')
        say(game,house);yesno(game,'PolishedMom_LeavingText1',accept)
        advance(game,lambda:not script_active(game))
        assert flag(game,fields()['open']) and flag(game,fields()['save'])==accept and balance(game)==0
        save_game(game);continue_game(game)
        say(game,house);assert text_matches(game,'PolishedMom_DoItText')
        close_resident_dialogue(game)
    travel(game,house,town);travel(game,town,house)
    start=money(game);assert start>=13
    bank_menu(game,house,1);amount(game,3,1)
    advance(game,lambda:text_matches(game,'PolishedMom_StoredMoneyText'))
    close_resident_dialogue(game)
    assert (balance(game),money(game))==(13,start-13)
    save_game(game);continue_game(game)
    assert (balance(game),money(game))==(13,start-13)
    bank_menu(game,house,0);amount(game,0,2)
    advance(game,lambda:text_matches(game,'PolishedMom_HaventSavedThatMuchText'))
    assert (balance(game),money(game))==(13,start-13)
    amount(game,6)
    advance(game,lambda:text_matches(game,'PolishedMom_TakenMoneyText'));close_resident_dialogue(game)
    assert (balance(game),money(game))==(7,start-7)
    bank_menu(game,house,1);amount(game,1,cancel=True);close_resident_dialogue(game)
    assert (balance(game),money(game))==(7,start-7)
    for yes in (False,True):
        bank_menu(game,house,2);yesno(game,'PolishedMom_SaveMoneyText',yes)
        close_resident_dialogue(game)
        assert flag(game,fields()['save'])==yes
    # A later DST change is one hour; the source refuses to cross midnight.
    game.set_hour(12)
    was=flag(game,fields()['dst']);say(game,house)
    yesno(game,'PolishedMom_IsThisAboutYourMoneyText',False)
    yesno(game,'PolishedMom_AskNotDSTText' if was else 'PolishedMom_AskDSTText',True)
    close_resident_dialogue(game)
    assert flag(game,fields()['dst'])!=was
    assert game.read(game.sym['gLocalTime']+2,1)[0]==(11 if was else 13)
    boundary=0 if flag(game,fields()['dst']) else 23
    game.set_hour(boundary);say(game,house)
    yesno(game,'PolishedMom_IsThisAboutYourMoneyText',False)
    yesno(game,'PolishedMom_AskAdjustDSTText',True)
    advance(game,lambda:text_matches(game,'PolishedMom_LostGearBookletText'));close_resident_dialogue(game)
    assert game.read(game.sym['gLocalTime']+2,1)[0]==boundary
    save_game(game);continue_game(game)
    assert balance(game)==7 and flag(game,fields()['save'])
    game.set_hour(10);travel(game,house,town)
    print('PASS: Mom offer, native transfers, retry, cancellation, DST, saving choice and Save/Continue',flush=True)


def portrait(game):
    ptr=game.u32(game.sym['gBattleStruct'])
    if not ptr:
        return False
    sid=game.read(ptr+layout()['trainer_sprites'],1)[0]
    if sid>=64:
        return False
    raw=game.read(game.sym['gSprites']+68*sid,68)
    if not raw[0x3e]&1 or raw[0x3e]&4 or struct.unpack_from('<h',raw,0x24)[0]:
        return False
    attr=struct.unpack_from('<H',raw,4)[0]
    tile=game.read(game.sym['gTrainerBackPic_PolishedLyra'],2048)
    pal=game.read(game.sym['gTrainerPalette_PolishedLyra'],32)
    return game.read(game.sym['gPlttBufferFaded']+(16+(attr>>12))*32,32)==pal and game.read(0x6010000+(attr&0x3ff)*32,2048)==tile


def tutorial(game,town,route,y,accept):
    walk(game,town,(0,y));game.step(LEFT);game.frames(100)
    route_walk(game,route,(54,y),[])
    before=party(game,details=True);n=balls(game,fields()['ball']);premier=balls(game,fields()['premier'])
    original_bag = inventory(game)
    wallet=money(game);name=game.read(game.u32(game.sym['gSaveBlock2Ptr']),8)
    game.step(LEFT);choice_prompt(game,'PolishedDeparture_CatchingTutorialIntroText')
    lyra=resident_id(route,'LOCALID_ROUTE29_LYRA')
    assert actors(game)[lyra]['position']==(52,y) and game.position()==(53,y)
    game.screenshot('departure-lyra-approach-'+str(y))
    game.press(A if accept else B,45)
    if accept:
        pic=bag=False
        for _ in range(700):
            if text_matches(game,'PolishedDeparture_CatchingTutorialDebriefText'):
                break
            if not pic and game.read(game.sym['gBattleTypeFlags'],4)[1]&2:
                if portrait(game):
                    pic=True;enemy=party(game,enemy=True,details=True)
                    assert [(m['species'],m['level']) for m in enemy]==[(fields()['pidgey'],5)],enemy
                    game.screenshot('departure-lyra-backpic')
            if not bag and task_active(game,'Task_WallyTutorialBagMenu'):
                assert balls(game,fields()['ball'])==10 and balls(game,fields()['premier'])==1
                bag=True;game.frames(28);game.screenshot('departure-lyra-balls')
            game.press(A,20)
        else:
            raise AssertionError(('Tutorial stalled',game.position(),pic,bag,game.u32(game.sym['gMain']+4)))
        assert pic and bag and game.read(game.sym['gBattleOutcome'],1)[0]==fields()['caught'],(pic,bag)
        assert party(game,details=True)==before and game.position()==(50,11)
        assert inventory(game) == original_bag
        assert balls(game,fields()['ball'])==n and balls(game,fields()['premier'])==premier
    else:
        advance(game,lambda:text_matches(game,'PolishedDeparture_CatchingTutorialRefusedText'))
        assert game.position()==(53,y)
        assert inventory(game) == original_bag
    advance(game,lambda:flag(game,fields()['done']) and not script_active(game))
    assert lyra not in actors(game) and party(game,details=True)==before
    assert name==game.read(game.u32(game.sym['gSaveBlock2Ptr']),8) and money(game)==wallet
    assert flag(game,fields()['decline'])!=accept and balls(game,fields()['ball'])==n+5
    save_game(game);continue_game(game)
    for row in (8,9):
        route_walk(game,route,(53,row),[])
        assert not script_active(game) and lyra not in actors(game)
    assert balls(game,fields()['ball'])==n+5 and flag(game,fields()['done'])
    print('PASS: Route 29 tutorial',y,'accept' if accept else 'decline',flush=True)
    return {
        'approach_row': y,
        'accepted': accept,
        'balls_before': n,
        'balls_after_continue': balls(game, fields()['ball']),
        'refusal_recorded': flag(game, fields()['decline']),
        'opponent_species': enemy[0]['species'] if accept else None,
        'opponent_level': enemy[0]['level'] if accept else None,
        'backpic_and_palette_in_vram': pic if accept else None,
        'tutorial_ball_pocket_checked': bag if accept else None,
        'party_preserved': party(game, details=True) == before,
    }


def check_departure(game,town,checks):
    game.command('save '+str(game.output/'departure-before-mom.state'))
    check_bank(game,town)
    checks.append('Mom honors both savings choices, stores and withdraws exact amounts, retries insufficient funds and preserves cash, balance and settings across Save/Continue')
    checks.append('Post-errand clock adjustment moves one hour in the chosen direction and prevents a date change at midnight')
    route=map_data('PolishedRoute29');game.snapshot('departure-before-lyra')
    branches = []
    for y,accept in ((8,False),(9,True),(8,True),(9,False)):
        game.restore('departure-before-lyra')
        branches.append(tutorial(game,town,route,y,accept))
        if y==8 and accept:
            game.snapshot('departure-taught')
    checks.append('Both Route 29 rows support accepting or declining; the native tutorial shows Lyra’s source backpic, her own Balls and Lv5 Pidgey, and preserves the real party and inventory')
    checks.append('Both tutorial outcomes award exactly five Poké Balls, record the refusal when chosen and persist without a replay after Save/Continue')
    game.restore('departure-taught')
    route_walk(game,route,(59,8),[]);game.step(RIGHT);game.frames(90)
    assert game.map_number()==map_id(game,town)
    for day in (1,2):
        game.timestamp+=(day-weekday(game))%7*86400
        game.command(f'rtc {game.timestamp}');game.refresh_clock()
        walk(game,town,(0,8));game.step(LEFT);game.frames(90)
        route_walk(game,route,(29,11),[])
        tuscany=resident_id(route,'LOCALID_ROUTE29_TUSCANY')
        assert (tuscany in actors(game))==(day==2),(day,weekday(game),actors(game))
        if day==2:
            count=count_item(game,fields()['scarf'])
            game.face(DOWN);game.press(A,100)
            advance(game,lambda:text_matches(game,'PolishedDeparture_TuscanyGaveGiftText'))
            game.screenshot('departure-tuscany');close_resident_dialogue(game)
            save_game(game);continue_game(game)
            game.face(DOWN);game.press(A,100)
            advance(game,lambda:text_matches(game,'PolishedDeparture_TuscanyTuesdayText'));close_resident_dialogue(game)
            assert count_item(game,fields()['scarf'])==count+1
            # While she is still on screen after the weekday changes, the
            # source gives her repeat greeting precedence over the weekday.
            game.timestamp += 86400
            game.command(f'rtc {game.timestamp}')
            game.refresh_clock()
            assert weekday(game) == 3
            game.face(DOWN)
            game.press(A, 100)
            advance(game, lambda: text_matches(game, 'PolishedDeparture_TuscanyTuesdayText'))
            close_resident_dialogue(game)
            assert count_item(game,fields()['scarf']) == count + 1
        route_walk(game,route,(59,8),[]);game.step(RIGHT);game.frames(90)
    checks.append('Tuscany is absent on Monday after the bank opens and gives one Silk Scarf on Tuesday; the gift stays collected across Save/Continue')
    report = {
        'tutorial_branches': branches,
        'bank_after_transfers': balance(game),
        'savings_enabled': flag(game, fields()['save']),
        'silk_scarves_after_continue': count_item(game, fields()['scarf']),
    }
    (game.output/'departure-branches.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: Tuscany weekday gate and one-time gift',flush=True)
