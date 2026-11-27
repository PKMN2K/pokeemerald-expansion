#!/usr/bin/env python3
"""Construct every source party with the shipping battle factory and verify trainer art and Flash flags."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import tempfile
import zlib

import build_trainer_portraits as art
import build_trainer_roster as roster
from audit_moves_machines import compile_probe
from test_johto_demo_emulator import Game

ROOT = roster.ROOT


def explicit_or_learned(mon, level, learnsets):
    if mon['moves']:
        return mon['moves']
    known = []
    for row in learnsets[mon['species']]:
        if 0 < row['level'] <= level and row['target_move'] not in known:
            known = (known + [row['target_move']])[-4:]
    return known


def fixture(classes, records, directory):
    files = roster.archive.authenticated_sources()
    inputs = art.inputs()
    learnsets = {r['target_species']:r['moves'] for r in
                 json.loads(roster.archive.source.MANIFEST.read_text())['records']}
    art_rows = []
    for cls,pic,pal in art.class_pictures(files,inputs.__getitem__):
        pixels = art.pixels(inputs[pic],directory/'reference.png')
        art_rows.append(f'    {{TRAINER_PIC_PC_{cls}, {zlib.crc32(pixels)}u, {zlib.crc32(art.palette(inputs[pal]))}u}},')
    mons, teams = [], []
    for r in records:
        first = len(mons)
        for m in r['mons']:
            levels = [m['level']]*2 if m['level']<=100 else [max(2, min(99, b+m['level']-178)) for b in (4,61)]
            moves = []
            for level in levels:
                four = explicit_or_learned(m,level,learnsets)
                moves.append('{'+', '.join(four+['MOVE_NONE']*(4-len(four)))+'}')
            spread = 'TRAINER_PARTY_IVS('+', '.join(map(str,m['ivs']))+')'
            evs = '{'+', '.join(map(str,m['evs']))+'}'
            mons.append(f'    {{{m["species"]}, {m["item"]}, {m["nature"]}, MON_{m["gender"]}, {m["ability"]}, '
                        f'{int(m["shiny"])}, {{{levels[0]}, {levels[1]}}}, {spread}, {evs}, {{{", ".join(moves)}}}}},')
        teams.append(f'    {{{r["key"]}, {first}, {len(r["mons"])}}},')
    encoded = (ROOT/'tools/polished_port/fixtures/battle_logic/inverse_type_matchups.asm').read_bytes()
    assert hashlib.sha1(b'blob '+str(len(encoded)).encode()+b'\x00'+encoded).hexdigest() == 'cf12f3cfd04f51b88b01c96b43b32902d3c31cdb'
    table = {(a,b):v for a,b,v in re.findall(r'^\s*db (\w+),\s*(\w+),\s*(\w+)', encoded.decode(), re.M)}
    types = sorted({t for pair in table for t in pair})
    assert len(types) == 18 and len(table) == 120
    matchups = []
    for atk in types:
        for defense in types:
            x = {'SUPER_EFFECTIVE':'2.0','NOT_VERY_EFFECTIVE':'0.5',None:'1.0'}[table.get((atk,defense))]
            matchups.append(f'    {{TYPE_{atk}, TYPE_{defense}, UQ_4_12({x})}},')
    template = (ROOT/'tools/polished_port/tests/trainer_roster_fixture.c').read_text()
    return template.replace('/* PICTURES */','\n'.join(art_rows)).replace('/* MONS */','\n'.join(mons)).replace('/* TEAMS */','\n'.join(teams)).replace('/* MATCHUPS */','\n'.join(matchups))


def run(driver, output):
    output.mkdir(parents=True,exist_ok=True)
    classes, records = roster.roster()
    with tempfile.TemporaryDirectory() as tmp:
        directory = Path(tmp)
        elf = compile_probe(ROOT,directory,fixture(classes,records,directory))
        game = Game(driver,elf,elf,output,directory)
        try:
            def result():
                return struct.unpack('<8I',game.read(game.sym['gRosterResult'],32))
            def wait(stage):
                for _ in range(2000):
                    game.frames(16)
                    state = result()
                    assert state[3] == 0, (records[state[1]]['key'] if state[1]<len(records) else state[1],state)
                    if state[0] == stage:
                        return state
                raise AssertionError(('trainer roster timeout',result()))
            checked = wait(2)
            assert checked[4:7] == (1912,154,127),checked
            game.command('reset')
            saved = wait(3)
            assert saved[7] == 934,saved
        finally:
            game.close()
    data = dict(source_commit=roster.archive.source.PIN, production_rom_sha256=hashlib.sha256((ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
                source_parties=len(records), roster_entries=sum(len(r['mons']) for r in records), party_builds=checked[4],
                portraits_and_palettes=checked[5], inver_omissions=7, inverse_matchups=324, saved_trainer_flags=saved[7])
    (output/'report.json').write_text(json.dumps(data,indent=2)+'\n')
    print('PASS: 956 trainer records at zero and sixteen badges, all seven Inver omissions, 154 portraits/palettes and 934 flags after Flash reset')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--driver',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    run(args.driver.resolve(),args.output.resolve())
