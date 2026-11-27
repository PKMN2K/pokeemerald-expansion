#!/usr/bin/env python3
"""Verify Factory BP awards, native victory/lobby scripts and Flash checkpoints."""
import argparse
import hashlib
import json
import re
import struct
import tempfile
from pathlib import Path

import build_tower_parties as source
from audit_moves_machines import compile_probe
from test_johto_demo_emulator import A, B, DOWN, LEFT, RIGHT, START, UP, Game, png_from_ppm
from test_shift_switch_emulator import ENGLISH


def source_awards():
    asm = source.trainers.archive.authenticated_sources()['engine/events/battle_tower/load_trainer.asm']['text']
    rows = re.findall(r'^; Challenge run (\d+): ([\d, ]+), (\d+) total', asm.split('; FACTORY\n', 1)[1], re.M)
    result = []
    for group, entries, total in rows:
        points = list(map(int, entries.split(', ')))
        assert int(group) == len(result) + 1 and len(points) == 7 and sum(points) == int(total)
        result.append(points)
    assert len(result) == 8
    return result


def state(game):
    values = struct.unpack('<15I', game.read(game.sym['gSettlement'], 60))
    assert not values[1], ('Factory fixture error, actual and expected', *values[1:4])
    return values


def phase(game, expected, stages=None):
    for _ in range(600):
        game.frames(20)
        values = state(game)
        if values[0] == expected and (stages is None or values[6] == stages):
            return values
    raise AssertionError(('Factory phase timed out', expected, values))


def reboot(game):
    game.command('reset')
    return phase(game, 3)


def victory(game, points, name):
    for _ in range(100):
        game.frames(10)
        raw = game.read(game.sym['gStringVar4'], 90).split(b'\xff', 1)[0]
        shown = ''.join(ENGLISH.get(byte, '?') for byte in raw)
        if shown == f'Red earned {points} BP!':
            break
    else:
        raise AssertionError((name, shown))
    # Flash writes can occupy hundreds of frames. Allow the text printer time
    # to draw the entire message after the script actually expands it.
    game.frames(120)
    frame = game.output / (name + '.ppm')
    game.command(f'shot {frame}')
    pixels = frame.read_bytes().split(b'\n', 3)[3]
    line = b''.join(pixels[(y * 240 + 12) * 3:(y * 240 + 120) * 3] for y in range(121, 139))
    assert len(set(zip(line[::3], line[1::3], line[2::3]))) >= 3, (name, 'missing dialogue pixels')
    png_from_ppm(frame)


def replay(game, awards, name, start, opening, stages, end):
    initial = phase(game, 1)
    assert initial[4] == 64
    game.frames(20)
    game.frames(12, start)
    phase(game, 2, stages)
    def award(i):
        return awards[min((opening + i) // 7, 7)][(opening + i) % 7]
    victory(game, award(stages - 1), name)
    if start == LEFT:
        game.press(START)
        assert phase(game, 5)[5:7] == (2, stages)
        assert reboot(game)[5:7] == (2, stages)
        game.press(START)
        stages += 1
        phase(game, 2, stages)
        victory(game, award(stages - 1), name + '-resumed')
    won = end == RIGHT
    wallet, card = (49998, 65530) if start == UP else (10, 20)
    wins = (opening + stages) & 0xffff
    saved = reboot(game)
    assert saved[5:13] == (3 if won else 1, stages, wallet, card, wins, opening, 1, 1), (name, saved)
    game.press(end)
    for _ in range(160):
        current = state(game)
        if current[5] == 0 and (end == B or current[13]):
            break
        game.press(A, 32)
    else:
        raise AssertionError(('Factory lobby did not save settlement', name, current))
    points = sum(award(i) for i in range(stages))
    record = wins if wins >= opening else opening
    expected = (min(50000, wallet + points), min(65535, card + points), wins if won else 0,
                record, int(won), int(won))
    assert current[7:13] == expected, (name, current, expected)
    for _ in range(2):
        current = reboot(game)
        assert current[5] == 0 and current[7:13] == expected, (name, current, expected)
        assert current[14] == int(record != opening), (name, current)
    return dict(completed=stages, earned=points, wallet=current[7], card=current[8],
                streak=current[9], record=current[10], swaps=current[11], verified_reloads=2)


def run(driver, output):
    awards = source_awards()
    output.mkdir(parents=True, exist_ok=True)
    code = (source.ROOT/'tools/polished_port/tests/factory_points_fixture.c').read_text()
    code = code.replace('/* SOURCE AWARDS */', '\n'.join('    {' + ', '.join(map(str, row)) + '},' for row in awards))
    cases = {}
    with tempfile.TemporaryDirectory() as tmp:
        directory = Path(tmp)
        elf = compile_probe(source.ROOT, directory, code)
        for name, start, opening, stages, end in (
            ('loss', A, 0, 4, A), ('factory-head', B, 14, 7, RIGHT),
            ('interruption', RIGHT, 255, 2, B), ('pause-resume', LEFT, 6, 2, A),
            ('wallet-and-wrap', UP, 65534, 2, B), ('long-streak', DOWN, 9996, 7, RIGHT),
        ):
            game = Game(driver, elf, elf, output, directory)
            try:
                cases[name] = replay(game, awards, name, start, opening, stages, end)
            finally:
                game.close()
    report = dict(source_commit=source.trainers.archive.source.PIN,
                  production_rom_sha256=hashlib.sha256((source.ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
                  prefix_cases=64, complete_set_awards=list(map(sum, awards)), cases=cases)
    (output/'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: 64 Factory award prefixes and {len(cases)} battle/lobby and Flash replays')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    run(args.driver.resolve(), args.output.resolve())
