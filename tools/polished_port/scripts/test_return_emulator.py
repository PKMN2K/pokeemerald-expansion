"""Return errand, driven with real input from the current ROM's campaign.

Read-only probes inspect the actual enemy party, native saves and object state.
Alternative starter branches begin at gameplay checkpoints made by check_intro.
"""
from functools import lru_cache
import json
import re
import subprocess
import struct

from build_johto_demo import ROOT, palette_rows
from build_new_bark import source_files
from test_johto_demo_emulator import A, B, START, UP, DOWN, LEFT, RIGHT
from test_new_bark_emulator import (map_data, walk, travel as door, actors,
    resident_id, save_game, continue_game, close_resident_dialogue, player_state)
from test_new_bark_intro_emulator import (layout, var, party, advance,
    script_active, text_matches, task_active, choice_prompt)
from test_new_bark_battle_emulator import controller, choose_cell, check_portrait
from test_cherrygrove_emulator import map_id, flag, scene, count_item, leave, finish_tour
from test_route29_emulator import route_walk
from test_route30_emulator import travel, state, values


@lru_cache
def constants():
    fields = {'name':'offsetof(struct SaveBlock2, polishedRivalName)',
        'rival':'FLAG_POLISHED_CHERRYGROVE_RIVAL_DONE',
        'theft':'FLAG_POLISHED_ELM_THEFT_REPORTED', 'egg':'FLAG_POLISHED_GAVE_MYSTERY_EGG',
        'quick':'MOVE_QUICK_ATTACK', 'joey':'TRAINER_POLISHED_JOEY', 'trainer_flags':'TRAINER_FLAGS_START',
        'icon':'OBJ_EVENT_GFX_POLISHED_RIVAL'}
    source = '#include "global.h"\n#include "constants/flags.h"\n#include "constants/opponents.h"\n#include "constants/event_objects.h"\nconst u32 values[] = {'+','.join(fields.values())+'};\n'
    r = subprocess.run(['arm-none-eabi-gcc','-mcpu=arm7tdmi','-mthumb','-mabi=apcs-gnu',
        '-DMODERN=1','-DEMERALD','-std=gnu17','-iquote','include','-iquote','.',
        '-S','-x','c','-o','-','-'],cwd=ROOT,input=source,text=True,capture_output=True,check=True)
    return dict(zip(fields,map(int,re.findall(r'\.word\s+(\d+)',r.stdout))))


def rival_name(game):
    return game.read(game.u32(game.sym['gSaveBlock2Ptr'])+constants()['name'],8).split(b'\xff',1)[0]


def battle(game, label, expected, lose=False, portrait=False):
    observed, portrait_seen = None, False
    switches = status_turns = 0
    for _ in range(3000):
        if portrait and not portrait_seen and observed is None:
            portrait_seen = check_portrait(game, 'return-'+label, 'rival1')
        if task_active(game,'Task_HandleChooseMonInput'):
            healthy = [i for i,m in enumerate(party(game,details=True)) if m['hp']]
            assert healthy
            target = healthy[0] if len(healthy)==1 else 1
            selected = game.read(game.sym['gPartyMenu']+9,1)[0]
            game.press(A if selected==target else RIGHT if target==1 else LEFT,30)
        elif controller(game,'PlayerHandleYesNoInput'):
            game.press(B,30)  # Decline the optional shift between enemy Pokemon.
        elif controller(game,'HandleInputChooseAction'):
            if observed is None:
                observed = party(game,enemy=True,details=True)
                assert [(m['species'],m['level'],m['item'],m['moves']) for m in observed] == expected, observed
                assert all(m['ivs']==[30]*6 and m['evs']==[0]*6 for m in observed),observed
                game.screenshot('return-'+label+'-first-turn')
            if lose and sum(bool(m['hp']) for m in party(game,details=True))>1:
                choose_cell(game,'gActionSelectionCursor',2)
                switches += 1
            else:
                choose_cell(game,'gActionSelectionCursor',0)
        elif controller(game,'HandleInputChooseMove'):
            if lose:
                assert game.read(game.sym['gBattleMons']+layout()['battle_pp'],4)[1], (label, switches, status_turns)
            choose_cell(game,'gMoveSelectionCursor',1 if lose else 0)
            status_turns += lose
        else:
            game.press(A,30)
        if observed and game.u32(game.sym['gMain']+4)==game.sym['CB2_Overworld']|1:
            break
    else:
        raise AssertionError(('Return battle stalled',label,party(game,details=True)))
    outcome = game.read(game.sym['gBattleOutcome'],1)[0]
    assert outcome==(2 if lose else 1),(label,outcome)
    assert not portrait or portrait_seen
    return {'party':observed,'outcome':outcome,'portrait_matches_source':portrait_seen if portrait else None,
            'switches':switches,'status_turns':status_turns}


