#!/usr/bin/env python3
"""Controller fixtures linked to unchanged production Johto objects.

The isolated boot fixture supplies a party at each rock; it is not campaign
progression evidence. No gameplay RAM or functions are patched. Production ROM
campaign replays remain separate checks.
"""
from collections import deque
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from test_johto_demo_emulator import Game, ROOT, A, B, UP, DOWN, RIGHT, LEFT, START
from test_cherrygrove_emulator import map_id
from test_new_bark_emulator import actors, save_game, map_data
from test_new_bark_intro_emulator import choice_prompt, advance, text_matches, script_active, task_active, party
from test_new_bark_battle_emulator import controller, choose_cell
from test_violet_mart_emulator import count
COMPILER='arm-none-eabi-gcc'


def constants():
    fields={'vars':'offsetof(struct SaveBlock1, vars)', 'flags':'offsetof(struct SaveBlock1, flags)',
            'sprite_size':'sizeof(struct Sprite)', 'sprite_oam':'offsetof(struct Sprite, oam)', 'active':'VAR_TEMP_5', 'brick':'ITEM_POLISHED_BRICK_PIECE', 'tm':'ITEM_TM_BRICK_BREAK',
            'krabby':'SPECIES_KRABBY','geodude':'SPECIES_GEODUDE','shuckle':'SPECIES_SHUCKLE',
            'rock_move':'FIELD_MOVE_ROCK_SMASH','party_actions':'MENU_FIELD_MOVES'}
    party_source=(ROOT/'src/party_menu.c').read_text()
    declarations=re.search(r'enum \{\s*MENU_SUMMARY,.*?\n\};',party_source,re.S)[0]+'\n'+re.search(r'struct PartyMenuInternal\n\{.*?\n\};',party_source,re.S)[0]+'\n'
    fields.update(actions_offset='offsetof(struct PartyMenuInternal, actions)',count_offset='offsetof(struct PartyMenuInternal, numActions)')
    code='#include "global.h"\n#include "task.h"\n#include "sprite.h"\n#include "main.h"\n#include "constants/vars.h"\n#include "constants/field_move.h"\n'+declarations+'const u32 values[]={'+','.join(fields.values())+'};\n'
    r=subprocess.run(['arm-none-eabi-gcc','-mcpu=arm7tdmi','-mthumb','-mabi=apcs-gnu','-DMODERN=1','-DEMERALD','-std=gnu17','-iquote','include','-iquote','.','-S','-x','c','-o','-','-'],cwd=ROOT,input=code,text=True,capture_output=True,check=True)
    return dict(zip(fields,map(int,re.findall(r'\.word\s+(\d+)',r.stdout)),strict=True))


def fixture(directory, mode, index):
    elf=directory/'rocks.elf'; obj=directory/'fixture.o'; linker=directory/'fixture.ld'
    subprocess.run([COMPILER,'-c','-Wa,-meabi=5','-mthumb','-mthumb-interwork','-O2','-mabi=apcs-gnu','-march=armv4t','-std=gnu17','-DMODERN=1','-DTESTING=0','-DEMERALD','-DPOLISHED_JOHTO_DEMO=1',f'-DFIXTURE_MODE={mode}',f'-DFIXTURE_INDEX={index}','-Wall','-Werror','-iquote','include','-iquote','.',str(ROOT/'tools/polished_port/tests/rock_fixture.c'),'-o',str(obj)],cwd=ROOT,check=True)
    linker.write_text((ROOT/'ld_script_modern.ld').read_text().replace('gInitialMainCB2 = CB2_InitCopyrightScreenAfterBootup;','gInitialMainCB2 = CB2_PolishedRockFixture;'))
    objects=[line[5:] for line in (ROOT/'pokeemerald-johto-demo.map').read_text().splitlines() if line.startswith('LOAD ') and line!='LOAD linker stubs']
    subprocess.run(['arm-none-eabi-ld','--gc-sections','-T',str(linker),'-o',str(elf),str(obj),*objects],cwd=ROOT/'build/emerald-johto-demo',check=True)
    subprocess.run([str(ROOT/'tools/gbafix/gbafix'),str(elf),'-tPOKEMON EMER','-cBPEE','-m01','-r0','-d0','--silent'],check=True)
    return elf


