#!/usr/bin/env python3
"""Rebuild the shipping GBA trainer portrait bank from the pinned Git tree."""
import argparse
import base64
import gzip
import json
import re
import struct
import subprocess
import tempfile
from pathlib import Path

from build_battle_suite import ROOT, ARCHIVE, authenticated_sources, git_hash, tree_entries, source
from convert_player_walking import _read_png_2bit_grayscale

PACK = ROOT/'tools/polished_port/fixtures/battle_logic/trainer_pictures.json.gz.b64'
BANK = ROOT/'graphics/trainers/front_pics/polished_roster.bin'
IDS = ROOT/'include/constants/polished_trainer_pics.h'
TABLE = ROOT/'src/data/graphics/polished_trainer_pics.h'


def class_pictures(source_files, read):
    classes = re.findall(r'^\s*trainerclass (\w+)', source_files['constants/trainer_constants.asm']['text'].split('DEF NUM_TRAINER_CLASSES')[0], re.M)[1:]
    pointers = re.findall(r'^\s*dba (\w+)', source_files['data/trainers/pic_pointers.asm']['text'], re.M)
    palette_table = source_files['data/trainers/palettes.asm']['text'].split('\nelse')[0]
    palettes = re.findall(r'^INCLUDE "([^"]+\.pal)"', palette_table, re.M)
    definitions = read('gfx/trainers.asm').decode()
    labels = dict(re.findall(r'^(\w+):\s+INCBIN "(gfx/trainers/[^"]+)\.2bpp\.lz"', definitions, re.M))
    for alias,target in re.findall(r'^(\w+):\n(\w+):\s+INCBIN',definitions,re.M):
        labels[alias] = labels[target]
    assert len(classes) == 147 and len(pointers) == len(palettes)
    out = [(cls, labels[p]+'.png', pal) for cls,p,pal in zip(classes,pointers,palettes)]
    extra = re.findall(r'^INCLUDE "(gfx/trainers/kimono_girl_\w+\.pal)"', source_files['data/trainers/palettes.asm']['text'].split('assert_table_length NUM_TRAINER_CLASS_PICS')[1], re.M)
    out.extend(('KIMONO_GIRL_'+Path(pal).stem.rsplit('_',1)[1].upper(), 'gfx/trainers/kimono_girl.png', pal) for pal in extra)
    assert len(out) == 154
    return out


def import_inputs(repo, files):
    def read(path):
        return subprocess.check_output(['git','-C',str(repo),'show',source.PIN+':'+path])
    inputs = {'gfx/trainers.asm'}
    for _,pic,pal in class_pictures(files, read):
        inputs.update((pic,pal))
    objects = {p:base64.b64encode(read(p)).decode() for p in sorted(inputs)}
    for path in ('gfx','gfx/trainers'):
        data = subprocess.check_output(['git','-C',str(repo),'cat-file','tree',source.PIN+':'+path])
        objects[path] = base64.b64encode(data).decode()
    raw = json.dumps(objects, sort_keys=True, separators=(',',':')).encode()
    PACK.write_bytes(base64.b64encode(gzip.compress(raw,mtime=0))+b'\n')


def inputs():
    objects = {k:base64.b64decode(v) for k,v in json.loads(gzip.decompress(base64.b64decode(PACK.read_bytes()))).items()}
    proof = json.loads(gzip.decompress(base64.b64decode(ARCHIVE.read_bytes())))
    commit = base64.b64decode(proof['commit'])
    assert git_hash('commit',commit) == source.PIN
    root = commit.splitlines()[0].decode().split()[1]
    trees = {'':base64.b64decode(proof['trees'][root]), 'gfx':objects.pop('gfx'), 'gfx/trainers':objects.pop('gfx/trainers')}
    assert git_hash('tree', trees['']) == root
    for path, buf in [*list(trees.items())[1:], *objects.items()]:
        parent, _, name = path.rpartition('/')
        expected = {n:s for _,n,s in tree_entries(trees[parent])}[name]
        assert git_hash('tree' if path in trees else 'blob',buf) == expected, path
    return objects


def pixels(buf, path):
    path.write_bytes(buf)
    w,h,rows = _read_png_2bit_grayscale(path)
    assert (w,h) == (56,56)
    def pixel(x,y):
        return 3-rows[y-8][x-4] if 4<=x<60 and y>=8 else 0
    return bytes(pixel(tx+x,ty+y) | (pixel(tx+x+1,ty+y)<<4)
                 for ty in range(0,64,8) for tx in range(0,64,8) for y in range(8) for x in range(0,8,2))


def palette(buf):
    colors = [tuple(map(int,m)) for m in re.findall(rb'RGB\s+(\d+),\s*(\d+),\s*(\d+)',buf)]
    assert len(colors)==2 and all(0<=c<=31 for row in colors for c in row)
    return struct.pack('<16H',0x7fff,*(r | g<<5 | b<<10 for r,g,b in colors),*([0]*13))


def convert(files):
    objs = inputs()
    mapping = class_pictures(files, objs.__getitem__)
    bank = bytearray()
    offsets = {}
    out = ['/* Generated source trainer pictures and palettes. */']
    ids = ['/* Generated source trainer picture IDs. */', f'#define POLISHED_PIC_COUNT {len(mapping)}']
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp)
        for cls,pic,pal in mapping:
            if pic not in offsets:
                tiles = pixels(objs[pic],path/'in.png')
                raw, compressed = path/'in.4bpp', path/'out.4bpp.lz'
                raw.write_bytes(tiles)
                subprocess.run([str(ROOT/'tools/gbagfx/gbagfx'),str(raw),str(compressed)],check=True)
                offsets[pic] = len(bank)
                bank.extend(compressed.read_bytes())
                bank.extend(bytes((-len(bank)) % 4))
            if pal not in offsets:
                offsets[pal] = len(bank)
                bank.extend(palette(objs[pal]))
            ids.append(f'#define TRAINER_PIC_PC_{cls} (TRAINER_PIC_POLISHED_GIOVANNI + {len(ids)-1})')
            out.append(f'    [TRAINER_PIC_PC_{cls}] = {{.frontPic = TRAINER_FRONT_PIC(sPolishedTrainerArt + {offsets[pic]//4}, '
                       f'(const u16 *)(sPolishedTrainerArt + {offsets[pal]//4}))}},')
    return {BANK:bytes(bank),TABLE:('\n'.join(out)+'\n').encode(),IDS:('\n'.join(ids)+'\n').encode()}


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source-repo',type=Path)
    p.add_argument('--check',action='store_true')
    args = p.parse_args()
    files = authenticated_sources()
    if args.source_repo:
        import_inputs(args.source_repo,files)
    for path,data in convert(files).items():
        if args.check:
            assert path.read_bytes()==data, f'Regenerate {path}'
        else:
            path.write_bytes(data)
    print('PASS: all 147 source trainer class portraits and seven Kimono Girl palette overrides')
