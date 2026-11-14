#!/usr/bin/env python3
"""Verify pinned Mean Look application, hidden-target and reflection gates."""
import argparse
import json

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    rows = [line for line in source.default_lines(files['data/moves/moves.asm']) if line.startswith('move MEAN_LOOK,')]
    assert len(rows) == 1
    assert [v.strip() for v in rows[0][5:].split(',')] == ['MEAN_LOOK', 'EFFECT_MEAN_LOOK', '0', 'NORMAL', '-1', '5', '0', 'STATUS']
    effect = files['data/moves/effects.asm'].split('\nMeanLook:', 1)[1].split('\n\tendmove', 1)[0]
    assert effect.split() == ['checkobedience', 'usedmovetext', 'doturn', 'hastarget', 'checkpriority', 'bounceback', 'arenatrap']
    command = files['engine/battle/move_effects/mean_look.asm']
    gates = ['call CheckHiddenOpponent\n\tjr nz, .failed',
             'ld a, BATTLE_VARS_SUBSTATUS2\n\tcall GetBattleVarAddr',
             'bit SUBSTATUS_CANT_RUN, [hl]\n\tjr nz, .failed',
             'call CheckIfTargetIsGhostType\n\tjr z, .immune',
             'set SUBSTATUS_CANT_RUN, [hl]', 'call AnimateCurrentMove']
    offsets = [command.index(gate) for gate in gates]
    assert offsets == sorted(offsets)
    commands = files['engine/battle/effect_commands.asm']
    assert 'and 1 << SUBSTATUS_FLYING | 1 << SUBSTATUS_UNDERGROUND\n\tret' in commands
    sub = commands.split('\nCheckSubstituteOpp:', 1)[1].split('\n.no_sound_move', 1)[0]
    assert 'call GetTrueUserAbility\n\tcp INFILTRATOR\n\tret z' in sub
    assert '\tdb MEAN_LOOK\n' not in files['data/moves/substitute_bypass_moves.asm']
    bounce = files['engine/battle/move_effects/magic_bounce.asm']
    gates = ['call GetOpponentAbilityAfterMoldBreaker', 'call CheckHiddenOpponent\n\tret nz',
             'ld hl, SubstituteBypassMoves', 'call CheckSubstituteOpp\n\tret nz',
             'bit SUBSTATUS_IN_ABILITY, a\n\tret nz', 'set SUBSTATUS_IN_ABILITY, [hl]']
    offsets = [bounce.index(gate) for gate in gates]
    assert offsets == sorted(offsets)
    core = files['engine/battle/core.asm']
    for label, own, other in [('NewEnemyMonStatus:', 'Enemy', 'Player'), ('NewBattleMonStatus:', 'Player', 'Enemy')]:
        reset = core.split('\n' + label, 1)[1]
        reset = reset.split('res SUBSTATUS_CANT_RUN, [hl]', 1)[0]
        assert f'ld hl, w{own}SubStatus1\n\tld [hli], a\n\tld [hli], a\n\tld [hli], a\n\tld [hl], a' in reset
        assert f'ld hl, w{other}SubStatus2\n\t' in reset
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Ordinary singles Mean Look application and selected reflection/switch-release controls.', pp=5,
        rules=['The effect script has no checkhit: Substitute and Protect do not prevent application.',
               'Fly and Dig fail at the effect itself, including No Guard and Lock-On cases.',
               'Ghost targets are immune; repeating an existing trap fails.',
               'Magic Bounce does not reflect through Substitute unless the attacker bypasses it with Infiltrator; Mold Breaker suppresses reflection.',
               'A reflected Mean Look still bypasses Substitute on the original user; reflection cannot recur.',
               'Ordinary switching by the trapper releases the target.'],
        adapter='Keep native target-side escapePrevention/battlerPreventingEscape instead of the source user-side CANT_RUN bit. Use move-specific execution gates, retaining native profiles, optional trapping moves, doubles and active gimmicks.',
        gaps=['Source failure messages/animations, complete called-move/ability ordering and AI estimates/choices remain separate.',
              'Baton Pass, Run Away/Shed Shell switching, fleeing, forced switches and broader trapping lifecycle remain separate audits.',
              'Accuracy modifiers and all priority interactions beyond existing native handling are not certified by this suite.'])
    return {source.ROOT/'tools/polished_port/manifests/mean_look.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Mean Look application, hidden-target and reflection gates')