def rival(game, city, y, label, lose=False):
    assert scene(game)==1 and not flag(game,constants()['rival'])
    guarded = {**city, 'warp_events':city['warp_events']+[{'x':33,'y':6},{'x':33,'y':7}]}
    walk(game,guarded,(32,y));game.step(RIGHT)
    advance(game,lambda:text_matches(game,'PolishedReturn_CherrygroveRivalText_Seen'))
    assert game.position()==(33,y)
    npc=resident_id(city,'LOCALID_CHERRYGROVE_RIVAL')
    assert actors(game)[npc]['position']==(34,y)
    game.screenshot('return-'+label+'-challenge')
    starter = {1:'Totodile',2:'Chikorita',3:'Cyndaquil'}[var(game,'starter')]
    data=layout()
    expected=[(data['Rattata'],4,0,[data['tackle'],data['leer'],constants()['quick'],0]),
        (data[starter],5,data['berry'],[data['scratch'] if starter=='Totodile' else data['tackle'],
            data['growl'] if starter=='Chikorita' else data['leer'],0,0])]
    report=battle(game,label,expected,lose,portrait=True)
    advance(game,lambda:text_matches(game,'PolishedReturn_CherrygroveRivalTextAfter1'))
    assert actors(game)[npc]['position']==(34,y) and not actors(game)[npc]['invisible']
    advance(game,lambda:text_matches(game,'PolishedReturn_CherrygroveRivalTextAfter2'))
    assert game.position()==(33,y+1) and actors(game)[npc]['position']==(33,y)
    game.screenshot('return-'+label+'-trainer-card')
    advance(game,lambda:scene(game)==2 and not script_active(game),limit=400)
    assert flag(game,constants()['rival']) and npc not in actors(game)
    assert game.position()==(33,y+1) and not player_state(game)['frozen']
    assert all(m['hp']==m['max_hp'] and m['status']==0 for m in party(game,details=True))
    game.screenshot('return-'+label+'-complete')
    report.update(player_starter=var(game,'starter'),enemy_starter=starter,approach_y=y)
    return report


def go_to_lab(game,city):
    route,town,lab=(map_data('Polished'+n) for n in ('Route29','NewBarkTown','ElmsLab'))
    walk(game,city,(39,6));game.step(RIGHT);game.frames(100)
    assert game.map_number()==map_id(game,route)
    route_walk(game,route,(59,8),[]);game.step(RIGHT);game.frames(100)
    assert game.map_number()==map_id(game,town)
    door(game,town,lab)
    assert game.map_number()==map_id(game,lab) and var(game,'lab_scene')==3
    return lab,town


