#!/usr/bin/env python3
"""Convert Polished Crystal's picture pointers, palettes, PNGs and frame scripts."""
import argparse
import base64
from functools import lru_cache
import gzip
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import subprocess
import tarfile

from PIL import Image, ImageDraw
import build_pokemon_learnsets as source

ROOT = source.ROOT
PACK = ROOT / 'tools/polished_port/fixtures/battle_logic/pokemon_pictures.json.gz'
ART = ROOT / 'graphics/pokemon/polished_battle'
TABLE = ROOT / 'src/data/graphics/polished_mon_pictures.h'
# Digest of the canonical path/blob inventory exported from source.PIN.
SOURCE_DIGEST = '692efeee8d803589a6f9bc29edcd19309cac5de5c1b2a569e19944f79282781f'
PIC_TABLE = 'data/pokemon/pic_pointers.asm'
PAL_TABLE = 'data/pokemon/palettes.asm'
ANIM = 'gfx/pokemon/'


def read_inputs(repo=None):
    if repo:
        raw = subprocess.check_output(['git', '-C', str(repo), 'archive', source.PIN,
                                       PIC_TABLE, PAL_TABLE, 'gfx/pokemon.asm', 'gfx/pokemon'])
        with tarfile.open(fileobj=io.BytesIO(raw)) as tree:
            files = {m.name: tree.extractfile(m).read() for m in tree if m.isfile()
                     and (m.name in (PIC_TABLE, PAL_TABLE, 'gfx/pokemon.asm') or
                          Path(m.name).name in ('front.png', 'back.png', 'normal.pal', 'shiny.pal',
                                                'anim.asm', 'anim_idle.asm', 'anims.asm', 'extras.asm',
                                                'anim_pointers.asm', 'extra_pointers.asm'))}
        raw = json.dumps({p: base64.b64encode(v).decode() for p, v in files.items()},
                         sort_keys=True, separators=(',', ':')).encode()
    else:
        raw = gzip.decompress(PACK.read_bytes())
    digest = hashlib.sha256(raw).hexdigest()
    if digest != SOURCE_DIGEST:
        raise ValueError(f'Pinned source picture inventory digest differs: {digest}')
    if repo:
        PACK.write_bytes(gzip.compress(raw, mtime=0))
    return {p: base64.b64decode(v) for p, v in json.loads(raw).items()}


def definitions(text):
    labels, pending = {}, []
    for line in text.splitlines():
        m = re.match(r'^(\w+):\s*(.*)', line)
        if not m:
            continue
        pending.append(m[1])
        if m[2]:
            path = re.fullmatch(r'(?:INCBIN|INCLUDE) "([^"]+)"', m[2])
            if not path:
                raise ValueError(line)
            labels.update(dict.fromkeys(pending, path[1]))
            pending.clear()
    assert not pending
    return labels


def mapping(files):
    def asm(path):
        return files[path].decode()
    pictures = definitions(asm('gfx/pokemon.asm'))
    main = definitions(asm(ANIM + 'anims.asm'))
    idle = definitions(asm(ANIM + 'extras.asm'))
    ptr = [m[1] or 'Egg' for m in re.finditer(r'^\s*(?:pics (\w+)|dbas EggPic, EggPic, EggFrames)', asm(PIC_TABLE), re.M)]
    ptr_main = re.findall(r'^\s*dw (\w+)', asm(ANIM + 'anim_pointers.asm'), re.M)
    ptr_idle = re.findall(r'^\s*dw (\w+)', asm(ANIM + 'extra_pointers.asm'), re.M)
    pals = re.findall(r'^INCLUDE "([^"]+\.pal)"', asm(PAL_TABLE).split('\nelse\n')[1], re.M)
    assert len(ptr) == len(ptr_main) == len(ptr_idle) and len(pals) % 2 == 0
    records = json.loads(source.MANIFEST.read_text())['records']
    cosmetics = [r for r in records if r.get('cosmetic_alias')]
    base_count = len(re.findall(r'^\s*(?:pics |dbas EggPic)', asm(PIC_TABLE).split('assert_table_length')[0], re.M))
    pal_indices = {r['source_species']:r['source_table_index'] for r in records if r['source_form']=='PLAIN_FORM'}
    rows = []
    for r in [*records, dict(target_species='SPECIES_EGG',source_table_index=ptr.index('Egg')+1)]:
        index = r['source_table_index']
        if index is None:
            idx = base_count + cosmetics.index(r)
            pal = pal_indices[r['source_species']] - 1
        else:
            idx = index - 1 + (len(cosmetics) if index > base_count else 0)
            pal = index - 1
        name = ptr[idx]
        front = pictures['EggPic' if name=='Egg' else name+'Frontpic'].replace('.animated.2bpp.lz','.png')
        back = front if name=='Egg' else pictures[name+'Backpic'].replace('.2bpp.lz','.png')
        rows.append((r['target_species'], front, back, *pals[2*pal:2*pal+2], main[ptr_main[idx]], idle[ptr_idle[idx]]))
    assert len(set(r[0] for r in rows)) == 392 and all(p in files for r in rows for p in r[1:])
    return rows


