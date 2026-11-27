#!/usr/bin/env python3
"""Compare the shipping Tower party builder to the default release's tiers and legality checks."""
import argparse
import hashlib
from itertools import accumulate
import json
from pathlib import Path
import struct
import tempfile

import build_tower_parties as source
from audit_moves_machines import compile_probe
from test_johto_demo_emulator import Game

ROOT = source.ROOT


def draws(seed):
    a, b, c, counter = 0, 0, seed, 1
    for i in range(100000):
        value = (a + b + counter) & 0xffffffff
        counter += 1
        a, b, c = b ^ (b >> 9), (c * 9) & 0xffffffff, (value + ((c << 21) | (c >> 11))) & 0xffffffff
        if i >= 16:
            yield (value >> 16) & 0xff


def select(stream, n):
    return next(v % n for v in stream if v < 256 - 256 % n)


def expected_teams(tiers, masks):
    offsets = list(accumulate(map(len, tiers), initial=0))
    sets = sum(tiers, [])
    seen = set()
    steps = []
    history = []
    repeats = 0
    for seed in range(1, 10001):
        wins = (seed - 1) % 65
        if seed % 19 == 0:
            wins = 258
        stream = draws(seed)
        rank = min(7, wins // 7 + (0, 1, 1, 2, 2, 3, 4)[wins % 7])
        mask = masks[rank][next(stream) % 2]
        slots = [(mask >> shift) & 3 for shift in (0, 2, 4)]
        for i in (2, 1):
            j = select(stream, i + 1)
            slots[i], slots[j] = slots[j], slots[i]
        while True:
            team = [offsets[t] + select(stream, len(tiers[t])) for t in slots]
            mons = [sets[i] for i in team]
            items = [m['item'] for m in mons if m['item'] != 'ITEM_NONE']
            if len({m['base'] for m in mons}) == 3 and len(set(items)) == len(items) and not set(team[:2]).intersection(history):
                break
        repeats += team[2] in history
        history = (history + team)[-9:]
        steps.append((wins, team))
        seen.update(team)
        if len(seen) == len(sets) and repeats and len(steps) >= 700:
            return sets, steps, history, repeats
    raise ValueError('deterministic replay did not visit all four source tiers')


def fixture(sets, steps, history):
    mons = []
    for m in sets:
        iv = ', '.join(map(str, m['iv']))
        moves = ', '.join(m['moves'])
        mons.append(f'    {{{m["species"]}, {m["item"]}, {{{moves}}}, TRAINER_PARTY_IVS({iv}), '
                    f'MON_{m["gender"]}, {m["nature"]}, {m["ability"]}}},')
    rows = [f'    {{{wins}, {{{", ".join(str(x) for x in team)}}}}},' for wins, team in steps]
    return (ROOT/'tools/polished_port/tests/tower_parties_fixture.c').read_text().replace(
        '/* SOURCE SETS */', '\n'.join(mons)).replace('/* EXPECTED PARTIES */', '\n'.join(rows)).replace(
        '/* FINAL HISTORY */', ', '.join(str(index + 1) for index in history))


def run(driver, output):
    output.mkdir(parents=True, exist_ok=True)
    sets, steps, history, repeats = expected_teams(*source.catalog())
    with tempfile.TemporaryDirectory() as tmp:
        directory = Path(tmp)
        elf = compile_probe(ROOT, directory, fixture(sets, steps, history))
        game = Game(driver, elf, elf, output, directory)
        try:
            for stage in (2, 3):
                for _ in range(1200):
                    game.frames(16)
                    state = struct.unpack('<6I', game.read(game.sym['gTowerAudit'], 24))
                    assert state[2] == 0, state
                    if state[0] == stage:
                        break
                else:
                    raise AssertionError(('Tower replay timeout', state))
                if stage == 2:
                    assert state[1] == len(steps), state
                    game.command('reset')
        finally:
            game.close()
    record = dict(source_commit=source.trainers.archive.source.PIN, tiers=[61, 140, 143, 88],
                  production_rom_sha256=hashlib.sha256((ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
                  sets=len(sets), generated_teams=len(steps), third_slot_history_repeats=repeats,
                  retained_history_slots=len(history), difficulty_rows=8, point_award_cases=65, flash_bp=50000)
    (output/'report.json').write_text(json.dumps(record, indent=2)+'\n')
    print(f'PASS: {len(steps)} production Tower parties, every one of 432 sets, eight difficulty rows and three retained teams after Flash reset')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    run(args.driver.resolve(), args.output.resolve())