def investigate(game,lab,x,custom):
    officer=resident_id(lab,'LOCALID_ELMS_LAB_OFFICER')
    lyra=resident_id(lab,'LOCALID_ELMS_LAB_LYRA')
    walk(game,lab,(x,6));game.step(UP)
    advance(game,lambda:text_matches(game,'PolishedReturn_ElmsLabOfficerText1'))
    assert game.position()==(4,3) and actors(game)[officer]['position']==(5,3)
    advance(game,lambda:text_matches(game,'PolishedReturn_ElmsLabLyraTheftInnocentText'))
    assert actors(game)[lyra]['position']==(5,4)
    game.screenshot('return-investigation-'+str(x))
    advance(game,lambda:game.u32(game.sym['gMain']+4)==game.sym['CB2_NamingScreen']|1)
    game.frames(240)
    # The native screen uses the source rival overworld sprite, not FRLG's rival.
    info=game.u32(game.sym['gObjectEventGraphicsInfoPointers']+constants()['icon']*4)
    images=game.u32(info+28)
    sprites=b''.join(game.read(game.sym['gSprites']+i*68,16*68) for i in range(0,64,16))
    sprite, = [sprites[i:i+68] for i in range(0,len(sprites),68) if sprites[i+0x3e]&1 and struct.unpack_from('<I',sprites,i+12)[0]==images]
    bank=struct.unpack_from('<H',sprite,4)[0]>>12
    red=palette_rows(source_files()['gfx/overworld/npc_sprites.pal'].decode())[20]
    assert game.read(game.sym['gPlttBufferFaded']+(16+bank)*32,32)==struct.pack('<16H',*red,*([0]*12))
    game.screenshot('return-rival-naming-'+str(x))
    if custom:
        game.press(A,120)
    game.press(START,30);game.press(A,180)
    advance(game,lambda:text_matches(game,'PolishedReturn_ElmsLabOfficerText3'))
    expected=b'\xbb' if custom else b'\xcd\xdd\xe0\xea\xd9\xe6' # A / Silver
    assert rival_name(game)==expected, rival_name(game)
    assert expected in game.read(game.sym['gStringVar4'],1024).split(b'\xff',1)[0]
    advance(game,lambda:text_matches(game,'PolishedReturn_ElmAfterTheftText3'))
    assert flag(game,constants()['theft']) and count_item(game,values()['egg'],True)==0
    assert officer not in actors(game) and lyra not in actors(game)
    assert actors(game)[resident_id(lab,'LOCALID_ELMS_LAB_ELM')]['position']==(5,1)
    game.screenshot('return-elm-egg-reaction-'+str(x))
    advance(game,lambda:flag(game,constants()['egg']) and not script_active(game))
    assert var(game,'lab_scene')==2 and game.position()==(5,3)
    assert actors(game)[resident_id(lab,'LOCALID_ELMS_LAB_ELM')]['position']==(5,2)
    save_game(game);continue_game(game)
    assert rival_name(game)==expected and count_item(game,values()['egg'],True)==0
    assert all(flag(game,constants()[f]) for f in ('rival','theft','egg'))
    assert officer not in actors(game) and lyra not in actors(game) and not script_active(game)
    game.face(UP);game.press(A,100)
    advance(game,lambda:text_matches(game,'PolishedReturn_ElmStudyingEggText'))
    close_resident_dialogue(game)
    walk(game,lab,(5,1));game.face(UP);game.press(A,100)
    advance(game,lambda:text_matches(game,'PolishedReturn_ElmsLabWindowText2'))
    close_resident_dialogue(game)
    return {'approach_x':x,'rival_name_bytes':expected.hex(),'saved_and_reloaded':True}


def go_to_city(game,town):
    route,city=(map_data('Polished'+n) for n in ('Route29','CherrygroveCity'))
    walk(game,town,(0,8));game.step(LEFT);game.frames(100)
    route_walk(game,route,(0,6),[]);game.step(LEFT);game.frames(100)
    assert game.map_number()==map_id(game,city)
    return city


def complete_outbound(game,city):
    route,mr=(map_data('Polished'+n) for n in ('Route30','MrPokemonsHouse'))
    walk(game,city,(34,7));game.step(LEFT)
    advance(game,lambda:text_matches(game,'PolishedCherrygrove_GuideGentIntroText'))
    finish_tour(game)
    walk(game,city,(17,0));game.step(UP);game.frames(100)
    travel(game,route,(17,6),[]);game.step(UP);game.frames(200)
    advance(game,lambda:state(game)==1 and not script_active(game),limit=500)
    leave(game,mr,route)
    advance(game,lambda:state(game,0xfe)==2 and not script_active(game))
    travel(game,route,(7,53),[]);game.step(DOWN);game.frames(100)
    assert game.map_number()==map_id(game,city)


