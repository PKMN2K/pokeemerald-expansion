#!/usr/bin/env python3
"""Compare the live Route 30 geography, encounters and art with the source pin."""
from collections import Counter
from pathlib import Path
import json
import re
import struct
import tempfile

from build_new_bark import ROOT, source_files
from polished_route30 import MAPS, GRAPHICS
from convert_player_walking import _read_png_2bit_grayscale


def main():
    files=source_files()
    route=json.loads((ROOT/'data/maps/PolishedRoute30/map.json').read_text())
    city=json.loads((ROOT/'data/maps/PolishedCherrygroveCity/map.json').read_text())
    attrs=files['data/maps/attributes.asm'].decode()
    assert 'connection north, Route30, ROUTE_30, 5' in attrs
    assert 'connection south, CherrygroveCity, CHERRYGROVE_CITY, -5' in attrs
    assert route['connections']==[{'map':city['id'],'offset':-10,'direction':'down'}]
    assert {'map':route['id'],'offset':10,'direction':'up'} in city['connections']
    for name,const in MAPS.items():
        live=json.loads((ROOT/f'data/maps/Polished{name}/map.json').read_text())
        src=files[f'maps/{name}.asm'].decode()
        expected=[(int(x),int(y),dest,int(n)-1) for x,y,dest,n in re.findall(r'warp_event\s+(\d+),\s*(\d+),\s*(\w+),\s*(\d+)',src)]
        actual=[(w['x'],w['y'],w['dest_map'].removeprefix('MAP_POLISHED_'),int(w['dest_warp_id'])) for w in live['warp_events']]
        assert actual==expected,name
    assert len(route['object_events'])==11
    for obj in route['object_events']:
        x,y=obj['x'],obj['y']
        assert re.search(rf'(?:object|pokemon|cuttree|fruittree|itemball)_event\s*{x},\s*{y},',files['maps/Route30.asm'].decode()),obj
    for sprite,color in (('Youngster','BLUE'),('BugCatcher','BROWN'),('Oak','BROWN')):
        assert re.search(sprite+r'SpriteGFX, WALKING_SPRITE, PAL_OW_'+color,files['data/sprites/sprites.asm'].decode())
    wild=json.loads((ROOT/'src/data/wild_encounters.json').read_text())['wild_encounter_groups'][0]
    entries=[row for row in wild['encounters'] if row['map']==route['id']]
    assert len(entries)==3
    grass=re.search(r'def_grass_wildmons ROUTE_30\n(.*?)end_grass_wildmons',files['data/wild/johto_grass.asm'].decode(),re.S)[1]
    source=re.findall(r'wildmon (\d+), (\w+)',grass)
    for n,row in enumerate(entries):
        actual,expected=Counter(),Counter()
        for mon,weight in zip(row['land_mons']['mons'],(20,20,10,10,10,10,5,5,4,4,1,1)):
            assert mon['min_level']==mon['max_level'];actual[mon['species'],mon['min_level']]+=weight
        for (lv,sp),weight in zip(source[n*7:n*7+7],(30,30,20,10,5,3,2)):
            expected['SPECIES_'+sp,int(lv)]+=weight
        assert actual==expected and row['land_mons']['encounter_rate']==18
        water=Counter()
        for mon,weight in zip(row['water_mons']['mons'],(60,30,5,4,1)):
            water[mon['species'],mon['min_level']]+=weight
        assert water=={('SPECIES_POLIWAG',20):60,('SPECIES_POLIWAG',15):30,('SPECIES_POLIWHIRL',20):10}
        assert row['water_mons']['encounter_rate']==4 and row['always_time_of_day']
    common=(ROOT/'include/constants/flags.h').read_text();frlg=(ROOT/'include/constants/flags_frlg.h').read_text()
    for suffix in ('CUT','HIDDEN_POTION','ANTIDOTE','MULCH'):
        name='FLAG_POLISHED_ROUTE30_'+suffix;value=int(re.search('#define '+name+r'\s+(0x\w+)',common)[1],16)
        assert 0x4ac<=value<=0x4af
        assert all(int(v,16)!=value or k==name for k,v in re.findall(r'#define (\w+)\s+(0x[\da-fA-F]+)',common+frlg))
    # Compare the actual converted item pixels and both two-color source banks.
    from PIL import Image
    with tempfile.TemporaryDirectory() as tmp:
        for item,bank,label in (('mulch','items','MULCH'),('mystery_egg','key_items','MYSTERY EGG')):
            p=Path(tmp)/'source.png';p.write_bytes(files[f'gfx/items/{item}.png'])
            w,h,pixels=_read_png_2bit_grayscale(p)
            live=Image.open(ROOT/f'graphics/items/icons/polished_{item}.png')
            assert live.size==(w,h)==(24,24)
            assert list(live.tobytes())==[3-v for row in pixels for v in row]
            pal=(ROOT/f'graphics/items/icon_palettes/polished_{item}.pal').read_text().splitlines()[3:7]
            actual=[tuple(int(c)>>3 for c in row.split()) for row in pal]
            body=files[f'gfx/items/{bank}.pal'].decode().split('; '+label+'\n')[1]
            colors=[tuple(map(int,v)) for v in re.findall(r'RGB\s+(\d+),\s*(\d+),\s*(\d+)',body)[:2]]
            assert actual==[(31,31,31),*colors,(0,0,0)]
    # Every scripted Oak/Player stop lies on original walkable floor.
    cells=struct.unpack('<64H',(ROOT/'data/layouts/PolishedMrPokemonsHouse/map.bin').read_bytes())
    for x,y in ((2,6),(3,6),(6,6),(5,6),(4,6),(4,7),(3,7)):
        assert cells[y*8+x]&0xfc00==0x3000,(x,y)
    mr=(ROOT/'data/maps/PolishedMrPokemonsHouse/scripts.inc').read_text()
    assert 'DEBUG' not in mr and 'playfanfare MUS_HEAL' in mr
    assert re.search(r'giveitem ITEM_POLISHED_MYSTERY_EGG\s+goto_if_eq VAR_RESULT, FALSE',mr)
    assert mr.index('giveitem ITEM_POLISHED_MYSTERY_EGG')<mr.index('setvar VAR_POLISHED_MR_POKEMON_SCENE, 1')
    for source,target in (('PlayerWalksToMrPokemon','PlayerApproach'),('OakWalksToPlayer','OakApproach')):
        src=re.search(r'MrPokemonsHouse_'+source+r':\n(.*?)step_end',files['maps/MrPokemonsHouse.asm'].decode(),re.S)[1]
        live=re.search(r'PolishedMrPokemonsHouse_'+target+r':\n(.*?)step_end',mr,re.S)[1]
        assert re.findall(r'step_(right|left|up|down)',src)==re.findall(r'walk_(right|left|up|down)',live)
    parties=(ROOT/'src/data/polished_route30.party').read_text()
    for trainer in ('MIKEY','DON'):
        source=re.search(r'def_trainer '+trainer+r',.*?\n(.*?)end_trainer',files['data/trainers/parties.asm'].decode(),re.S)[1]
        actual=re.search(r'=== TRAINER_POLISHED_'+trainer+r' ===\n(.*?)(?==== TRAINER_|\Z)',parties,re.S)[1]
        expected=[(species.title(),level) for level,species in re.findall(r'tr_mon (\d+), (\w+)',source)]
        assert re.findall(r'^(\w+) \(M\)\nLevel: (\d+)',actual,re.M)==expected
        assert actual.count('12 HP / 12 Atk / 12 Def / 12 SpA / 12 SpD / 12 Spe')==len(expected)
    print('PASS: Route 30 source connections, six reciprocal doors, first-visit objects and safe flags')
    print('PASS: all three grass distributions and source water species/level weights')
    print('PASS: authentic item pixels/colors, walkable Oak choreography and guarded quest handoff')


if __name__=='__main__':
    main()
