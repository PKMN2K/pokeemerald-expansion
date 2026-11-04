"""Route 30 and Oak, driven through real buttons with read-only RAM probes."""
from datetime import datetime, timezone
from functools import lru_cache
import re
import struct
import subprocess

from build_johto_demo import ROOT
from test_johto_demo_emulator import A, B, START, UP, DOWN, LEFT, RIGHT
from test_new_bark_emulator import map_data, walk, actors, close_resident_dialogue, save_game, continue_game, player_state
from test_new_bark_intro_emulator import layout, party, advance, task_active, script_active, text_matches, choice_prompt
from test_new_bark_battle_emulator import controller, choose_cell
from test_cherrygrove_emulator import map_id, flag, count_item, leave
from test_route29_emulator import route_walk


@lru_cache
def values():
    fields={'berries':'offsetof(struct SaveBlock1, bag.berries)','berry_count':'BAG_BERRIES_COUNT',
        'oran':'ITEM_ORAN_BERRY','pecha':'ITEM_PECHA_BERRY','mulch':'ITEM_POLISHED_MULCH',
        'egg':'ITEM_POLISHED_MYSTERY_EGG','antidote':'ITEM_ANTIDOTE','potion':'ITEM_POTION',
        'dex':'FLAG_SYS_POKEDEX_GET'}
    fields.update({s:'SPECIES_'+s.upper() for s in ('Ledyba','Caterpie','Weedle','Pidgey','Hoppip','Spinarak','Hoothoot','Poliwag','Zubat')})
    code='#include "global.h"\n#include "constants/items.h"\n#include "constants/flags.h"\nconst u32 values[] = {'+','.join(fields.values())+'};\n'
    r=subprocess.run(['arm-none-eabi-gcc','-mcpu=arm7tdmi','-mthumb','-mabi=apcs-gnu','-DMODERN=1','-DEMERALD',
        '-std=gnu17','-iquote','include','-iquote','.','-S','-x','c','-o','-','-'],cwd=ROOT,input=code,text=True,capture_output=True,check=True)
    return dict(zip(fields,map(int,re.findall(r'\.word\s+(\d+)',r.stdout))))


def state(game, offset=0xfd):
    return struct.unpack('<H',game.read(game.u32(game.sym['gSaveBlock1Ptr'])+layout()['vars']+2*offset,2))[0]


def berries(game,item):
    key=game.u32(game.u32(game.sym['gSaveBlock2Ptr'])+layout()['encryption_key'])&0xffff
    raw=game.read(game.u32(game.sym['gSaveBlock1Ptr'])+values()['berries'],values()['berry_count']*4)
    return sum(n^key for i,n in struct.iter_unpack('<HH',raw) if i==item)


def finish_wild(game,observed,label):
    seen=False
    for _ in range(400):
        if task_active(game,'Task_BattleStart'):
            game.frames(20);continue
        if game.u32(game.sym['gMain']+4)==game.sym['CB2_Overworld']|1:
            game.frames(30)
            if task_active(game,'Task_BattleStart') or game.u32(game.sym['gMain']+4)!=game.sym['CB2_Overworld']|1:
                continue
            return
        if controller(game,'HandleInputChooseAction'):
            if not seen:
                enemy=party(game,enemy=True,details=True)
                assert len(enemy)==1 and enemy[0]['level'] in (3,4),enemy
                hour=datetime.fromtimestamp(game.timestamp,timezone.utc).hour
                names=('Ledyba','Caterpie','Pidgey','Weedle','Hoppip') if 5<=hour<9 else ('Pidgey','Caterpie','Weedle','Hoppip') if 9<=hour<17 else ('Spinarak','Hoothoot','Poliwag','Zubat')
                assert enemy[0]['species'] in {values()[n] for n in names},(hour,enemy)
                observed.append(enemy[0]);seen=True
                game.screenshot('route30-wild-'+label)
            choose_cell(game,'gActionSelectionCursor',3)
        else:
            game.press(A,20)
    raise AssertionError('Route 30 wild battle did not finish')


def travel(game,route,pos,seen):
    route_walk(game,route,pos,seen,finish_wild)
    assert game.map_number()==map_id(game,route),'Unexpected defeat on Route 30'


