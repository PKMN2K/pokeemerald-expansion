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
    assert 'cp BATTLETYPE_ROAMING\n\tjr nz, .but_it_failed' in roar
    assert 'ld a, 1 << SWITCH_DEFERRED | 1 << SWITCH_TARGET | 1 << SWITCH_FORCED' in roar
    bounce = files['engine/battle/move_effects/magic_bounce.asm']
    assert 'cp ATKFAIL_PROTECT\n\tret z' in bounce
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Roar execution hit gates in singles without active gimmicks; selected native forced-switch regressions.',
        rules=['Roar calls checkhit before priority, reflection and forced switching.',
               'Protect blocks Roar even with No Guard or Substitute, and prevents Magic Bounce reflection.',
               'No Guard on either side bypasses Fly/Dig, but Lock-On is checked after Fly/Dig and cannot bypass them.',
               'Roar bypasses Substitute and the ordinary accuracy/evasion calculation after those gates.',
               'Trainer Roar needs a living replacement. Ordinary wild Roar requires user level at least target level on either side.',
               'Suction Cups is checked after Mold Breaker; selected Soundproof/Mold Breaker cases retain the native result.'],
        adapter='Apply Protect and hidden-target rules during move execution, preserving native AI estimation and the existing switch/party machinery.',
        gaps=['Source special encounter prohibitions and the roaming opponent level exemption remain separate from ordinary wild coverage.',
              'Full source reflection, Prankster/Soundproof ordering, turn priority, deferred switch timing, candidate selection, ability activation order and presentation remain separate audits.',
              'In particular, Roar reflection behind Substitute still needs the source reflection eligibility rule.',
              'Doubles, active gimmicks, Whirlwind and optional protective effects retain native behavior. Whirlwind is not a move in the pinned source.',
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
