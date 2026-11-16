#!/usr/bin/env python3
"""Verify the pinned PoisonPowder/Corrosion powder-immunity exception."""
import argparse
import json

import build_pokemon_learnsets as source
import build_magic_bounce as reflection


def build():
    files = source.sources()
    powders = reflection.move_list(files, 'data/moves/powder_moves.asm')
    assert powders == ['POISONPOWDER', 'SLEEP_POWDER', 'SPORE', 'STUN_SPORE']
    moves = {row[0]: row for row in [[v.strip() for v in line[5:].split(',')] for line in
             source.default_lines(files['data/moves/moves.asm']) if line.startswith('move ')]}
    assert moves['POISONPOWDER'][1] == 'EFFECT_POISON'
    assert moves['POISONPOWDER'][3] == 'POISON' and moves['POISONPOWDER'][7] == 'STATUS'
    scripts = files['data/moves/effects.asm']
    effect = scripts.split('\nDoPoison:\n', 1)[1].split('\nToxic:', 1)[0]
    assert list(source.default_lines(effect)) == ['checkobedience', 'usedmovetext', 'doturn', 'hastarget',
        'checkhit', 'checkpowder', 'checkpriority', 'bounceback', 'checksafeguard', 'poison', 'endmove']
    commands = files['engine/battle/effect_commands.asm']
    powder = commands.split('\nBattleCommand_checkpowder:', 1)[1].split('\nBattleCommand_resettypematchup:', 1)[0]
    corrosion = powder.split('\n.not_sing\n', 1)[1]
    assert [line for line in source.default_lines(corrosion) if line] == [
        'cp THUNDER_WAVE', 'jr z, BattleCommand_resettypematchup', 'cp TOXIC', 'jr z, .check_corrosion',
        'cp POISONPOWDER', 'jr nz, .powder', '.check_corrosion', 'ld b, a', 'call GetTrueUserAbility',
        'cp CORROSION', 'ret z', 'ld a, b', 'cp TOXIC', 'jr z, BattleCommand_resettypematchup',
        '.powder', 'ld hl, PowderMoves', 'call IsInByteArray', 'ret nc']
    matchup = commands.split('\n_CheckTypeMatchup:', 1)[1].split('\n.skip_powder\n', 1)[0]
    gates = ['ld hl, PowderMoves', 'cp HELD_SAFETY_GOGGLES', 'cp GRASS',
             'call GetOpponentAbilityAfterMoldBreaker', 'cp OVERCOAT']
    offsets = [matchup.index(gate) for gate in gates]
    assert offsets == sorted(offsets) and matchup.count('cp GRASS') == 2
    reset = commands.split('\nBattleCommand_resettypematchup:', 1)[1].split('\nINCLUDE "data/moves/powder_moves.asm"', 1)[0]
    assert 'ld a, ATKFAIL_IMMUNE\n\tld [wAttackMissed], a\n\tret' in reset
    assert 'EndMoveEffect' not in reset and 'failuretext' not in reset
    bounce = files['engine/battle/move_effects/magic_bounce.asm']
    assert 'ld a, [wAttackMissed]\n\tcp ATKFAIL_PROTECT\n\tret z' in bounce
    assert 'ATKFAIL_IMMUNE' not in bounce and 'call ResetTurn' in bounce
    # Status application still runs after the powder gate; this is not a
    # blanket Corrosion implementation or a claim about all immunity stages.
    status = commands.split('\nCanStatusTarget:', 1)[1].split('\nBattleCommand_draintarget:', 1)[0]
    assert 'cp EFFECT_POISON\n\tjr z, .check_user_corrosion' in status
    assert '.check_user_corrosion\n\tcall GetTrueUserAbility' in status
    assert 'cp CORROSION\n\tjr nz, .not_corrosive\n\tpop af\n\tjr .immunities_done' in status
    assert '.immunities_done\n\tld a, BATTLE_VARS_STATUS_OPP\n\tcall GetBattleVar\n\tand a' in status
    safeguard = files['engine/battle/move_effects/safeguard.asm']
    assert 'call GetTrueUserAbility\n\tcp INFILTRATOR\n\tret' in safeguard
    assert 'BattleCommand_checksafeguard:\n\tcall SafeCheckSafeguard\n\tret z' in safeguard
    assert 'CORROSION' not in safeguard and safeguard.rstrip().endswith('jmp EndMoveEffect')
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Poison-type PoisonPowder execution with actual Corrosion in singles without active gimmicks; powder immunity only.',
        powder_moves=powders, corrosion_powder_exceptions=['POISONPOWDER'],
        rules=['PoisonPowder and Toxic return from checkpowder when the actual user has Corrosion; Toxic is not a powder move.',
               'The skipped powder matchup checks Safety Goggles, either Grass type and Overcoat after Mold Breaker.',
               'Sleep Powder, Spore and Stun Spore still perform their powder checks; Mold Breaker only suppresses Overcoat.',
               'Powder immunity records a failed matchup but does not end the move. Magic Bounce still activates on an otherwise eligible immune target.',
               'Returned moves use the actual reflector ability; Magic Bounce does not inherit the original attacker Corrosion.',
               'Protect, Safeguard, accuracy and existing major status still prevent successful poisoning in the tested direct-move cases.'],
        adapter='Skip only the powder blocker during execution for matching source PoisonPowder and actual Corrosion. Preserve shared AI estimates and the later status/substitute handlers. Exclude optional Magic Coat returns, doubles, active gimmicks and dynamically retyped moves.',
        gaps=['The separate corrosion_substitute suite covers Toxic/PoisonPowder Substitute bypass. Status-stage ability/item immunity bypasses, Toxic Orb and damaging poison effects remain separate audits.',
              'The full source accuracy/random stream, general status rules, returned-effect ordering, AI and presentation remain separate audits.',
              'Optional moves and mechanics retain native behavior; optional protective combinations outside the tests are not certified.'])
    return {source.ROOT/'tools/polished_port/manifests/corrosion_powder.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned four-move powder list, Corrosion exception, matchup gates and Magic Bounce continuation')
