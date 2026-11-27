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
from test_shift_switch_emulator import ENGLISH
from test_smart_field_ai import replay as replay_smart_field_ai
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


def gender_cases():
    # Source identification uses one-based species numbers. These mixed-gender
    # targets distinguish extension mismatch, the variant table and the egg gate.
    cases = [
        ('JIGGLYPUFF', 'FEMALE', 'CUTE_CHARM', False, 'PIDGEY', 4),
        ('JIGGLYPUFF', 'MALE', 'CUTE_CHARM', False, 'PIDGEY', 4),
        ('ABRA', 'MALE', 'SYNCHRONIZE', False, 'GROWLITHE', 2),
        ('ABRA', 'MALE', 'SYNCHRONIZE', True, 'GROWLITHE', 2),
        ('MISMAGIUS', 'FEMALE', 'NONE', False, 'GLOOM', 6),
        ('MISMAGIUS', 'FEMALE', 'NONE', False, 'EXEGGCUTE', 11),
        ('MISMAGIUS', 'FEMALE', 'NONE', False, 'KABUTO', 10),
        ('URSALUNA_BLOODMOON', 'MALE', 'NONE', False, 'URSALUNA', 0),
        ('PIDGEY', 'MALE', 'NONE', False, 'URSALUNA', 8),
    ]
    rows = []
    for lead, lead_gender, ability, egg, target, ratio in cases:
        for seed in range(1,65):
            rolls = (x & 255 for x in draws(seed))
            for _ in range(3):
                next(rolls)  # Six DVs
            if ability == 'SYNCHRONIZE' and not egg:
                nature = 'NATURE_HARDY'
            else:
                nature = str(next(x % 25 for x in rolls if x < 250))
            next(rolls)  # Ability slot
            if next(rolls) == 0:
                next(rolls)  # Shiny check
            if ability == 'CUTE_CHARM' and next(x % 3 for x in rolls if x < 255):
                gender_roll = 0  # The release compares the packed lead byte to FEMALE.
            else:
                gender_roll = next(rolls) & 7
            gender = 'MON_FEMALE' if gender_roll < ratio else 'MON_MALE'
            rows.append(f'{{SPECIES_{lead}, SPECIES_{target}, ABILITY_{ability}, MON_{lead_gender}, {int(egg)}, {seed}, {nature}, {gender}}},')
    return rows


def ai_tie_cases():
    # Highest native score corresponds to the source's lowest score. Rejected
    # positions retain their place in the four-slot draw after usable packing.
    scenarios = [
        (('TACKLE','GROWL','LEER','BITE'), 0, (20,25,20,25)),
        (('TACKLE','GROWL','LEER','BITE'), 1, (25,20,25,20)),
        (('TACKLE','NONE','LEER','NONE'), 0, (20,0,20,0)),
        (('TACKLE','GROWL','LEER','BITE'), 0, (-3,-3,-3,-3)),
        (('TACKLE','GROWL','TACKLE','NONE'), 1, (0,5,9,0)),
        (('TACKLE','GROWL','LEER','BITE'), 15, (20,20,20,20)),
    ]
    rows = []
    for moves, limitations, scores in scenarios:
        usable = [i for i,m in enumerate(moves) if m != 'NONE' and not limitations & (1<<i)]
        for seed in range(1,33):
            stream = draws(seed)
            index = 4
            if usable:
                best = max(scores[i] for i in usable)
                allowed = {slot for slot,i in enumerate(usable) if scores[i] == best}
                chosen = next(roll & 3 for roll in stream if roll & 3 in allowed)
                index = moves.index(moves[usable[chosen]])
            rows.append('{{'+', '.join('MOVE_'+m for m in moves)+'}, {'+', '.join(map(str,scores))+'}, '
                        +f'{limitations}, {seed}, {index}, {next(stream)}'+'},')
    return rows