def interact(game,route,pos,direction,seen,label=None):
    travel(game,route,pos,seen);game.face(direction);game.press(A,100)
    if label:
        advance(game,lambda:text_matches(game,label))
    assert script_active(game)


def pick(game,route,pos,seen,item):
    before=berries(game,item)
    interact(game,route,pos,UP,seen)
    advance(game,lambda:any(text_matches(game,'PolishedRoute29_FruitObtained'+n) for n in ('One','Two','Three')))
    close_resident_dialogue(game)
    assert 1<=berries(game,item)-before<=3


def check_route30(game,city,checks):
    route,house,mr=(map_data('Polished'+n) for n in ('Route30','Route30BerrySpeechHouse','MrPokemonsHouse'))
    seen=[];game.set_hour(10)
    game.command('save '+str(game.output/'route30-start.state'))
    walk(game,city,(17,0));game.step(UP);game.frames(100)
    assert game.map_number()==map_id(game,route) and game.position()==(7,53),(game.map_number(),game.position())
    game.screenshot('route30-south-entry')
    interact(game,route,(9,44),UP,seen,'PolishedRoute30_Route30SignText_Text')
    close_resident_dialogue(game)
    # Real gift, source use prompt (No then Yes), regeneration and saved state.
    travel(game,route,(7,40),seen);game.step(UP);game.frames(200)
    assert game.map_number()==map_id(game,house)
    walk(game,house,(2,4));game.face(UP);game.press(A,100)
    advance(game,lambda:text_matches(game,'PolishedRoute30BerrySpeechHouse_AfterText'))
    close_resident_dialogue(game)
    assert count_item(game,values()['mulch'])==1 and flag(game,0x4af)
    game.screenshot('route30-berry-master')
    save_game(game);continue_game(game)
    game.face(UP);game.press(A,100);close_resident_dialogue(game)
    assert count_item(game,values()['mulch'])==1
    leave(game,house,route)
    pick(game,route,(10,40),seen,values()['oran'])
    interact(game,route,(10,40),UP,seen)
    choice_prompt(game,'PolishedFruit_WantMulch')
    game.press(DOWN,12);game.press(A,60);advance(game,lambda:not script_active(game))
    assert count_item(game,values()['mulch'])==1
    interact(game,route,(10,40),UP,seen)
    choice_prompt(game,'PolishedFruit_WantMulch')
    game.press(A,60)
    advance(game,lambda:text_matches(game,'PolishedFruit_UsedMulch'))
    game.screenshot('route30-mulch-used');close_resident_dialogue(game)
    assert count_item(game,values()['mulch'])==0
    pick(game,route,(10,40),seen,values()['oran'])
    save_game(game);continue_game(game)
    before=berries(game,values()['oran'])
    interact(game,route,(10,40),UP,seen,'PolishedRoute29_FruitNothing');close_resident_dialogue(game)
    assert berries(game,values()['oran'])==before
    checks.append('Route 30 Berry Master gives Mulch once; No preserves it, Yes replenishes one picked tree, and harvest/gift state survives Save/Continue')
    print('PASS: Route 30 Mulch and daily fruit persistence',flush=True)
    before=count_item(game,values()['antidote'])
    interact(game,route,(9,35),LEFT,seen);close_resident_dialogue(game)
    assert count_item(game,values()['antidote'])==before+1 and flag(game,0x4ae)
    interact(game,route,(5,27),UP,seen,'PolishedRoute30_UseTackleText')
    advance(game,lambda:text_matches(game,'PolishedRoute30_BigBattleText'))
    game.screenshot('route30-joey-battle');close_resident_dialogue(game)
    assert actors(game)[1]['position']==(5,26) and actors(game)[2]['position']==(5,24) and actors(game)[3]['position']==(5,25)
    interact(game,route,(13,30),UP,seen,'PolishedRoute30_MrPokemonsHouseDirectionsSignText_Text');close_resident_dialogue(game)
    before=count_item(game,values()['potion'])
    # Approach sideways so turning north does not walk onto the hidden item.
    travel(game,route,(13,10),seen)
    interact(game,route,(14,10),UP,seen);close_resident_dialogue(game)
    assert count_item(game,values()['potion'])==before+1 and flag(game,0x4ad)
    pick(game,route,(11,6),seen,values()['pecha'])
    assert state(game,0xfb) and state(game,0xfc)
    travel(game,route,(17,6),seen)
    game.command('save '+str(game.output/'route30-before-mr-pokemon.state'))
    game.step(UP);game.frames(200)
    assert game.map_number()==map_id(game,mr)
    advance(game,lambda:text_matches(game,'PolishedMrPokemonsHouse_MrPokemonIntroText1'))
    game.screenshot('route30-mr-pokemon-welcome')
    advance(game,lambda:text_matches(game,'PolishedMrPokemonsHouse_MrPokemonIntroText3'))
    assert game.position()==(3,6),game.position()
    assert count_item(game,values()['egg'],True)==1
    game.screenshot('route30-mystery-egg')
    advance(game,lambda:text_matches(game,'PolishedMrPokemonsHouse_MrPokemonsHouse_OakText1'))
    assert actors(game)[2]['position']==(4,6)
    game.screenshot('route30-oak')
    advance(game,lambda:text_matches(game,'PolishedMrPokemonsHouse_ReceivedDex'),limit=400)
    assert flag(game,values()['dex']) and 3 not in actors(game)
    game.screenshot('route30-pokedex-gift')
    advance(game,lambda:state(game)==1 and not script_active(game),limit=400)
    assert 2 not in actors(game) and 3 not in actors(game)
    assert flag(game,0x20) and state(game,0xfe)==1
    assert all(m['hp']==m['max_hp'] and not m['status'] for m in party(game,details=True))
    assert not player_state(game)['frozen']
    save_game(game);continue_game(game)
    assert state(game)==1 and flag(game,values()['dex']) and count_item(game,values()['egg'],True)==1
    assert 2 not in actors(game) and 3 not in actors(game)
    # Open the real newly unlocked Pokédex from the native Start menu.
    game.press(START,40)
    count=game.read(game.sym['sNumStartMenuActions'],1)[0]
    actions=game.read(game.sym['sCurrentStartMenuActions'],count)
    dex=next(a for a in actions if game.u32(game.sym['sStartMenuItems']+a*8+4)==game.sym['StartMenuPokedexCallback']|1)
    for _ in range(count):
        cursor=game.read(game.sym['sStartMenuCursorPos'],1)[0]
        if actions[cursor]==dex:break
        game.press(DOWN,12)
    game.press(A,180);game.frames(240)
    assert game.u32(game.sym['gMain']+4)==game.sym['CB2_Pokedex']|1
    game.screenshot('route30-native-pokedex')
    for _ in range(12):
        game.press(B,60)
        if game.u32(game.sym['gMain']+4)==game.sym['CB2_Overworld']|1:break
    else:raise AssertionError('Pokédex did not return to field')
    # CB2_Overworld resumes before the returning Start menu accepts input.
    # Wait for its native input callback, then close it before walking.
    for _ in range(120):
        if task_active(game,'Task_ShowStartMenu') and game.u32(game.sym['gMenuCallback'])==game.sym['HandleStartMenuInput']|1:
            break
        game.frames(1)
    else:raise AssertionError('Returning Start menu did not become ready')
    game.press(B,60)
    assert not task_active(game,'Task_ShowStartMenu') and not player_state(game)['frozen']
    for index in (0,1):
        leave(game,mr,route,index)
        travel(game,route,(17,6),seen);game.step(UP);game.frames(200)
        assert not script_active(game) and state(game)==1 and count_item(game,values()['egg'],True)==1
    game.screenshot('route30-mr-pokemon-complete')
    leave(game,mr,route)
    checks.append('Route 30 Antidote, hidden Potion, Pecha tree and Joey battle animation work at source coordinates; both houses have reciprocal exits')
    checks.append('Mr. Pokémon grants one Mystery Egg; Oak follows the source choreography, grants a usable native Pokédex and departs; the full party heals')
    checks.append('Mystery Egg, Pokédex, Oak departure and return-errand state survive Save/Continue and repeated entry without duplicate gifts')
    travel(game,route,(7,53),seen);game.step(DOWN);game.frames(100)
    assert game.map_number()==map_id(game,city) and game.position()==(17,0)
    assert seen
    checks.append('Route 30 can be traversed to Mr. Pokémon and back through the source Cherrygrove connection with native wild battles')
    print('PASS: Route 30 Mystery Egg, Oak, native Pokédex, save/reentry and return connection',flush=True)
