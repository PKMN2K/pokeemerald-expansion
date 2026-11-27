#!/usr/bin/env python3
"""Replay v3.2.3 NewRentalTeam and drive the cartridge selection screen."""
import argparse
import hashlib
import json
import struct
import tempfile
from itertools import accumulate, product
from pathlib import Path

import build_tower_parties as source
from audit_moves_machines import compile_probe
from test_johto_demo_emulator import A, DOWN, LEFT, RIGHT, Game
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


def run(driver, output):
    sets, cases, rows, retries, repeats = reference()
    assert retries and repeats
    output.mkdir(parents=True, exist_ok=True)
    c = (source.ROOT/'tools/polished_port/tests/factory_rentals_fixture.c').read_text()
    c = c.replace('/* SOURCE SETS */', source_set_rows(sets)).replace('/* DRAFT CASES */', '\n'.join(
        f'    {{{wins}, {swaps}, {{{", ".join(map(str, team))}}}}},' for wins, swaps, team in cases))
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
                    game.command('reset')
            assert flash == 3
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
                  flash_rentals=flash, selection_order=order)
    (output/'report.json').write_text(json.dumps(result, indent=2) + '\n')
    print(f'PASS: {len(cases)} Factory drafts, six Tower carry cases, Flash rental recovery and menu picks {order}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    run(args.driver.resolve(), args.output.resolve())