def setup_cases():
    # Focus Energy lies within the source stat-up effect interval. Growl is
    # a stat-down, Curse uses the user's Ghost type, and Tackle draws nothing.
    rows = []
    for first in (0, 1):
        for ghost in (0, 1):
            for limit in (0, 1, 2, 4, 7):
                for seed in range(1, 33):
                    rng = draws(seed)
                    scores = [100] * 4
                    for slot in range(2 + ghost):
                        if limit & (1 << slot):
                            continue
                        roll = next(rng) & 255
                        if slot == 1 and first:
                            scores[slot] += 2 * (roll >= 128)
                        else:
                            scores[slot] -= 2 * (roll >= 30)
                    rows.append('{' + f'{first}, {ghost}, {limit}, {seed}, {next(rng)}, '
                                + '{' + ', '.join(map(str, scores)) + '}},')
    return rows


def ai_policy_cases():
    # Reflect is a stall move, Conversion and Substitute are in both move
    # lists, and Tackle is a damaging negative control.
    rows = []
    for mask in (8, 32, 128, 168):
        for hp in (25, 26, 50, 51):
            for limit in (0, 2):
                for seed in range(1,9):
                    stream = draws(seed)
                    stall = bool(mask & 32) and hp <= 50 and (hp <= 25 or next(stream)&255 >= 128)
                    cautious = bool(mask & 128)
                    scores = [100]*4
                    for i in range(3):
                        if limit & (1<<i):
                            continue
                        scores[i] -= 2 * bool(mask & 8) + stall
                        if cautious and i != 2:
                            cautious = (next(stream)&255) < 230
                            scores[i] -= cautious
                    rows.append('{'+f'{mask}, {hp}, {limit}, {seed}, {next(stream)}, '+
                                '{'+', '.join(map(str,scores))+'}},')
    return rows


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


def roam_cases():
    raw = (ROOT/'tools/polished_port/fixtures/roammon_maps.asm').read_text()
    graph = {}
    for line in raw.splitlines():
        if line.strip().startswith('roam_map ROUTE_'):
            start, *near = map(int, re.findall(r'ROUTE_(\d+)', line))
            graph[start] = near
    routes = list(graph)
    seen = {}
    encounters = {}
    for seed in range(12000):
        rng = draws(seed)
        first = next(rng)
        byte = first & 255
        encounters.setdefault(byte, (seed, byte, next(rng)))
        for start, neighbors in graph.items():
            rng = draws(seed)
            while True:
                index = next(rng) & 31
                if index == 0:
                    while (dest := routes[next(rng) & 15]) == 29:
                        pass
                    break
                if (index & 3) < len(neighbors):
                    dest = neighbors[index & 3]
                    break
            seen.setdefault((start, index == 0, dest), (seed, start, dest, next(rng)))
        if len(seen) == 15 * len(routes) + sum(map(len, graph.values())) and len(encounters) == 256:
            break
    assert len(seen) == 15 * len(routes) + sum(map(len, graph.values())) and len(encounters) == 256
    return list(seen.values()), list(encounters.values())


def payout_cases():
    # CheckPayDay saturates doubling. The three-byte wallet addition discards
    # its carry before AddBattleMoneyToAccount compares against 9,999,999.
    for wallet in (0, 1, 999998, 9999998, 9999999):
        for pot in (0, 400, 65536, 1000001, 0x7fffff, 0x800000, 0xffffff):
            for doubled in (0, 1):
                paid = min(0xffffff, pot * (1 + doubled))
                balance = min(9999999, (wallet + paid) & 0xffffff)
                yield f'        {{{wallet}, {pot}, {doubled}, {paid}, {balance}}},'


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


def displayed(game):
    raw = game.read(game.sym['gDisplayedStringBattle'], 128).split(b'\xff', 1)[0]
    return ' '.join(''.join(ENGLISH.get(value, '?') for value in raw).split())


