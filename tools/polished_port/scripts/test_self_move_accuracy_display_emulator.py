#!/usr/bin/env python3
"""Check actual summary accuracy glyphs with controller input in both profiles."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

from PIL import Image
from test_johto_demo_emulator import Game, A, RIGHT, DOWN, ROOT
from test_new_bark_intro_emulator import task_active

COMPILER = 'arm-none-eabi-gcc'


def fixture(directory, profile):
    johto = profile == 'johto'
    name = 'pokeemerald-johto-demo' if johto else 'pokeemerald'
    build = ROOT/'build'/('emerald-johto-demo' if johto else 'emerald')
    obj, elf, linker = directory/'fixture.o', directory/'fixture.elf', directory/'fixture.ld'
    subprocess.run([COMPILER, '-c', '-Wa,-meabi=5', '-mthumb', '-mthumb-interwork',
                    '-O2', '-mabi=apcs-gnu', '-march=armv4t', '-std=gnu17',
                    '-DMODERN=1', '-DTESTING=0', '-DEMERALD', f'-DPOLISHED_JOHTO_DEMO={int(johto)}',
                    '-Wall', '-Werror', '-iquote', 'include', '-iquote', '.',
                    str(ROOT/'tools/polished_port/tests/self_move_accuracy_display_fixture.c'), '-o', str(obj)], cwd=ROOT, check=True)
    linker.write_text((ROOT/'ld_script_modern.ld').read_text().replace(
        'gInitialMainCB2 = CB2_InitCopyrightScreenAfterBootup;',
        'gInitialMainCB2 = CB2_PolishedSelfMoveAccuracyDisplayFixture;'))
    objects = [line[5:] for line in (ROOT/(name+'.map')).read_text().splitlines()
               if line.startswith('LOAD ') and line != 'LOAD linker stubs']
    subprocess.run(['arm-none-eabi-ld', '--gc-sections', '-T', str(linker), '-o', str(elf), str(obj), *objects], cwd=build, check=True)
    subprocess.run([str(ROOT/'tools/gbafix/gbafix'), str(elf), '-tPOKEMON EMER', '-cBPEE', '-m01', '-r0', '-d0', '--silent'], check=True)
    return elf, ROOT/(name+'.gba')


def run(driver, output, profile):
    output.mkdir(parents=True, exist_ok=True)
    source = (ROOT/'src/pokemon_summary_screen.c').read_text()
    window = source.split('[PSS_LABEL_WINDOW_MOVES_POWER_ACC] = {', 1)[1].split('}', 1)[0]
    left = int(re.search(r'\.tilemapLeft = (\d+)', window)[1]) * 8
    top = int(re.search(r'\.tilemapTop = (\d+)', window)[1]) * 8
    assert 'text, 53, 17, 0, 0' in source, 'Review the accuracy-label crop after layout changes'
    rect = (left + 53, top + 17, left + 72, top + 33)
    images = []
    with tempfile.TemporaryDirectory(prefix='self-move-accuracy-display-') as tmp:
        directory = Path(tmp)
        elf, rom = fixture(directory, profile)
        game = Game(driver, elf, elf, output, directory)
        try:
            game.frames(300)
            game.press(RIGHT, 90)
            game.press(RIGHT, 90)
            game.press(A, 90)
            assert task_active(game, 'Task_HandleInput_MoveSelect'), 'Summary move selection did not open'
            for i, name in enumerate(('dragon-dance', 'sleep-talk', 'swift-reference-dashes', 'water-gun-reference-100')):
                if i:
                    game.press(DOWN, 60)
                game.screenshot(name)
                images.append(Image.open(output/(name+'.png')).convert('RGB').crop(rect).tobytes())
            assert images[2] != images[3], 'Dash and numeric accuracy references are not distinct'
            reference = images[3] if profile == 'johto' else images[2]
            assert images[0] == reference, 'Dragon Dance accuracy differs from the expected profile reference'
            assert images[1] == reference, 'Sleep Talk accuracy differs from the expected profile reference'
        except Exception:
            game.screenshot('failure')
            raise
        finally:
            game.close()
    return dict(profile=profile, production_rom_sha256=hashlib.sha256(rom.read_bytes()).hexdigest(),
                crop=rect, glyph_hashes=[hashlib.sha256(value).hexdigest() for value in images],
                result='source 100 accuracy matches Water Gun' if profile == 'johto' else 'native accuracy dashes match Swift')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--profile', choices=('johto', 'native'))
    args = parser.parse_args()
    results = []
    for profile in ([args.profile] if args.profile else ['johto', 'native']):
        result = run(args.driver.resolve(), args.output.resolve()/profile, profile)
        results.append(result)
        print(f'PASS: {profile}: {result["result"]}', flush=True)
    (args.output/'report.json').write_text(json.dumps(dict(fixture_only=True, results=results), indent=2)+'\n')
