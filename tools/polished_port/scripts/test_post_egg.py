#!/usr/bin/env python3
"""Compare the compiled port's inputs to the pinned graphics, trigger geometry and source allocations."""
import json
import re
import struct
import tempfile
from pathlib import Path
from PIL import Image
from build_new_bark import ROOT, source_files
from convert_player_walking import _read_png_2bit_grayscale


def main():
    files=source_files()
    with tempfile.TemporaryDirectory() as tmp:
        src=Path(tmp)/'lyra.png';src.write_bytes(files['gfx/battle/lyra_back.png'])
        width,height,rows=_read_png_2bit_grayscale(src)
    assert (width,height)==(48,48)
    image=Image.open(ROOT/'graphics/trainers/back_pics/polished_lyra.png')
    assert image.size==(64,64)
    assert list(image.tobytes())==[3-rows[y-16][x-8] if 16<=y<64 and 8<=x<56 else 0 for y in range(64) for x in range(64)]
    source=files['maps/Route29.asm'].decode()
    route=json.loads((ROOT/'data/maps/PolishedRoute29/map.json').read_text())
    assert [(c['x'],c['y']) for c in route['coord_events']]==[(53,8),(53,9)]
    cells=struct.unpack('<'+str(60*18)+'H',(ROOT/'data/layouts/PolishedRoute29/map.bin').read_bytes())
    script=(ROOT/'data/maps/PolishedRoute29/scripts.inc').read_text()
    for row in (1,2):
        lx,ly=50,12
        for part in ('a','b'):
            label=str(row)+part
            original=source.split('LyraMovementData'+label+':\n',1)[1].split('step_end')[0]
            live=script.split('PolishedDeparture_Move'+label+':\n')[1].split('step_end')[0]
            steps=re.findall(r'^\s*step_(left|up|down|right)\s*$',original,re.M)
            assert steps==re.findall(r'walk_(left|up|down|right)',live)
            for d in steps:
                dx,dy={'up':(0,-1),'down':(0,1),'left':(-1,0),'right':(1,0)}[d]
                lx+=dx;ly+=dy
                assert cells[ly*60+lx]&0xfc00==0x3000,(row,lx,ly)
            assert (lx,ly)==((52,7+row) if part=='a' else (50,12))
    flags=''.join((ROOT/('include/constants/'+p)).read_text() for p in ('flags.h','flags_frlg.h'))
    nums=re.findall(r'#define (\w+)\s+(0x[\da-fA-F]+)',flags)
    for i,name in enumerate(('MOM_BANK_OPEN','MOM_SAVES_MONEY','MOM_FIRST_VISIT','CAUGHT_TUTORIAL','DECLINED_TUTORIAL','MET_TUSCANY','TUSCANY_SILK_SCARF')):
        assert [k for k,v in nums if int(v,16)==0x4c0+i]==['FLAG_POLISHED_'+name]
    vars=''.join((ROOT/('include/constants/'+p)).read_text() for p in ('vars.h','vars_frlg.h'))
    for key,value in [('LO',0x4091),('HI',0x409b)]:
        assert [k for k,v in re.findall(r'#define (\w+)\s+(0x[\da-fA-F]+)',vars) if int(v,16)==value]==['VAR_POLISHED_MOM_BALANCE_'+key]
    print('PASS: exact Lyra back pixels, both source paths, walkable landings and independent shared save allocations')


if __name__=='__main__':
    main()