def trapped_legendary(driver, directory, code, output):
    work = directory / 'legendary'
    work.mkdir()
    elf = compile_probe(ROOT, work, '#define PROBE_LOSS 0\n#define PROBE_LEGENDARY 1\n' + code)
    game = Game(driver, elf, elf, output, work)
    try:
        def wait_for(predicate, label):
            for step in range(4000):
                if predicate():
                    return
                game.frames(3, A if step % 10 == 0 else 0)
            raise AssertionError((label, displayed(game), game.u32(game.sym['gBattleOutcome'])))
        wait_for(lambda: displayed(game).casefold() == 'magikarp appeared!', 'legendary introduction')
        def action():
            wait_for(lambda: controller(game, 'HandleInputChooseAction'), 'legendary action menu')
        action()
        choose_cell(game, 'gActionSelectionCursor', 3)
        wait_for(lambda: 'escape!' in displayed(game), 'Ghost holding Smoke Ball must stay')
        action()
        for slot in (0, 1):
            choose_cell(game, 'gActionSelectionCursor', 0)
            wait_for(lambda: controller(game, 'HandleInputChooseMove'), 'legendary move menu')
            choose_cell(game, 'gMoveSelectionCursor', slot)
            wait_for(lambda: 'But it failed!' in displayed(game), 'legendary escape move')
            action()
        assert game.read(game.sym['gBattleOutcome'], 1) == bytes([0])
        return ['Run', 'Teleport', 'Roar']
    finally:
        game.close()


