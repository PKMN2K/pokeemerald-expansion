#!/usr/bin/env python3
"""Verify pinned Roar hit gates and selected unchanged forced-switch rules."""
import argparse
import json

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    effect = files['data/moves/effects.asm'].split('\nRoar:\n', 1)[1].split('\nSkillSwap:', 1)[0]
    assert list(source.default_lines(effect)) == ['checkobedience', 'usedmovetext', 'doturn', 'hastarget',
                                           'checkhit', 'checkpriority', 'bounceback', 'roar', 'endmove']
    commands = files['engine/battle/effect_commands.asm']
    hit = commands.split('\nBattleCommand_checkhit:', 1)[1].split('\n.Protect:', 1)[0]
    gates = ['call .Protect', 'call .Substitute', 'call .NoGuardCheck', 'call .FlyDigMoves',
             'call .LockOn', 'cp EFFECT_ROAR\n\tret z']
    offsets = [hit.index(gate) for gate in gates]
    assert offsets == sorted(offsets)
    priorities = [line for line in source.default_lines(files['data/moves/priorities.asm'])
                  if line.startswith('db ROAR,')]
    assert len(priorities) == 1 and int(priorities[0].split(',')[1]) == -6
    priority = files['engine/battle/core.asm'].split('\nGetMovePriority:', 1)[1].split('\nINCLUDE', 1)[0]
    assert 'cp PRANKSTER\n\tjr nz, .no_priority' in priority
    assert 'cp STATUS\n\tjr nz, .no_priority\n\tinc b' in priority
    gates = commands.split('\nBattleCommand_checkpriority:', 1)[1].split('\nBattleCommand_effectchance:', 1)[0]
    assert 'cp $81\n\tjr c, .check_prankster' in gates
    assert gates.index('cp ARMOR_TAIL') < gates.index('cp PRANKSTER') < gates.index('cp SOUNDPROOF')
    assert 'call GetTrueUserAbility\n\tcp PRANKSTER\n\tjr z, .prankster' in gates
    assert 'call GetOpponentAbilityAfterMoldBreaker\n\tcp SOUNDPROOF\n\tret nz' in gates
    prankster = gates.split('\n.prankster\n', 1)[1]
    assert 'cp STATUS\n\tret nz\n\tcall CheckIfTargetIsDarkType\n\tret nz' in prankster
    assert 'SOUNDPROOF' not in prankster and 'jmp BattleCommand_failuretext' in prankster
    failure = commands.split('\nBattleCommand_failuretext:', 1)[1].split('\nBattleCommand_applydamage:', 1)[0]
    assert 'cp EFFECT_DOUBLE_HIT\n\tjmp nz, EndMoveEffect' in failure
    protect = commands.split('\n.Protect:', 1)[1].split('\n.Substitute:', 1)[0]
    assert 'bit SUBSTATUS_PROTECT, a\n\tret' in protect
    substitute = commands.split('\n.Substitute:', 1)[1].split('\n.LockOn:', 1)[0]
    assert 'cp SWAGGER' in substitute and 'cp EFFECT_TRAP' in substitute and 'ROAR' not in substitute
    hidden = commands.split('\n.FlyDigMoves:', 1)[1].split('\n.WeatherAccCheck:', 1)[0]
    assert 'SUBSTATUS_FLYING | 1 << SUBSTATUS_UNDERGROUND' in hidden and 'ROAR' not in hidden
    no_guard = commands.split('\n.NoGuardCheck:', 1)[1].split('\n.AntiMinimize:', 1)[0]
    assert 'call GetTrueUserAbility\n\tcp NO_GUARD\n\tret z\n\tcall GetOpponentAbility\n\tcp NO_GUARD' in no_guard
    roar = files['engine/battle/move_effects/roar.asm']
    assert 'call GetOpponentAbilityAfterMoldBreaker\n\tcp SUCTION_CUPS' in roar
    assert 'call CheckAnyOtherAliveOpponentMons\n\tjr nz, .trainer_success' in roar
    assert 'ld a, c\n\tcp b\n\tjr nc, .wild_succeed' in roar
    wild = [line for line in source.default_lines(roar.split('\n.wild_got_party_vars\n', 1)[1].split('\n.wild_succeed\n', 1)[0]) if line]
    assert wild == ['ld a, c', 'cp b', 'jr nc, .wild_succeed', 'pop af',
                    'jr z, .but_it_failed', 'ld a, [wBattleType]',
                    'cp BATTLETYPE_ROAMING', 'jr nz, .but_it_failed', 'push af']
    turn = roar.split('\n.wild\n', 1)[1].split('\n.wild_got_party_vars\n', 1)[0]
    assert 'ldh a, [hBattleTurn]\n\tand a\n\tpush af\n\tjr z, .wild_got_party_vars' in turn
    assert 'ld a, b\n\tld b, c\n\tld c, a' in turn
    assert 'ld a, 1 << SWITCH_DEFERRED | 1 << SWITCH_TARGET | 1 << SWITCH_FORCED' in roar
    bounce = files['engine/battle/move_effects/magic_bounce.asm']
    assert 'cp ATKFAIL_PROTECT\n\tret z' in bounce
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Roar execution hit gates and Normal-type Prankster/Dark/Soundproof ordering in singles without active gimmicks; roaming-opponent level exemption; selected native forced-switch regressions.',
        rules=['Roar calls checkhit before priority, reflection and forced switching.',
               'Protect blocks Roar even with No Guard or Substitute, and prevents Magic Bounce reflection.',
               'No Guard on either side bypasses Fly/Dig, but Lock-On is checked after Fly/Dig and cannot bypass them.',
               'Roar bypasses Substitute and the ordinary accuracy/evasion calculation after those gates.',
               'Roar priority is -6, raised to -5 by Prankster; Armor Tail does not block either negative priority.',
               'Prankster Roar fails against Dark targets before Magic Bounce; its non-Dark branch returns before the Soundproof check.',
               'Trainer Roar needs a living replacement. Ordinary wild Roar requires user level at least target level on either side.',
               'Only a roaming opponent may use Roar against a higher-level target; the player still needs an equal or higher level, including after reflection.',
               'Suction Cups is checked after Mold Breaker; selected Soundproof/Mold Breaker cases retain the native result.'],
        adapter='Apply Protect and hidden-target rules during move execution. Resolve source Prankster Dark immunity before reflection and bypass Soundproof for non-Dark targets of Prankster Roar. Allow the source roaming opponent level exception only for Roar in singles without active gimmicks. Preserve native AI estimation and the existing switch/party machinery. Clear completed reflection target bookkeeping in both profiles so the reflector can attack on later turns.',
        gaps=['Source special encounter prohibitions, roaming encounter selection, flee AI and post-battle roaming persistence remain separate audits.',
              'Full source reflected-move execution, turn ordering beyond Roar priority, deferred switch timing, candidate selection, ability activation order and presentation remain separate audits.',
              'Roar reflection through Substitute is covered by the subsequent Magic Bounce suite: Roar belongs to the source sound bypass list.',
              'Doubles, active gimmicks and Whirlwind retain native behavior. Whirlwind is not a move in the pinned source; broader combinations with optional protective effects remain unaudited.',
              'Dynamically retyped Roar retains native priority-immunity handling; other source moves using checkpriority remain separate audits.',
              'Native AI hit estimates remain unchanged; passing runtime tests does not certify full AI parity.'])
    return {source.ROOT/'tools/polished_port/manifests/roar_hit_rules.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Roar Protect, No Guard/Fly/Dig/Lock-On ordering and selected switching rules')