def active(game,c):
    return struct.unpack('<H',game.read(game.u32(game.sym['gSaveBlock1Ptr'])+c['vars']+2*(c['active']-0x4000),2))[0]


def idle(game):
    return game.u32(game.sym['gMain']+4)==game.sym['CB2_Overworld']|1 and not script_active(game) and not game.read(game.sym['sLockFieldControls'],1)[0]


def boot(game):
    for _ in range(100):
        game.frames(20)
        if idle(game): return
    raise AssertionError('Rock fixture did not reach the field')


def bag_contents(game):
    chunks=[]
    for pocket in range(5):
        pointer,capacity=struct.unpack('<IH',game.read(game.sym['gBagPockets']+pocket*8,6))
        chunks.append(game.read(pointer,(capacity&0x3ff)*4))
    return b''.join(chunks)


def verify_brick_icon(game,c):
    from PIL import Image
    sprite_id=game.read(game.sym['sRockItemSpritePlusOne'],1)[0]-1
    if sprite_id<0:return False
    attribute=int.from_bytes(game.read(game.sym['gSprites']+sprite_id*c['sprite_size']+c['sprite_oam']+4,2),'little')
    tiles=game.read(0x6010000+(attribute&0x3ff)*32,512)
    expected=Image.open(ROOT/'graphics/items/icons/polished_brick_piece.png')
    for y in range(32):
        for x in range(32):
            byte=tiles[((y//8)*4+x//8)*32+(y%8)*4+(x%8)//2]
            pixel=(byte>>(4*(x&1)))&15
            assert pixel==(expected.getpixel((x,y)) if x<24 and y<24 else 0),(x,y,pixel)
    colors=json.loads((ROOT/'tools/polished_port/manifests/rock_smash.json').read_text())['brick_piece']['source_rgb555']
    palette=[0x7fff]+[r|(g<<5)|(b<<10) for r,g,b in colors]+[0]*13
    actual=list(struct.unpack('<16H',game.read(game.sym['gPlttBufferUnfaded']+(16+(attribute>>12))*32,32)))
    assert actual==palette,(actual,palette)
    return True


def resolve(game,c,expected_ask=False):
    encounter=None; used=False; picture=False; seen_prompt=False; animation=[]; reward=None; full=False; icon_verified=False
    for _ in range(1800):
        if idle(game):
            assert game.read(game.sym['sRockItemSpritePlusOne'],1)[0]==0, 'Item sprite was not released'
            return dict(icon_verified=icon_verified,encounter=encounter,used=used,picture=picture,prompt=seen_prompt,animation=animation,reward=reward,full=full)
        if task_active(game,'Task_HandleYesNoInput'):
            assert expected_ask, 'Automatic smash unexpectedly asked again'
            seen_prompt=True;game.press(A,30);continue
        if game.u32(game.sym['gMain']+4)!=game.sym['CB2_Overworld']|1:
            if controller(game,'HandleInputChooseAction'):
                current=party(game,enemy=True)
                assert len(current)==1 and current[0]['level']==15 and current[0]['species'] in [c[n] for n in ('krabby','geodude','shuckle')],current
                assert not game.u32(game.sym['gBattleTypeFlags']) & ((1<<0)|(1<<3)|(1<<6)|(1<<13))
                encounter=current[0];game.screenshot('rock-encounter')
                choose_cell(game,'gActionSelectionCursor',3) # Run: ordinary wild battle
            else: game.press(B,20)
            continue
        used |= text_matches(game,'PolishedRockSmash_UsedText')
        if text_matches(game,'PolishedRockSmash_GainedText') or text_matches(game,'PolishedRockSmash_FullText'):
            reward=int.from_bytes(game.read(game.sym['gSpecialVar_0x8000'],2),'little')
            full |= text_matches(game,'PolishedRockSmash_FullText')
        if text_matches(game,'PolishedRockSmash_PutAwayText') and reward is not None:
            if reward==c['brick']: icon_verified |= verify_brick_icon(game,c)
            game.screenshot('reward-'+str(reward))
        if full: game.screenshot('full-pocket')
        picture |= task_active(game,'Task_PokemonPicWindow') if 'Task_PokemonPicWindow' in game.sym else False
        if task_active(game,'AnimateTask'):
            animation.append({k:v['invisible'] for k,v in actors(game).items() if 5<=k<=8})
            game.frames(1)
        else: game.press(A,15)
    raise AssertionError(('Rock resolution stalled',game.position(),party(game,details=True)))


def walk(game,data,goal):
    # The metatile's behavior can also block a cell whose elevation/collision
    # bits permit movement. Learn those barriers from normal attempted steps.
    rejected=set()
    for _ in range(400):
        start=game.position()
        if start==goal:return
        blocked=rejected | {(w['x'],w['y']) for w in data['warp_events']} | {a['position'] for a in actors(game).values()}
        blocked.discard(goal)
        queue=deque([start]);paths={start:[]}
        while queue and goal not in paths:
            x,y=queue.popleft()
            for key,dx,dy in [(UP,0,-1),(DOWN,0,1),(LEFT,-1,0),(RIGHT,1,0)]:
                pos=x+dx,y+dy
                if pos in paths or pos in blocked or not(0<=pos[0]<data['width'] and 0<=pos[1]<data['height']):continue
                cell=data['cells'][pos[1]*data['width']+pos[0]]
                if cell&0xc00 or cell>>12!=3:continue
                paths[pos]=paths[x,y]+[(key,pos)];queue.append(pos)
        assert goal in paths, ('No walking route',start,goal,rejected)
        key,pos=paths[goal][0]
        try:game.step(key)
        except AssertionError as e:
            if not str(e).startswith('Blocked at'):raise
            rejected.add(pos)
        game.frames(10)
    raise AssertionError(('Walking budget exceeded',game.position(),goal))


def party_menu(game,c,use):
    game.press(START,40)
    n=game.read(game.sym['sNumStartMenuActions'],1)[0]
    actions=game.read(game.sym['sCurrentStartMenuActions'],n)
    desired=next(i for i in actions if game.u32(game.sym['sStartMenuItems']+i*8+4)==game.sym['StartMenuPokemonCallback']|1)
    for _ in range(n):
        if actions[game.read(game.sym['sStartMenuCursorPos'],1)[0]]==desired: break
        game.press(DOWN,10)
    game.press(A,150);game.press(A,90)
    assert task_active(game,'Task_HandleSelectionMenuInput')
    base=game.u32(game.sym['sPartyMenuInternal'])
    n=game.read(base+c['count_offset'],1)[0]
    actions=list(game.read(base+c['actions_offset'],n))
    target=c['party_actions']+c['rock_move']
    assert (target in actions)==use, actions
    game.screenshot('party-options')
    if use:
        for _ in range(actions.index(target)): game.press(DOWN,12)
        game.press(A,45)
    else:
        game.press(B,30);game.press(B,150);game.press(B,30)
        assert idle(game)


def run_case(driver,out,mode,index,c):
    out.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp_name:
        tmp=Path(tmp_name);elf=fixture(tmp,mode,index)
        game=Game(driver,elf,elf,out,tmp)
        try:
            boot(game);local=5+index;initial_bag=bag_contents(game)
            if mode==4:
                assert active(game,c)==1
                cave=map_data('PolishedDarkCaveVioletEntrance');route=map_data('PolishedRoute31')
                game.frames(20,DOWN);game.frames(200)
                assert game.map_number()==map_id(game,route)
                warp=next(w for w in route['warp_events'] if w['dest_map']==cave['id'])
                walk(game,route,(warp['x'],warp['y']+1));game.frames(20,UP);game.frames(200)
                assert game.map_number()==map_id(game,cave) and active(game,c)==0
                assert 7 in actors(game), 'Rock did not respawn after reentry'
                game.screenshot('reentry-reset')
                return {'mode':mode,'index':index,'reentry':True}
            assert local in actors(game) and active(game,c)==0
            game.face(RIGHT if index==0 else UP);game.press(A,20)
            if mode==2:
                advance(game,lambda:text_matches(game,'PolishedRockSmash_UnavailableText'))
                game.screenshot('unavailable')
                advance(game,lambda:idle(game))
                assert local in actors(game) and active(game,c)==0
                return {'mode':mode,'index':index,'unavailable':True}
            choice_prompt(game,'PolishedRockSmash_AskText');game.screenshot('ask')
            game.press(B,40)
            assert idle(game) and local in actors(game) and active(game,c)==0
            if mode==0 and index==0:
                party_menu(game,c,True)
            else:
                if mode==1: party_menu(game,c,False)
                game.press(A,20);choice_prompt(game,'PolishedRockSmash_AskText')
                game.snapshot('before-confirm')
                game.press(A,20)
            result=resolve(game,c)
            if mode==3 or (mode==0 and index==1):
                for delay in range(1,61):
                    satisfied=result['full'] if mode==3 else result['reward']==c['brick']
                    if satisfied: break
                    game.restore('before-confirm');game.frames(delay)
                    game.press(A,20);result=resolve(game,c)
                else: raise AssertionError('Required reward branch not observed within controller replay budget')
                if mode==3:
                    assert result['full'] and result['reward'] is not None and result['encounter'] is None
                    assert bag_contents(game)==initial_bag, 'Full-pocket reward changed inventory'
                else:
                    assert count(game,c['brick'])==1 and not result['full'] and result['icon_verified']
            assert local not in actors(game) and active(game,c)==1
            assert result['used'] and result['picture'], result
            assert [a.get(str(local),a.get(local)) for a in result['animation'][-10:]]==[False,True]*5, result['animation']
            game.screenshot('broken')
            if mode==1: assert count(game,c['tm'])==1, 'Owned TM was consumed'
            save_game(game)
            game.command('reset');boot(game)
            assert active(game,c)==1 and local not in actors(game), 'Continue reset a broken rock or active flag'
            game.screenshot('continued')
            if mode==0 and index==2:
                # Both nearby rocks can be reached on foot from this source entrance.
                cave=map_data('PolishedDarkCaveVioletEntrance')
                walk(game,cave,(15,14));game.face(RIGHT);game.press(A,1)
                repeated=resolve(game,c)
                assert 5 not in actors(game) and not repeated['used'] and not repeated['picture']
                result['repeat']=repeated
            result.update(mode=mode,index=index,saved=True)
            return result
        except BaseException:
            game.screenshot('failure');game.snapshot('failure')
            import shutil
            shutil.copy2(elf,out/'failure.elf');shutil.copy2(tmp/'failure.state',out/'failure.state')
            raise
        finally:game.close()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--driver',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--quick',action='store_true');args=p.parse_args()
    c=constants();results=[]
    for mode,index in ([(0,2)] if args.quick else [(0,i) for i in range(4)]+[(1,2),(2,2),(3,2),(4,2)]):
        result=run_case(args.driver.resolve(),args.output.resolve()/f'{mode}-{index}',mode,index,c)
        results.append(result);print(f'PASS: rock fixture {mode}/{index}; battle={result.get("encounter")}; save={result.get("saved")}',flush=True)
    args.output.mkdir(parents=True,exist_ok=True)
    (args.output/'report.json').write_text(json.dumps(dict(production_sha256=hashlib.sha256((ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),fixture_only=True,results=results),indent=2)+'\n')
if __name__=='__main__':main()