def source_frames(data, front):
    image = Image.open(io.BytesIO(data)).convert('L')
    width, height = image.size
    assert width in (40, 48, 56) and height % width == 0
    # PadFrontpic places 5x5 and 6x6 pictures one tile from the left of
    # the 7x7 source tilemap. Embed that tilemap in the GBA's 64px OBJ.
    left = 4 + (8 if width < 56 else 0) if front else 8
    for top in range(0, height, width):
        canvas = Image.new('L', (64,64), 255)
        canvas.paste(image.crop((0,top,width,top+width)), (left,64-width))
        # GBC color zero is white. Keep enclosed white pixels opaque over
        # the GBA battle background and key the connected exterior to OBJ 0.
        ImageDraw.floodfill(canvas, (0,0), 254)
        raw = canvas.tobytes()
        assert not set(raw) - {0,85,170,254,255}
        yield raw.translate(bytes([3 if i==0 else 2 if i==85 else 1 if i==170 else 4 if i==255 else 0 for i in range(256)]))


def tile_bytes(frame):
    return [bytes(frame[(y+dy)*64+x+dx] | frame[(y+dy)*64+x+dx+1]<<4
                  for dy in range(8) for dx in range(0,8,2))
            for y in range(0,64,8) for x in range(0,64,8)]


def colors(data):
    nums = [int(n) for line in data.decode().splitlines() if line.strip().startswith('RGB')
            for n in re.findall(r'\d+', line.split(';')[0])]
    assert len(nums)==6 and max(nums)<32
    return struct.pack('<16H', 0x7fff, *(r|g<<5|b<<10 for r,g,b in (nums[:3],nums[3:])),
                       0, 0x7fff, *([0]*11))


def sequence(data, count, slow):
    """Execute the source repeat bytecode and emit display durations."""
    commands = [line.split(';')[0].strip().split(maxsplit=1) for line in data.decode().splitlines()
                if line.split(';')[0].strip()]
    pc = repeat = 0
    steps = []
    for _ in range(4096):
        op, *arg = commands[pc]
        pc += 1
        if op == 'endanim':
            return steps
        if op == 'setrepeat':
            repeat = int(arg[0])
        elif op == 'dorepeat':
            repeat = max(0, repeat-1)
            if repeat:
                pc = int(arg[0])
            else:
                # DoRepeat returns to the scene loop for one VBlank on expiry.
                frame, duration = steps[-1]
                steps[-1] = (frame, duration+1)
        else:
            assert op == 'frame'
            frame, duration = map(int, arg[0].split(','))
            assert 0 <= frame < count and duration > 0
            steps.append((frame, duration + (duration//4 if slow else 0)))
    raise ValueError('Unterminated source animation')


def convert(files):
    tiles, maps, scripts, palettes = {}, [], bytearray(), bytearray()
    @lru_cache(None)
    def picture(path, front):
        start = len(maps)
        for frame in source_frames(files[path], front):
            maps.append(struct.pack('<64H', *(tiles.setdefault(tile,len(tiles)) for tile in tile_bytes(frame))))
        return start, len(maps)-start
    @lru_cache(None)
    def palette(path):
        offset = len(palettes)//32
        palettes.extend(colors(files[path]))
        return offset
    @lru_cache(None)
    def script(steps):
        start = len(scripts)//2
        for frame, duration in steps:
            scripts.extend((frame, duration))
        scripts.extend((0,0))
        return start
    table = ['/* Generated from Polished Crystal v3.2.3 by build_mon_pictures.py. */']
    for mon,front,back,normal,shiny,anim,idle in mapping(files):
        first, count = picture(front, True)
        rear = first if mon=='SPECIES_EGG' else picture(back,False)[0]
        width = Image.open(io.BytesIO(files[front])).width
        # Scene callbacks run once per tick. SetWait falls through to the
        # first of its 18 wait ticks. Play's end and Extra each take one more.
        battle = [(0, 2)] + sequence(files[anim], count, True) + [(0, 1)]
        if mon == 'SPECIES_EGG':
            # Eggs never enter battle. Reuse their two script offsets for
            # Egg1 (near hatch) and Egg2 (six to ten cycles remaining).
            battle = [(0, 1)] + sequence(files[anim], count, False) + [(0, 1)]
            menu = [(0, 1)] + sequence(files[idle], count, False) + [(0, 1)]
        else:
            menu = ([(0, 2)] + sequence(files[anim], count, False) + [(0, 20)]
                    + sequence(files[idle], count, False) + [(0, 1)])
        table.append(f'    [{mon}] = {{{first}, {rear}, {palette(normal)}, {palette(shiny)}, '
                     f'{script(tuple(battle))}, {script(tuple(menu))}, {count}, {width}}},')
    return {ART/'tiles.bin':b''.join(tiles), ART/'maps.bin':b''.join(maps),
            ART/'scripts.bin':bytes(scripts), ART/'palettes.bin':bytes(palettes),
            TABLE:('\n'.join(table)+'\n').encode()}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-repo',type=Path)
    parser.add_argument('--check',action='store_true')
    args = parser.parse_args()
    outputs = convert(read_inputs(args.source_repo))
    for path, value in outputs.items():
        if args.check:
            assert path.read_bytes() == value, f'Regenerate {path}'
        else:
            path.parent.mkdir(parents=True,exist_ok=True)
            path.write_bytes(value)
    print('PASS: 391 source species/forms and Egg, front/back artwork, both palettes and frame scripts')
