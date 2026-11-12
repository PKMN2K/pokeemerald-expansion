#!/usr/bin/env python3
"""Controller-only TM70 lessons against unchanged production Johto objects."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import test_flash_emulator as flash
import test_rock_smash_emulator as rock
from test_johto_demo_emulator import Game, ROOT, A, B, UP, DOWN, RIGHT, START
from test_new_bark_intro_emulator import layout, party, task_active, advance, text_matches
from test_violet_mart_emulator import select_bag_item, count
from test_new_bark_emulator import save_game
from test_academy_emulator import wait_for
COMPILER = 'arm-none-eabi-gcc'


def constants():
    c=rock.constants()
    fields={'tm':'ITEM_TM70', 'toxic_tm':'ITEM_TM_TOXIC', 'flash':'MOVE_FLASH', 'toxic':'MOVE_TOXIC',
            'bag_sprites':'offsetof(struct BagMenu, spriteIds)', 'item_sprite':'ITEMMENUSPRITE_ITEM',
            'friendship':'offsetof(struct PokemonSubstruct0, friendship)',
            'bonuses':'offsetof(struct PokemonSubstruct0, ppBonuses)', 'party_slot':'offsetof(struct PartyMenu, slotId)'}
    code='#include "global.h"\n#include "item_menu.h"\n#include "party_menu.h"\nconst u32 values[]={'+','.join(fields.values())+'};'
    r=subprocess.run(['arm-none-eabi-gcc','-mcpu=arm7tdmi','-mthumb','-mabi=apcs-gnu','-DMODERN=1','-DEMERALD','-std=gnu17','-iquote','include','-iquote','.','-S','-x','c','-o','-','-'],cwd=ROOT,input=code,text=True,capture_output=True,check=True)
    c.update(zip(fields,map(int,re.findall(r'\.word\s+(\d+)',r.stdout)),strict=True))
    return c


def fixture(directory,mode):
    elf=directory/'tm70.elf';obj=directory/'fixture.o';linker=directory/'fixture.ld'
    subprocess.run([COMPILER,'-c','-Wa,-meabi=5','-mthumb','-mthumb-interwork','-O2','-mabi=apcs-gnu','-march=armv4t','-std=gnu17','-DMODERN=1','-DTESTING=0','-DEMERALD','-DPOLISHED_JOHTO_DEMO=1',f'-DFIXTURE_MODE={mode}','-Wall','-Werror','-iquote','include','-iquote','.',str(ROOT/'tools/polished_port/tests/tm70_fixture.c'),'-o',str(obj)],cwd=ROOT,check=True)
    linker.write_text((ROOT/'ld_script_modern.ld').read_text().replace('gInitialMainCB2 = CB2_InitCopyrightScreenAfterBootup;','gInitialMainCB2 = CB2_PolishedTM70Fixture;'))
    objects=[line[5:] for line in (ROOT/'pokeemerald-johto-demo.map').read_text().splitlines() if line.startswith('LOAD ') and line!='LOAD linker stubs']
    subprocess.run(['arm-none-eabi-ld','--gc-sections','-T',str(linker),'-o',str(elf),str(obj),*objects],cwd=ROOT/'build/emerald-johto-demo',check=True)
    subprocess.run([str(ROOT/'tools/gbafix/gbafix'),str(elf),'-tPOKEMON EMER','-cBPEE','-m01','-r0','-d0','--silent'],check=True)
    return elf


def details(game,c):
    result=party(game,details=True);d=layout();base=game.u32(game.sym['gPlayerPartyPtr'])
    for i,mon in enumerate(result):
        raw=game.read(base+i*d['mon_size'],d['mon_size']);personality,trainer=struct.unpack_from('<II',raw)
        secure=raw[d['secure']:d['secure']+4*d['substruct_size']]
        decoded=b''.join(struct.pack('<I',v^personality^trainer) for v in struct.unpack(f'<{len(secure)//4}I',secure))
        offset=game.read(game.sym['sSubstructOffsets']+personality%24,1)[0]*d['substruct_size']
        mon['friendship']=decoded[offset+c['friendship']];mon['bonuses']=decoded[offset+c['bonuses']]
    return result


def icon(game,c):
    from PIL import Image
    sprites=game.read(game.u32(game.sym['gBagMenu'])+c['bag_sprites']+c['item_sprite'],2)
    ids=[s for s in sprites if s<64]
    assert len(ids)==1, list(sprites)
    attr=int.from_bytes(game.read(game.sym['gSprites']+ids[0]*c['sprite_size']+c['sprite_oam']+4,2),'little')
    tiles=game.read(0x6010000+(attr&0x3ff)*32,512)
    expected=Image.open(ROOT/'graphics/items/icons/polished_flash_tm.png')
    for y in range(32):
        for x in range(32):
            byte=tiles[((y//8)*4+x//8)*32+(y%8)*4+(x%8)//2]
            assert (byte>>(4*(x&1)))&15 == (expected.getpixel((x,y)) if x<24 and y<24 else 0),(x,y)
    colors=json.loads((ROOT/'tools/polished_port/manifests/tm70.json').read_text())['source_rgb555']
    expected_palette=[0x7fff]+[r|(g<<5)|(b<<10) for r,g,b in colors]+[0]*13
    assert list(struct.unpack('<16H',game.read(game.sym['gPlttBufferUnfaded']+(16+(attr>>12))*32,32)))==expected_palette


def open_bag(game):
    game.press(START);actions=game.read(game.sym['sCurrentStartMenuActions'],9)
    for _ in range(9):
        if game.read(game.sym['sStartMenuCursorPos'],1)[0]==actions.index(2):break
        game.press(DOWN)
    game.press(A,160)
    wait_for(game,lambda:task_active(game,'Task_BagMenu_HandleInput'),'field Bag')


def use(game,c,item,slot=0,check_icon=False):
    select_bag_item(game,item,pocket=2)
    if check_icon:
        icon(game,c);game.screenshot('tm70-bag')
    game.press(A,80)
    wait_for(game,lambda:task_active(game,'Task_ShowTMHMContainedMessage'),'TM boot text')
    if item==c['tm']:assert text_matches(game,'sText_BootedUpTM'), 'TM70 booted as HM'
    game.press(A,90)
    advance(game,lambda:task_active(game,'Task_HandleChooseMonInput'))
    game.frames(50)
    for _ in range(6):
        current=game.read(game.sym['gPartyMenu']+c['party_slot'],1)[0]
        if current==slot:break
        game.press(DOWN if current<slot else UP)
    assert game.read(game.sym['gPartyMenu']+c['party_slot'],1)[0]==slot
    game.press(A,60)


def finish(game):
    advance(game,lambda:task_active(game,'Task_BagMenu_HandleInput'),limit=350)
    game.frames(60)


def close(game):
    for _ in range(30):
        if rock.idle(game):return
        game.press(B,40)
    raise AssertionError('Bag did not return to field')


def run(driver,out,mode,c):
    out.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory() as directory:
        tmp=Path(directory);elf=fixture(tmp,mode);game=Game(driver,elf,elf,out,tmp)
        try:
            rock.boot(game);before=details(game,c);bag=flash.inventory(game)
            open_bag(game);use(game,c,c['tm'],check_icon=True)
            if mode in (2,3,4):
                # Eggs remain on the selector instead of opening a lesson.
                if mode==4:
                    assert task_active(game,'Task_HandleChooseMonInput')
                    assert details(game,c)==before and flash.inventory(game)==bag
                    game.press(B,100)
                finish(game);assert details(game,c)==before
                assert flash.inventory(game)==bag
                close(game)
                return dict(mode=mode,rejected=True,icon_verified=True)
            if mode in (1,5):
                advance(game,lambda:task_active(game,'Task_HandleReplaceMoveInput'))
                game.frames(50);game.screenshot('replace-menu')
                if mode==5:
                    game.press(B,50);finish(game)
                    assert details(game,c)==before and flash.inventory(game)==bag
                    return dict(mode=mode,cancelled=True,icon_verified=True)
                game.press(A,80)
            finish(game)
            after=details(game,c);slot=0 if mode==1 else 1
            expected=dict(before[0]);expected['moves']=before[0]['moves'].copy();expected['moves'][slot]=c['flash']
            expected['pp']=before[0]['pp'].copy();expected['pp'][slot]=20
            expected['bonuses']=before[0]['bonuses'] & ~(3<<(slot*2));expected['friendship']=162
            assert after[0]==expected,(after[0],expected)
            assert after[1]==before[1] and flash.inventory(game)==bag
            game.screenshot('learned')
            # Reuse the same owned copy on another Pokemon; high friendship has
            # zero base change and no bonus items/ball in this fixture.
            use(game,c,c['tm'],slot=1)
            if mode==1:
                advance(game,lambda:task_active(game,'Task_HandleReplaceMoveInput'));game.frames(50);game.press(A,80)
            finish(game);after=details(game,c)
            assert after[1]['moves'][slot]==c['flash'] and after[1]['friendship']==160
            assert flash.inventory(game)==bag
            if mode==1:
                # Full moveset: ordinary machine teaching must allow Flash to
                # be selected and forgotten without a move-deleter visit.
                use(game,c,c['toxic_tm'])
                advance(game,lambda:task_active(game,'Task_HandleReplaceMoveInput'));game.frames(50);game.press(A,80)
                finish(game)
                assert details(game,c)[0]['moves'][0]==c['toxic'],details(game,c)
                assert count(game,c['tm'])==1
                game.screenshot('forgot-flash')
            close(game);saved=details(game,c);saved_bag=flash.inventory(game)
            save_game(game);game.command('reset');rock.boot(game)
            assert details(game,c)==saved and flash.inventory(game)==saved_bag
            return dict(mode=mode,learned=True,reused=True,saved=True,icon_verified=True,forgotten=mode==1)
        except BaseException:
            game.screenshot('failure');game.snapshot('failure')
            import shutil
            shutil.copy2(elf,out/'failure.elf');shutil.copy2(tmp/'failure.state',out/'failure.state')
            raise
        finally:game.close()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--driver',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--mode',type=int);a=p.parse_args()
    c=constants();results=[]
    for mode in ([a.mode] if a.mode is not None else range(6)):
        results.append(run(a.driver.resolve(),a.output.resolve()/str(mode),mode,c));print(f'PASS: TM70 fixture {mode}: {results[-1]}',flush=True)
    (a.output/'report.json').write_text(json.dumps(dict(production_sha256=hashlib.sha256((ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),fixture_only=True,results=results),indent=2)+'\n')


if __name__=='__main__':main()
