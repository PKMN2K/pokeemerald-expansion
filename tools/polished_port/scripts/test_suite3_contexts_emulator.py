#!/usr/bin/env python3
"""Check wild construction, postbattle rewards, real win, capture, escape, loss and Flash reload."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import tempfile

from audit_moves_machines import ROOT, compile_probe
from build_battle_suite import authenticated_sources
from build_pokemon_learnsets import PIN
from test_johto_demo_emulator import Game, A, B
from test_new_bark_battle_emulator import choose_cell, controller


def draws(seed):
    # The probe controls the shipping SFC32 byte stream. The expected mapping
    # and number of draws below are independently derived from the source ASM.
    mask = 0xffffffff
    c = seed
    a = b = 0
    counter = 1
    for index in range(100):
        result = (a + b + counter) & mask
        a, b = b ^ (b >> 9), (c * 9) & mask
        c = (result + ((c << 21) | (c >> 11))) & mask
        counter += 1
        if index >= 16:
            yield result >> 16


def cases():
    tables = authenticated_sources()['data/items/pickup_items.asm']['text'].split('RarePickupTable:')
    base, rare = (re.findall(r'^\s*db (\w+)\s*$', part, re.M) for part in tables)
    assert len(base) == 18 and len(rare) == 11
    seen = {}
    for seed in range(6000):
        level = seed % 100 + 1
        for honey in (False, True):
            rng = draws(seed)
            def sample(n):
                while (raw := next(rng) & 255) >= 256 - 256 % n:
                    pass
                return raw % n
            tier = (level - 1) // 10
            item = 'NONE'
            if honey:
                if level + 9 >= 11 * (1 + sample(20)):
                    item = 'HONEY'
            elif sample(10) == 0:
                roll = sample(100)
                if roll < 2:
                    item = rare[tier + int((next(rng) & 255) < 128)]
                else:
                    item = base[tier + sum(roll < threshold for threshold in (6, 10, 20, 30, 40, 50, 60, 70))]
            seen.setdefault((honey, tier, item), (seed, level, item, next(rng), int(honey)))
    return list(seen.values())


def medicine_cases():
    rows = []
    def case(seed, hp, level, index, count, status='0'):
        rows.append(f'        {{{seed}, {hp}, {level}, {index}, {count}, {status}}},')
    none = 'MAX_TRAINER_ITEMS'
    case(0, 25, 10, 2, 0)
    case(0, 1, 11, none, 0)
    case(0, 51, 10, none, 0)
    case(0, 100, 10, 0, 0, 'STATUS1_FREEZE')
    case(0, 100, 10, 0, 0, 'STATUS1_SLEEP')
    case(0, 100, 10, none, 0, 'STATUS1_BURN')
    case(0, 100, 10, none, 0, 'STATUS1_TOXIC_POISON | STATUS1_TOXIC_TURN(3)')
    seen = set()
    for seed in range(256):
        rng = draws(seed)
        a, b = next(rng) & 255, next(rng) & 255
        for hp in (26, 50):
            index, count = (2, 1) if a < 50 else (1 if b < 50 else none, 2)
            if (hp, index) not in seen:
                seen.add((hp, index))
                case(seed, hp, 10, index, count)
        index = 0 if a < 128 else none
        if ('toxic', index) not in seen:
            seen.add(('toxic', index))
            case(seed, 100, 10, index, 1, 'STATUS1_TOXIC_POISON | STATUS1_TOXIC_TURN(4)')
    assert len(seen) == 8
    return rows


def run(driver, output):
    output.mkdir(parents=True, exist_ok=True)
    fixtures = cases()
    code = (ROOT/'tools/polished_port/tests/suite3_context_fixture.c').read_text()
    rows = ['    {%d, %d, ITEM_%s, %d, %d},' % row for row in fixtures]
    code = code.replace('/* GENERATED_REWARD_CASES */', 'static const struct RewardCase sCases[] = {\n' + '\n'.join(rows) + '\n};')
    medicine = medicine_cases()
    code = code.replace('/* GENERATED_MEDICINE_CASES */', 'static const struct MedicineCase medicine[] = {\n' + '\n'.join(medicine) + '\n    };')
    with tempfile.TemporaryDirectory(prefix='polished-contexts-') as tmp:
        directory = Path(tmp)
        outcomes = {}
        for name, expected in (('win', 1), ('capture', 7), ('escape', 4), ('loss', 2)):
            if name in ('win', 'loss'):
                build = directory / ('winnable' if name == 'win' else 'blackout')
                build.mkdir()
                elf = compile_probe(ROOT, build, f"#define PROBE_LOSS {int(name == 'loss')}\n" + code)
            state = directory / name
            state.mkdir()
            game = Game(driver, elf, elf, output, state)
            try:
                def result():
                    return struct.unpack('<10I', game.read(game.sym['gContextResult'], 40))
                def wait(predicate, label, key=A):
                    for frame in range(3000):
                        if predicate(): return
                        game.frames(4, key if frame % 8 == 0 else 0)
                    raise AssertionError((label, result()))
                wait(lambda: controller(game, 'HandleInputChooseAction'), 'action')
                assert result()[:3] == (1, 0, 0) and result()[6] == 0, result()
                eligible = name in ('win', 'capture')
                game.snapshot("before-action")
                # A legitimate win can produce zero Honey. Exercise an award through
                # a real battle by varying input timing, keeping every outcome check.
                for delay in range(16):
                    if delay:
                        game.restore("before-action")
                        game.frames(7 * delay)
                    if name == 'capture':
                        game.press(256, 8)  # R throws the only ball in the bag.
                    else:
                        choose_cell(game, 'gActionSelectionCursor', 3 if name == 'escape' else 0)
                        if name != 'escape':
                            wait(lambda: controller(game, 'HandleInputChooseMove'), 'move')
                            choose_cell(game, 'gMoveSelectionCursor', 0)
                    wait(lambda: result()[0] == 2, 'battle outcome', B if name == 'capture' else A)
                    after = result()
                    assert after[3:6] == (expected, 0 if eligible else 16, 1), (name, after)
                    assert after[8:] == (int(name == 'capture'), 1), (name, after)
                    if not eligible or after[7]:
                        break
                assert (after[7] > 0) == eligible, (name, after)
                game.command('reset')
                wait(lambda: result()[0] == 3, 'Flash load')
                loaded = result()
                assert loaded[2] == 0 and loaded[4] == after[4] and loaded[7:] == after[7:], (name, after, loaded)
                outcomes[name] = dict(outcome=expected, status=after[4], honey_in_bag=after[7],
                                      magikarp_delivered=bool(after[8]), everstone_retained=bool(after[9]))
            finally:
                game.close()
    report = dict(source_commit=PIN, production_rom_sha256=hashlib.sha256((ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
                  seeded_reward_cases=len(fixtures), wild_factory_cases=96, trainer_medicine_cases=len(medicine), outcomes=outcomes, flash_reloads=len(outcomes))
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(f'PASS: {len(fixtures)} reward cases, {len(medicine)} trainer medicine cases, wild and roamer construction, real win/capture/escape/loss and four Flash reloads')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    run(args.driver.resolve(), args.output.resolve())
