#!/usr/bin/env python3
"""Verify source Corrosion's Substitute bypass for Toxic and PoisonPowder."""
import argparse
import json

import build_pokemon_learnsets as source
import build_magic_bounce as reflection


def build():
    files = source.sources()
    moves = [[v.strip() for v in line[5:].split(',')] for line in
             source.default_lines(files['data/moves/moves.asm']) if line.startswith('move ')]
    selected = [row for row in moves if row[1] in ('EFFECT_POISON', 'EFFECT_TOXIC')]
    assert [row[0] for row in selected] == ['POISONPOWDER', 'TOXIC']
    assert all(row[3] == 'POISON' and row[7] == 'STATUS' for row in selected)
    scripts = files['data/moves/effects.asm']
    for label, after, command in [('DoPoison', 'Toxic', 'poison'), ('Toxic', 'DoBurn', 'toxic')]:
        body = scripts.split('\n' + label + ':\n', 1)[1].split('\n' + after + ':', 1)[0]
        assert [line for line in source.default_lines(body) if line] == [
            'checkobedience', 'usedmovetext', 'doturn', 'hastarget', 'checkhit', 'checkpowder',
            'checkpriority', 'bounceback', 'checksafeguard', command, 'endmove']
    commands = files['engine/battle/effect_commands.asm']
    sub = commands.split('\n.Substitute:', 1)[1].split('\n.LockOn:', 1)[0]
    assert [line for line in source.default_lines(sub) if line] == [
        'call CheckSubstituteOpp', 'jr z, .not_blocked', 'ld a, BATTLE_VARS_MOVE_ANIM',
        'call GetBattleVar', 'cp SWAGGER', 'jr z, .blocked', 'ld a, BATTLE_VARS_MOVE_EFFECT',
        'call GetBattleVar', 'cp EFFECT_TRAP', 'jr z, .blocked', '.not_blocked', 'xor a', 'ret',
        '.blocked', 'ld a, 1', 'and a', 'ret']
    status = commands.split('\nCanStatusTarget:', 1)[1].split('\nBattleCommand_draintarget:', 1)[0]
    branch = status.split('\n.check_psn_status_move\n', 1)[1].split('\n.not_corrosive\n', 1)[0]
    assert [line for line in source.default_lines(branch) if line] == [
        'ld a, BATTLE_VARS_MOVE_EFFECT', 'call GetBattleVar', 'cp EFFECT_POISON',
        'jr z, .check_user_corrosion', 'cp EFFECT_TOXIC', 'jr nz, .not_corrosive',
        '.check_user_corrosion', 'call GetTrueUserAbility', '.check_corrosion', 'cp CORROSION',
        'jr nz, .not_corrosive', 'pop af', 'jr .immunities_done']
    assert status.index('and h\n\tjmp nz, .already_statused') < status.index('.check_psn_status_move\n')
    assert status.index('call CheckSubstituteOpp') > status.index('\n.not_corrosive\n')
    assert '.immunities_done\n\tld a, BATTLE_VARS_STATUS_OPP\n\tcall GetBattleVar\n\tand a\n\tld hl, ButItFailedText\n\tret' in status
    poison = commands.split('\n_BattleCommand_poison:', 1)[1].split('\nBattleCommand_paralyze:', 1)[0]
    assert 'ld hl, CanPoisonTarget\n\tjr StatusTargetVerbose' in poison
    toxic = files['engine/battle/move_effects/toxic.asm']
    assert toxic.startswith('BattleCommand_toxic:\n\tld a, 1 << PSN | 1 << TOX\n\tcall _BattleCommand_poison\n\tret nz')
    verbose = commands.split('\nStatusTargetVerbose:', 1)[1].split('\nBattleCommand_raisesubnoanim:', 1)[0]
    assert 'ld a, [wAttackMissed]\n\tand a\n\tjr nz, .failed_ineffective' in verbose
    safeguard = files['engine/battle/move_effects/safeguard.asm']
    assert 'call GetTrueUserAbility\n\tcp INFILTRATOR\n\tret' in safeguard
    assert 'CORROSION' not in safeguard and safeguard.rstrip().endswith('jmp EndMoveEffect')
    bypass = reflection.move_list(files, 'data/moves/substitute_bypass_moves.asm')
    assert not {'POISONPOWDER', 'TOXIC'} & set(bypass)
    check_sub = commands.split('\nCheckSubstituteOpp:', 1)[1].split('\nCheckUserMove:', 1)[0]
    assert 'call GetTrueUserAbility\n\tcp INFILTRATOR\n\tret z' in check_sub and 'CORROSION' not in check_sub
    bounce = files['engine/battle/move_effects/magic_bounce.asm']
    assert 'call CheckSubstituteOpp\n\tret nz' in bounce
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Substitute bypass for Poison-type Toxic and PoisonPowder with actual Corrosion in singles without active gimmicks.',
        moves=[row[0] for row in selected],
        rules=['Only the two source poison-status effects take the direct-user Corrosion branch in CanStatusTarget.',
               'The general checkhit Substitute branch blocks Swagger and trapping effects, so neither poison-status move exits there.',
               'Corrosion skips the later Substitute check and leaves the substitute intact while applying poison to its owner.',
               'Earlier Protect, Safeguard and accuracy gates still apply; an existing major status is not overwritten.',
               'Corrosion does not bypass the separate Magic Bounce Substitute gate. A protected reflector is poisoned without reflecting; returned moves do not inherit the original user ability.'],
        adapter='Use one execution-only eligibility helper at both the move canceler and trynonvolatilestatus Substitute checks. Preserve shared Substitute/AI helpers, reflection eligibility and the later native status-immunity checks. Exclude optional Magic Coat returns, doubles, active gimmicks and retyped moves.',
        gaps=['Corrosion status-stage ability/item immunity bypasses, Toxic Orb, damaging poison effects, general status rules and their healing lifecycle remain separate audits.',
              'Full source accuracy/RNG, reflected-effect ordering beyond the tested cases, AI decisions and presentation remain separate audits.',
              'Optional mechanics retain native paths; broader Magic Coat combinations are not certified.'])
    return {source.ROOT/'tools/polished_port/manifests/corrosion_substitute.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Toxic/PoisonPowder Corrosion Substitute bypass and separate hit/reflection gates')