def check_return(game,city,checks):
    game.command('save '+str(game.output/'return-before-rival.state'))
    game.snapshot('return-before-rival')
    reports=[]
    for y,label,lose in ((6,'north-win',False),(7,'south-win',False),(7,'south-loss',True)):
        game.restore('return-before-rival')
        reports.append(rival(game,city,y,label,lose))
        save_game(game);continue_game(game)
        assert scene(game)==2 and flag(game,constants()['rival'])
        for pos in ((33,6),(33,7)):
            walk(game,city,pos)
            assert not script_active(game)
        if not lose:
            game.snapshot('return-after-rival')
        print('PASS: return rival '+label,flush=True)
    checks.append('Cherrygrove rival uses the source two-Pokemon party and authentic portrait; both approach rows, victory and genuine full-party defeat continue, heal and save without a repeat')
    game.restore('return-after-rival')
    lab,town=go_to_lab(game,city)
    game.snapshot('return-before-cop')
    game.command('save '+str(game.output/'return-before-cop.state'))
    investigations=[]
    for x in (4,5):
        game.restore('return-before-cop')
        investigations.append(investigate(game,lab,x,custom=x==4))
        door(game,lab,town);door(game,town,lab)
        walk(game,lab,(5,3))
        assert var(game,'lab_scene')==2 and not script_active(game)
        assert count_item(game,values()['egg'],True)==0
        print('PASS: theft, rival name and Mystery Egg, approach',x,flush=True)
    checks.append('Both lab approaches run the cop/Lyra investigation, support custom and default rival names, remove the Mystery Egg once and preserve all progress through native Save/Continue and reentry')
    # Walk back out after the handoff. Joey must now replace the battle tableau.
    door(game,lab,town)
    walk(game,town,(6,9));game.face(UP);game.press(A,100)
    advance(game,lambda:text_matches(game,'PolishedReturn_Text_TellMomIfLeaving'))
    choice_prompt(game,'PolishedNewBarkTown_Garden');game.press(B,60)
    advance(game,lambda:not script_active(game))
    from test_post_egg_emulator import check_departure, balance
    from test_cherrygrove_emulator import money
    check_departure(game,town,checks)
    go_to_city(game,town)
    walk(game,city,(17,0));game.step(UP);game.frames(100)
    route=map_data('PolishedRoute30')
    travel(game,route,(3,29),[])
    game.step(UP)
    advance(game,lambda:text_matches(game,'PolishedReturn_JoeySeen'))
    assert not any(n in actors(game) for n in (1,2,3))
    data=layout()
    bank_before, wallet_before = balance(game), money(game)
    joey=battle(game,'joey',[(data['Rattata'],5,0,[data['tackle'],data['leer'],constants()['quick'],0])])
    advance(game,lambda:not script_active(game))
    save_game(game);continue_game(game)
    assert flag(game,constants()['trainer_flags']+constants()['joey'])
    saved = balance(game) - bank_before
    earned = money(game) - wallet_before + saved
    assert saved == earned // 4 and saved > 0, (saved,earned)
    checks.append(f'Mom keeps {saved} of Joey’s {earned} prize money; wallet and savings survive native Save/Continue')
    # Re-cross his sight after saving: beaten trainer must not restart a battle.
    walk(game,route,(3,29));walk(game,route,(3,28));game.frames(90)
    assert not task_active(game,'Task_BattleStart') and not script_active(game)
    game.screenshot('return-joey-defeated')
    checks.append('Delivering the Egg removes Joey/Pidgey/Rattata staging and activates the source Lv5 Joey battle; victory persists through Save/Continue')
    for species in ('Totodile','Chikorita'):
        game.command('load '+str(game.output/('return-starter-'+species.lower()+'.state')))
        game.frames(2);game.set_hour(10)
        door(game,lab,town);go_to_city(game,town)
        complete_outbound(game,city)
        reports.append(rival(game,city,6,species.lower()))
        print('PASS: return rival starter branch',species,flush=True)
    checks.append('All three real starter selections complete Mr. Pokemon and select the matching stolen-starter rival party without RAM edits')
    (game.output/'return-branches.json').write_text(json.dumps({'rivals':reports,'investigation':investigations,'joey':joey},indent=2)+'\n')
    game.restore('return-after-rival')
