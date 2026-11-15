#!/usr/bin/env python3
"""Verify pinned binding countdown, damage pauses and live held-item rules."""
import argparse
import json
import re

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    commands = files['engine/battle/effect_commands.asm']
    trap = commands.split('\nBattleCommand_traptarget:', 1)[1].split('\nBattleCommand_recoil:', 1)[0]
    assert 'ld a, [hl]\n\tand a\n\tret nz\n\tcall CheckSubstituteOpp\n\tret nz' in trap
    assert 'ld a, HELD_PROLONG_WRAP\n\tcall GetItemBoostedDuration\n\tjr z, .got_count' in trap
    assert 'call BattleRandom\n\tand 1\n\tadd 4\n\tjr .got_count' in trap
    assert '.got_count\n\tld [hl], a' in trap
    substitute = files['engine/battle/move_effects/substitute.asm']
    assert 'ld hl, wPlayerWrapCount\n\tld de, wPlayerTrappingMove' in substitute
    assert 'ld hl, wEnemyWrapCount\n\tld de, wEnemyTrappingMove' in substitute
    assert '.player\n\txor a\n\tld [hl], a\n\tld [de], a' in substitute
    sub_check = commands.split('\nCheckSubstituteOpp:', 1)[1].split('\n', 7)[:7]
    assert '\tcall GetTrueUserAbility' in sub_check and '\tcp INFILTRATOR' in sub_check and '\tret z' in sub_check
    moves = re.findall(r'\bdbw (\w+),', trap)
    assert moves == ['WRAP', 'FIRE_SPIN', 'WHIRLPOOL']
    move_rows = source.default_lines(files['data/moves/moves.asm'])
    assert [line.split(',')[0].split()[-1] for line in move_rows if 'EFFECT_TRAP,' in line] == moves
    duration = commands.split('\nGetItemBoostedDuration:', 1)[1].split('\nPrintDoesntAffect:', 1)[0]
    assert 'call GetUserItem' in duration
    assert 'cp h\n\tld a, 5\n\tjr nz, .got_duration\n\tld a, c' in duration
    items = files['data/items/attributes.asm']
    assert '; GRIP CLAW\n\titem_attribute 4000, HELD_PROLONG_WRAP, 7,' in items
    assert '; BINDING BAND\n\titem_attribute 4000, HELD_BINDING_BAND, 0,' in items

    endturn = files['engine/battle/endturn.asm']
    prevent = endturn.split('\nPreventEndturnDamage:', 1)[1].split('\nHandleLeechSeed:', 1)[0]
    assert 'call GetTrueUserAbility\n\tcp MAGIC_GUARD\n\tcall nz, HasUserFainted\n\tret' in prevent
    wrap = endturn.split('\nHandleWrap:', 1)[1].split('\nHandleEncore:', 1)[0]
    gates = ['call PreventEndturnDamage\n\tret z', 'bit SUBSTATUS_SUBSTITUTE, a\n\tret nz',
             'dec [hl]', 'jr z, .print_text', 'farcall GetOpponentItemAfterUnnerve']
    offsets = [wrap.index(gate) for gate in gates]
    assert offsets == sorted(offsets)
    assert 'and 1 << SUBSTATUS_FLYING | 1 << SUBSTATUS_UNDERGROUND\n\tjr nz, .skip_anim' in wrap
    assert 'cp HELD_BINDING_BAND\n\tjr nz, .no_binding_band\n\tcall GetSixthMaxHP' in wrap
    assert '.no_binding_band\n\tcall GetEighthMaxHP' in wrap
    item_gate = commands.split('\nGetOpponentItemAfterUnnerve:', 1)[1].split('\nNoItem:', 1)[0]
    assert 'call StackCallOpponentTurn' in item_gate
    assert 'call GetUserItem\n\tcall GetOpponentAbility\n\tcp UNNERVE\n\tret nz' in item_gate
    assert 'ld hl, EdibleBerries\n\tcall IsInByteArray' in item_gate
    home = files['home/battle.asm']
    assert 'GetEighthMaxHP::\n\tcall GetQuarterMaxHP\n\tjr HalveBC' in home
    assert 'GetSixthMaxHP::\n\tcall GetThirdMaxHP\n\tjr HalveBC' in home
    assert 'FloorBC::\n\tld a, c\n\tor b\n\tret nz\n\tinc c\n\tret' in home

    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        source_moves=moves, scope='Binding countdown and residual damage in ordinary singles without active gimmicks.',
        rules=['Apply a random count of 4 or 5 with equal probability, or 7 with Grip Claw. Do not refresh an existing count.',
               'Magic Guard, fainting and Substitute return before decrement or release.',
               'Creating Substitute clears an existing bind. Infiltrator can apply a new bind through an existing Substitute, which then pauses it.',
               'Decrement first. Zero releases the target without residual damage; otherwise damage is floor(max HP / 8), minimum 1.',
               'Read the current opponent item at every damage tick; Binding Band changes the divisor to 6, minimum 1.',
               'Unnerve only rejects edible berries and does not suppress Binding Band.',
               'Fly and Dig skip the source animation, but do not pause the countdown or prevent residual damage.'],
        ordinary_duration_vectors=[dict(initial_source_count=n, damaging_turns=n-1, release_turn=n,
                                        target_remaining_damage_ticks=n-1) for n in [4, 5, 7]],
        adapter='Store one fewer native damage tick at application; pause before native decrement/release; read live effective Binding Band for scoped residual damage. Preserve native storage and cleanup.',
        gaps=['Initial hit/secondary-effect ordering and all immunity interactions, complete switching and Baton Pass cleanup, and source AI estimates remain separate audits.',
              'Native animations/messages remain presentation gaps, including source Fly/Dig residual-animation suppression.',
              'Optional item suppression, Gastro Acid, doubles, active gimmicks and other binding moves remain available; optional behavior is not certified as source content.',
              'An already-created counter keeps its stored value if an optional gimmick starts later; no retroactive duration conversion is introduced.'])
    return {source.ROOT/'tools/polished_port/manifests/binding_moves.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Wrap/Fire Spin/Whirlpool countdown, damage pauses and live Binding Band rules')
