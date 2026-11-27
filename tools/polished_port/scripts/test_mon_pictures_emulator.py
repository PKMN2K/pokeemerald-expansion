#!/usr/bin/env python3
"""Compare the Johto ROM's picture and playback entry points with pinned source sheets."""
import argparse
from functools import lru_cache
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import zlib

import build_mon_pictures as art
from audit_moves_machines import compile_probe
from test_johto_demo_emulator import Game


def fixture(files):
    steps, cases = [], []
    @lru_cache(None)
    def pixels(path, front):
        return [zlib.crc32(b''.join(art.tile_bytes(frame)))
                for frame in art.source_frames(files[path], front)]

    for mon, front, back, normal, shiny, anim, idle in art.mapping(files):
        frames = pixels(front, True)
        rear = frames[0] if mon == 'SPECIES_EGG' else pixels(back,False)[0]
        seq = art.sequence(files[anim],len(frames),True)
        if mon == 'SPECIES_CYNDAQUIL':
            # Explicit slow-script reference, including a four-pass loop and
            # the source interpreter's one-tick return on its final dorepeat.
            assert seq == [(1,5),(4,5),(1,5)] + [(4,3),(1,3)]*3 + [(4,3),(1,4),(2,3),(3,11),(1,3)]
        seq = ([(0,1)] + art.sequence(files[anim],len(frames),False) + [(0,1)]
               if mon == 'SPECIES_EGG' else [(0,2)] + seq + [(0,1)])
        battle = len(steps)
        steps.extend((frames[f],d) for f,d in seq+[(0,0)])
        menu = len(steps)
        idle_frames = art.sequence(files[idle],len(frames),False)
        seq = ([(0,1)] + idle_frames + [(0,1)] if mon == 'SPECIES_EGG' else
               [(0,2)] + art.sequence(files[anim],len(frames),False) + [(0,20)] + idle_frames + [(0,1)])
        steps.extend((frames[f],d) for f,d in seq+[(0,0)])
        palettes = [zlib.crc32(art.colors(files[p])) for p in (normal,shiny)]
        cases.append(f'    {{{mon}, {frames[0]}u, {rear}u, {palettes[0]}u, {palettes[1]}u, {battle}, {menu}}},')
    code = (art.ROOT/'tools/polished_port/tests/mon_pictures_fixture.c').read_text()
    return code.replace('/* CASES */','\n'.join(cases)).replace(
        '/* STEPS */','\n'.join(f'    {{{crc}u, {duration}}},' for crc,duration in steps)), len(steps)


def run(driver, output):
    files = art.read_inputs()
    code, steps = fixture(files)
    output.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        work = Path(tmp)
        elf = compile_probe(art.ROOT,work,code)
        game = Game(driver,elf,elf,output,work)
        try:
            for _ in range(2400):
                game.frames(128)
                state = struct.unpack('<6I', game.read(game.sym['gPicAudit'],24))
                if state[1]:
                    raise AssertionError((state, art.mapping(files)[min(state[0],391)][0]))
                if state[5]:
                    assert state[0] == 392
                    break
            else:
                raise AssertionError(('Picture playback timed out', state))
        finally:
            game.close()
    report = dict(source_commit=art.source.PIN, identities=391, egg=True, palettes=784,
                  playback_sequences=784, playback_steps=steps, stationary_egg_from_cycle=11, expansion_control='RAYQUAZA',
                  production_rom_sha256=hashlib.sha256((art.ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest())
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS: 392 picture rows, both palettes and 784 OBJ VRAM playback sequences ({steps} steps)')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    run(args.driver.resolve(),args.output.resolve())
