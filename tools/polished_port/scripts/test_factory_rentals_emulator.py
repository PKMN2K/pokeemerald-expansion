#!/usr/bin/env python3
"""Replay v3.2.3 NewRentalTeam and drive the cartridge selection screen."""
import argparse
import hashlib
import json
import re
from collections import Counter
import struct
import tempfile
from itertools import accumulate, product
from pathlib import Path

import build_tower_parties as source
from audit_moves_machines import compile_probe
from test_johto_demo_emulator import A, DOWN, LEFT, RIGHT, START, Game
from test_shift_switch_emulator import ENGLISH
from test_tower_parties_emulator import draws, select, source_set_rows


def reference():
    tiers, masks = source.catalog()
    offsets = list(accumulate(map(len, tiers), initial=0))
    sets = sum(tiers, [])
    # BT_GetPointsForTrainer: trainers 1–3, 4–6, and 7 have different rewards.
    points = (1, 1, 1, 2, 2, 2, 3)
    cases, rows = [], set()
    retries = repeated_items = 0
    for seed, (wins, swaps) in enumerate(product((0, 1, 2, 7, 14, 28, 42, 255, 65535),
                                                  (0, 5, 6, 11, 12, 36, 99)), start=1):
        group, trainer = divmod(wins + 5, 7)
        row = min(group + points[trainer] + swaps // 6, 8)
        rows.add(row)
        slots = [(mask >> shift) & 3 for mask in masks[row - 1] for shift in (0, 2, 4)]
        rng = draws(seed)
        for last in range(5, 0, -1):
            pick = select(rng, last + 1)
            slots[last], slots[pick] = slots[pick], slots[last]
        while True:
            ids = [offsets[t] + select(rng, len(tiers[t])) for t in slots]
            if len({sets[i]['base'] for i in ids}) == 6:
                break
            retries += 1
        items = [sets[i]['item'] for i in ids if sets[i]['item'] != 'ITEM_NONE']
        repeated_items += len(set(items)) < len(items)
        cases.append((wins, swaps, ids))
    return sets, cases, sorted(rows), retries, repeated_items


def reference_opponents(sets, drafts):
    tiers, masks = source.catalog()
    offsets = list(accumulate(map(len, tiers), initial=0))
    records = json.loads(source.trainers.species.MANIFEST.read_text())['records']
    types = {r['target_species']: r['types'] for r in records}
    counters = Counter()
    cases = []
    openings = (0, 6, 7, 13, 14, 20, 21, 27, 28, 42, 255, 65535)
    for index, (_, _, offer) in enumerate(drafts):
        opening = openings[index % len(openings)]
        history = []
        available = offer
        for stage in range(7):
            rng = draws(500 + len(cases))
            wins = opening + stage
            row = min(7, wins // 7 + (0, 0, 0, 1, 1, 1, 2)[wins % 7])
            mask = masks[row][next(rng) & 1]
            slots = [(mask >> shift) & 3 for shift in (0, 2, 4)]
            for last in (2, 1):
                pick = select(rng, last + 1)
                slots[last], slots[pick] = slots[pick], slots[last]
            while True:
                team = [offsets[t] + select(rng, len(tiers[t])) for t in slots]
                if len({sets[i]['base'] for i in team}) != 3:
                    continue
                if set(team[:2]).intersection(available):
                    counters['rental_rejections'] += 1
                    continue
                if set(team[:2]).intersection(history):
                    counters['history_rejections'] += 1
                    continue
                break
            counters['third_slot_repeats'] += team[2] in available + history
            # Reproduce the pinned outer loop: DE points at the same first
            # entry throughout, while B shortens the prefix counted via HL.
            ordered = []
            for i in reversed(team):
                first, second = types[sets[i]['species']]
                ordered.extend((first, second if second != first else None))
            candidate = ordered[0]
            highest = max(ordered[:length].count(candidate) for length in range(6, 0, -1))
            best = candidate if highest > 1 else 'TYPE_NONE'
            hint = min(opening // 7, 4)
            if hint == 4:
                counts = Counter(t for t in ordered if t)
                counters['other_shared_type_ignored'] += max(counts.values()) > 1 and highest == 1
                if highest > 1:
                    hint = 5
            counters[f'hint_{hint}'] += 1
            cases.append((opening, team, hint, best))
            history = (history + team)[-9:]
            available = offer[:3] + team
    return cases, counters


def source_scout_text():
    asm = source.trainers.archive.authenticated_sources()['engine/events/battle_tower/battle_tower.asm']['text']
    charmap = (source.ROOT/'charmap.txt').read_text().split('@ Hiragana')[0]
    chars = {c: int(n, 16) for c, n in re.findall(r"^'(.)'\s*=\s*([0-9A-F]{2})", charmap, re.M)}
    chars.update({"'": 0xb4, ' ': 0})
    labels = ('ExpectThese3', 'ExpectThese2', 'ExactMonUsingMove', 'SomeMonUsingMove', 'NoTypePreference', 'PrefersType')
    rows = []
    for label in labels:
        body = asm.split(f'.{label}:', 1)[1].split('\tprompt', 1)[0]
        parts = []
        for line in body.strip().splitlines():
            cmd, arg = line.strip().split(' ', 1)
            if cmd == 'text_ram':
                if arg == 'wStringBuffer1':
                    parts.append('{1}' if label == 'PrefersType' else '{2}')
                else:
                    parts.append({'wOTPartyMonNicknames': '{1}', 'wOTPartyMonNicknames + MON_NAME_LENGTH': '{2}',
                                  'wOTPartyMonNicknames + MON_NAME_LENGTH * 2': '{3}'}[arg])
            else:
                assert cmd in ('text', 'line', 'cont')
                parts.append(('' if cmd == 'text' else ' ') + arg.strip('"'))
        sentence = ' '.join(''.join(parts).replace('#mon', 'Pokémon').split()).replace('- type', '-type')
        encoded = []
        for token in re.findall(r'\{[123]\}|.', sentence):
            encoded.extend((0xfd, int(token[1]) + 1) if len(token) == 3 else (chars[token],))
        rows.append('    {' + ', '.join(map(str, [*encoded, 255])) + '},')
    return '\n'.join(rows)


def run(driver, output):
    sets, cases, rows, retries, repeats = reference()
    opponents, counts = reference_opponents(sets, cases)
    assert retries and repeats and all(counts[k] for k in ('rental_rejections', 'history_rejections', 'third_slot_repeats', 'other_shared_type_ignored'))
    assert all(counts[f'hint_{i}'] for i in range(6))
    output.mkdir(parents=True, exist_ok=True)
    c = (source.ROOT/'tools/polished_port/tests/factory_rentals_fixture.c').read_text()
    c = c.replace('/* SOURCE SETS */', source_set_rows(sets)).replace('/* DRAFT CASES */', '\n'.join(
        f'    {{{wins}, {swaps}, {{{", ".join(map(str, team))}}}}},' for wins, swaps, team in cases))
    c = c.replace('/* SCOUT TEXT */', source_scout_text()).replace('/* OPPONENT CASES */', '\n'.join(
        '    {' + str(opening) + ', {' + ', '.join(map(str, team)) + '}, ' + f'{hint}, {best}' + '},'
        for opening, team, hint, best in opponents))
    with tempfile.TemporaryDirectory() as tmp:
        directory = Path(tmp)
        elf = compile_probe(source.ROOT, directory, c)
        game = Game(driver, elf, elf, output, directory)
        try:
            for phase in (1, 2):
                for _ in range(200):
                    game.frames(12)
                    state, error, actual, expected, count, carries, flash, rental_offset = struct.unpack(
                        '<8I', game.read(game.sym['gRentalAudit'], 32))
                    assert not error, (phase, error, actual, expected, count)
                    if state == phase:
                        break
                else:
                    raise AssertionError(('Factory timeout', phase, state, count))
                if phase == 1:
                    assert count == len(cases) and carries == 6
                    carries_checked = carries
                    verified = game.u32(game.sym['gCheckedOpponents'])
                    assert verified == len(opponents)
                    game.command('reset')
            assert flash == 3
            game.frames(160)
            raw = game.read(game.sym['gStringVar4'], 120).split(b'\xff', 1)[0]
            scout = ''.join(ENGLISH.get(c, '?') for c in raw)
            assert scout.startswith('You can expect to see ') and ', ' in scout and ' and ' in scout, scout
            game.screenshot('opponent-scout')
            game.press(A, 60)
            game.screenshot('opponent-scout-final-line')
            game.press(START)
            state, error, _, _, _, _, _, rental_offset = struct.unpack('<8I', game.read(game.sym['gRentalAudit'], 32))
            assert (state, error) == (3, 0), (state, error)
            # The native selector copies the chosen IDs back to the three saved
            # player slots only once the player accepts the confirmation dialog.
            stride = game.u32(game.sym['gRentalStride'])
            def saved_ids():
                rentals = game.u32(game.sym['gSaveBlock2Ptr']) + rental_offset
                return [int.from_bytes(game.read(rentals + i * stride, 2), 'little') for i in range(6)]
            draft = saved_ids()
            game.frames(120)
            for moves in ((RIGHT,) * 4, (LEFT,) * 3, (RIGHT,) * 2):
                for key in (*moves, A, DOWN, A):
                    game.press(key, 32)
            game.frames(90)
            game.screenshot('rental-confirmation')
            game.press(A, 1)
            for _ in range(120):
                game.frames(1)
                if saved_ids()[0] == draft[4]:
                    break
            order = [draft.index(i) + 1 for i in saved_ids()[:3]]
            assert order == [5, 2, 4], (order, draft, saved_ids())
        finally:
            game.close()
    result = dict(source_commit=source.trainers.archive.source.PIN,
                  production_rom_sha256=hashlib.sha256((source.ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
                  drafts=len(cases), draft_rows=rows,
                  whole_draft_retries=retries, repeated_item_drafts=repeats, tower_factory_carry_cases=carries_checked,
                  flash_rentals=flash, selection_order=order, opponent_teams=verified,
                  scout_messages=[counts[f'hint_{i}'] for i in range(6)],
                  rental_rejections=counts['rental_rejections'], history_rejections=counts['history_rejections'],
                  third_slot_repeats=counts['third_slot_repeats'], other_shared_type_ignored=counts['other_shared_type_ignored'], attendant_dialogue=scout)
    (output/'report.json').write_text(json.dumps(result, indent=2) + '\n')
    print(f'PASS: {len(cases)} Factory drafts, {verified} opponent teams and scouting hints, Flash recovery and menu picks {order}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    run(args.driver.resolve(), args.output.resolve())