def run(driver, output):
    output.mkdir(parents=True, exist_ok=True)
    fixtures = cases()
    code = (ROOT/'tools/polished_port/tests/suite3_context_fixture.c').read_text()
    rows = ['    {%d, %d, ITEM_%s, %d, %d},' % row for row in fixtures]
    code = code.replace('/* GENERATED_REWARD_CASES */', 'static const struct RewardCase sCases[] = {\n' + '\n'.join(rows) + '\n};')
    movement, encounters = roam_cases()
    for tag, name, rows in [('MOVEMENT', 'sMovement', movement), ('ENCOUNTER', 'sEncounter', encounters)]:
        values = ',\n'.join('    {' + ', '.join(map(str, row)) + '}' for row in rows)
        code = code.replace(f'/* GENERATED_{tag}_CASES */', f'static const u16 {name}[][{len(rows[0])}] = {{\n{values}\n}};')
    payouts = list(payout_cases())
    code = code.replace('/* PAYOUT CASES */', '\n'.join(payouts))
    medicine = medicine_cases()
    setup = setup_cases()
    code = code.replace('/* SETUP CASES */', '\n'.join(setup))
    policies = ai_policy_cases()
    code = code.replace('/* POLICY CASES */', '\n'.join(policies))
    ties = ai_tie_cases()
    code = code.replace('/* TIE CASES */', '\n'.join(ties))
    genders = gender_cases()
    code = code.replace('/* GENDER CASES */', '\n'.join(genders))
    code = code.replace('/* GENERATED_MEDICINE_CASES */', 'static const struct MedicineCase medicine[] = {\n' + '\n'.join(medicine) + '\n    };')
    with tempfile.TemporaryDirectory(prefix='polished-contexts-') as tmp:
        directory = Path(tmp)
        smart_dir = directory / 'smart-field-ai'
        smart_dir.mkdir()
        smart_field_ai = replay_smart_field_ai(driver, smart_dir, output, draws)
        blocked = trapped_legendary(driver, directory, code, output)
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
                    return struct.unpack('<13I', game.read(game.sym['gContextResult'], 52))
                def wait(predicate, label, key=A):
                    for frame in range(3000):
                        if predicate(): return
                        game.frames(4, key if frame % 8 == 0 else 0)
                    raise AssertionError((label, result()))
                wait(lambda: displayed(game).casefold() == ('a wild snorlax appeared!' if name == 'loss' else 'a wild magikarp appeared!'), 'wild introduction')
                wait(lambda: controller(game, 'HandleInputChooseAction'), 'action')
                assert result()[:3] == (1, 0, 0) and result()[6] == 0, result()
                eligible = name in ('win', 'capture')
                # Scatter coins with the actual move before every outcome. Focus Sash
                # keeps the first wild opponent alive for the win, capture and escape.
                choose_cell(game, 'gActionSelectionCursor', 0)
                wait(lambda: controller(game, 'HandleInputChooseMove'), 'Pay Day move')
                choose_cell(game, 'gMoveSelectionCursor', 0)
                wait(lambda: game.u32(game.sym['gPaydayMoney']) == 400, 'scattered coins')
                if name != 'loss':
                    wait(lambda: controller(game, 'HandleInputChooseAction'), 'second action')
                game.snapshot("before-action")
                # A legitimate win can produce zero Honey. Exercise an award through
                # a real battle by varying input timing, keeping every outcome check.
                for delay in range(16):
                    if delay:
                        game.restore("before-action")
                        game.frames(7 * delay)
                    if name == 'capture':
                        game.press(256, 8)  # R throws the only ball in the bag.
                    elif name != 'loss':
                        choose_cell(game, 'gActionSelectionCursor', 3 if name == 'escape' else 0)
                        if name != 'escape':
                            wait(lambda: controller(game, 'HandleInputChooseMove'), 'move')
                            choose_cell(game, 'gMoveSelectionCursor', 1)
                    wait(lambda: result()[0] == 2, 'battle outcome', B if name == 'capture' else A)
                    after = result()
                    assert after[3:6] == (expected, 0 if eligible else 16, 1), (name, after)
                    assert all(r in (*range(29, 40), *range(42, 47)) for r in after[10:12]), (name, after)
                    assert after[8:10] == (int(name == 'capture'), 1), (name, after)
                    if not eligible or after[7]:
                        break
                assert (after[7] > 0) == eligible, (name, after)
                assert after[12] == 1, ('link standings', after)
                wallet = game.u32(game.sym['gWalletResult'])
                wallet_delta = {'win': 800, 'capture': 800, 'escape': 0, 'loss': -99 * 120}
                expected_wallet = 1200123 + wallet_delta[name]
                assert wallet == expected_wallet, (name, wallet, expected_wallet)
                game.command('reset')
                wait(lambda: result()[0] == 3, 'Flash load')
                loaded = result()
                assert loaded[2] == 0 and loaded[4] == after[4] and loaded[7:] == after[7:], (name, after, loaded)
                assert game.u32(game.sym['gWalletResult']) == wallet, (name, 'saved wallet')
                outcomes[name] = dict(outcome=expected, wallet_after_reset=wallet, status=after[4], honey_in_bag=after[7],
                                      magikarp_delivered=bool(after[8]), everstone_retained=bool(after[9]), link_records_reloaded=bool(loaded[12]))
            finally:
                game.close()
    report = dict(source_commit=PIN, production_rom_sha256=hashlib.sha256((ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
                  smart_field_ai=smart_field_ai, legendary_blocked=blocked, trainer_setup_cases=len(setup), seeded_reward_cases=len(fixtures), payout_cases=len(payouts), wild_factory_cases=96, wild_gender_cases=len(genders), trainer_tie_cases=len(ties), trainer_policy_cases=len(policies), trainer_medicine_cases=len(medicine), roamer_movement_cases=len(movement),
                  roamer_encounter_bytes=len(encounters), simultaneous_beasts=2, outcomes=outcomes, flash_reloads=len(outcomes))
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(f'PASS: {len(fixtures)} reward cases, {len(payouts)} payout cases, {len(genders)} wild gender cases, {len(ties)} trainer tie cases, {len(policies)} scoring cases, {len(medicine)} trainer medicine cases, {len(movement)} roamer movements, 256 encounter bytes, two saved beasts, real win/capture/escape/loss and four Flash reloads')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    run(args.driver.resolve(), args.output.resolve())
