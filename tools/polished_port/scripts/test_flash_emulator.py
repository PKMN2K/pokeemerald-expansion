#!/usr/bin/env python3
"""Controller-only Flash fixtures linked against production Johto objects.

Parties/items at boot are explicit fixture state, not campaign acquisition.
After boot only ordinary controller input mutates gameplay state.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import test_rock_smash_emulator as rock
from test_johto_demo_emulator import Game, ROOT, A, B, UP, DOWN, RIGHT
from test_new_bark_emulator import save_game, map_data
from test_new_bark_intro_emulator import choice_prompt, advance, text_matches, task_active, party, layout
from test_cherrygrove_emulator import map_id
COMPILER = "arm-none-eabi-gcc"


def constants():
    c=rock.constants()
    fields={'flash_level':'offsetof(struct SaveBlock1, flashLevel)', 'flash_flag':'FLAG_SYS_USE_FLASH',
            'badge':'FLAG_BADGE01_GET', 'rock_move':'FIELD_MOVE_FLASH', 'tm':'ITEM_HM_FLASH',
            'bellsprout':'SPECIES_BELLSPROUT','mareep':'SPECIES_MAREEP'}
    code='#include "global.h"\n#include "constants/field_move.h"\nconst u32 values[]={'+','.join(fields.values())+'};'
    r=subprocess.run(['arm-none-eabi-gcc','-mcpu=arm7tdmi','-mthumb','-mabi=apcs-gnu','-DMODERN=1','-DEMERALD','-std=gnu17','-iquote','include','-iquote','.','-S','-x','c','-o','-','-'],cwd=ROOT,input=code,text=True,capture_output=True,check=True)
    c.update(zip(fields,map(int,re.findall(r'\.word\s+(\d+)',r.stdout)),strict=True))
    return c


def fixture(directory, mode):
    elf=directory/'flash.elf';obj=directory/'fixture.o';linker=directory/'fixture.ld'
    subprocess.run([COMPILER,'-c','-Wa,-meabi=5','-mthumb','-mthumb-interwork','-O2','-mabi=apcs-gnu','-march=armv4t','-std=gnu17','-DMODERN=1','-DTESTING=0','-DEMERALD','-DPOLISHED_JOHTO_DEMO=1',f'-DFIXTURE_MODE={mode}','-Wall','-Werror','-iquote','include','-iquote','.',str(ROOT/'tools/polished_port/tests/flash_fixture.c'),'-o',str(obj)],cwd=ROOT,check=True)
    linker.write_text((ROOT/'ld_script_modern.ld').read_text().replace('gInitialMainCB2 = CB2_InitCopyrightScreenAfterBootup;','gInitialMainCB2 = CB2_PolishedFlashFixture;'))
    objects=[line[5:] for line in (ROOT/'pokeemerald-johto-demo.map').read_text().splitlines() if line.startswith('LOAD ') and line!='LOAD linker stubs']
    subprocess.run(['arm-none-eabi-ld','--gc-sections','-T',str(linker),'-o',str(elf),str(obj),*objects],cwd=ROOT/'build/emerald-johto-demo',check=True)
    subprocess.run([str(ROOT/'tools/gbafix/gbafix'),str(elf),'-tPOKEMON EMER','-cBPEE','-m01','-r0','-d0','--silent'],check=True)
    return elf


def inventory(game):
    # Opening the native party menu can relocate/re-encrypt save blocks.
    # Compare decoded item identities/counts, not the encrypted byte storage.
    key=game.u32(game.u32(game.sym['gSaveBlock2Ptr'])+layout()['encryption_key']) & 0xffff
    result={}
    for pocket in range(5):
        pointer,capacity=struct.unpack('<IH',game.read(game.sym['gBagPockets']+pocket*8,6))
        for item,quantity in struct.iter_unpack('<HH',game.read(pointer,(capacity&0x3ff)*4)):
            if item: result[pocket,item]=result.get((pocket,item),0)+(quantity^key)
    return result


def flag(game,c,value):
    return bool(game.read(game.u32(game.sym['gSaveBlock1Ptr'])+c['flags']+value//8,1)[0] & (1<<(value%8)))


def level(game,c):
    return game.read(game.u32(game.sym['gSaveBlock1Ptr'])+c['flash_level'],1)[0]


def illuminated(game,c):
    assert flag(game,c,c['flash_flag']) and level(game,c)==0
    assert not any(flag(game,c,c['badge']+i) for i in range(8)), 'Flash unexpectedly awarded a badge'
    assert struct.unpack('<H',game.read(game.sym['sGpuRegBuffer']+0x40,2))[0]==0xff, 'Lighting is still restricted to a circle'


def resolve(game,c):
    picture=False;message=False
    for _ in range(300):
        if rock.idle(game):
            illuminated(game,c)
            assert picture and message, (picture,message)
            return
        assert not task_active(game,'Task_HandleYesNoInput'), 'Menu/accepted Flash asked again'
        picture |= task_active(game,'Task_PokemonPicWindow')
        message |= text_matches(game,'PolishedFlash_UsedText')
        game.press(A,10)
    raise AssertionError('Flash did not release field controls')


def run(driver,out,mode,c):
    out.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory() as directory:
        tmp=Path(directory);elf=fixture(tmp,mode);game=Game(driver,elf,elf,out,tmp)
        try:
            rock.boot(game)
            if mode==6:
                assert level(game,c)==1 and flag(game,c,c['flash_flag'])
                save_game(game);game.command('reset');rock.boot(game);illuminated(game,c)
                game.screenshot('legacy-continued')
                return dict(mode=mode,legacy_save_normalized=True)
            assert level(game,c)>0 and not flag(game,c,c['flash_flag'])
            before=party(game,details=True);bag=inventory(game)
            if mode==5:
                game.face(UP);game.press(A,20)
                choice_prompt(game,'PolishedRockSmash_AskText')
                game.press(B,40)
                assert rock.idle(game) and level(game,c)>0
            game.face(DOWN if mode==5 else UP);game.press(A,20)
            if mode in (2,3):
                assert rock.idle(game) and level(game,c)>0
                assert inventory(game)==bag
                game.screenshot('ineligible')
                return dict(mode=mode,ineligible=True)
            choice_prompt(game,'PolishedFlash_AskText')
            assert int.from_bytes(game.read(game.sym['gSpecialVar_0x8004'],2),'little')==c['mareep' if mode==0 else 'bellsprout']
            game.screenshot('ask');game.press(B,40)
            assert rock.idle(game) and level(game,c)>0 and not flag(game,c,c['flash_flag'])
            assert party(game,details=True)==before, (before,party(game,details=True))
            assert inventory(game)==bag, "Flash changed inventory"
            if mode==4:
                rock.party_menu(game,c,True)
            else:
                if mode==1: rock.party_menu(game,c,False)
                game.press(A,20);choice_prompt(game,'PolishedFlash_AskText');game.press(A,20)
            resolve(game,c)
            assert party(game,details=True)==before, (before,party(game,details=True))
            assert inventory(game)==bag, "Flash changed inventory"
            game.screenshot('lit')
            game.press(A,30);assert rock.idle(game), 'Already lit cave offered Flash again'
            save_game(game);game.command('reset');rock.boot(game);illuminated(game,c)
            game.screenshot('continued')
            result=dict(mode=mode,saved=True,full_brightness=True)
            cave=map_data('PolishedDarkCaveVioletEntrance');route=map_data('PolishedRoute31')
            if mode==5:
                # Separate rock fixture: the source terrain separates this area
                # from the exit. Do not invent a walkable campaign route.
                game.face(UP);game.press(A,20)
                choice_prompt(game,'PolishedRockSmash_AskText');game.snapshot('before-rock')
                for delay in range(50):
                    if delay: game.restore('before-rock');game.frames(delay)
                    game.press(A,20);outcome=rock.resolve(game,c)
                    if outcome['encounter']: break
                else: raise AssertionError('No ordinary battle branch reached')
                illuminated(game,c);game.screenshot('battle-return');result['battle_return']=True
            if mode==0:
                rock.walk(game,cave,(3,14));game.frames(20,DOWN);game.frames(200)
                assert game.map_number()==map_id(game,route) and not flag(game,c,c['flash_flag'])
                warp=next(w for w in route['warp_events'] if w['dest_map']==cave['id'])
                rock.walk(game,route,(warp['x'],warp['y']+1));game.frames(20,UP);game.frames(200)
                assert game.map_number()==map_id(game,cave) and level(game,c)>0
                game.screenshot('reentry-dark');result['reentry_reset']=True
            return result
        except BaseException:
            game.screenshot('failure');game.snapshot('failure')
            import shutil
            shutil.copy2(elf,out/'failure.elf');shutil.copy2(tmp/'failure.state',out/'failure.state')
            raise
        finally: game.close()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--driver',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--quick',action='store_true');a=p.parse_args()
    c=constants();results=[]
    for mode in ([1] if a.quick else range(7)):
        results.append(run(a.driver.resolve(),a.output.resolve()/str(mode),mode,c))
        print(f'PASS: Flash fixture {mode}: {results[-1]}',flush=True)
    (a.output/'report.json').write_text(json.dumps(dict(production_sha256=hashlib.sha256((ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),fixture_only=True,results=results),indent=2)+'\n')


if __name__=='__main__': main()
