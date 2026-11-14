#!/usr/bin/env python3
"""Verify pinned Perish Song countdown and ordered opponent immunity gates."""
import argparse
import json

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    rows = [line for line in source.default_lines(files['data/moves/moves.asm']) if line.startswith('move PERISH_SONG,')]
    assert len(rows) == 1
    assert [v.strip() for v in rows[0][5:].split(',')] == ['PERISH_SONG', 'EFFECT_PERISH_SONG', '0', 'NORMAL', '-1', '5', '0', 'STATUS']
    command = files['engine/battle/move_effects/perish_song.asm']
    assert 'ld [hl], 4' in command
    assert command.index('ld [hl], 4') < command.index('call BattleCommand_checkpriority')
    assert 'ld a, [hl]\n\tand a\n\tjr nz, .user_done' in command
    assert 'ld a, [de]\n\tand a\n\tjr nz, .opponent_done' in command
    assert 'ld a, [wAttackMissed]\n\tand a\n\tjr nz, .immune' in command
    assert 'ld a, 4\n\tld [de], a' in command
    assert '.opponent_done\n\tdec b\n\tjr z, .failed' in command
    effect = files['data/moves/effects.asm'].split('\nPerishSong:', 1)[1].split('\n\tendmove', 1)[0]
    assert effect.strip().splitlines() == ['checkobedience', '\tusedmovetext', '\tdoturn', '\tperishsong']
    gates = files['engine/battle/effect_commands.asm'].split('\nBattleCommand_checkpriority:', 1)[1].split('\nBattleCommand_effectchance:', 1)[0]
    assert 'cp $81\n\tjr c, .check_prankster' in gates
    assert 'call GetOpponentAbilityAfterMoldBreaker\n\tcp ARMOR_TAIL' in gates
    assert gates.index('cp ARMOR_TAIL') < gates.index('cp PRANKSTER') < gates.index('cp SOUNDPROOF')
    assert 'call GetTrueUserAbility\n\tcp PRANKSTER\n\tjr z, .prankster' in gates
    assert 'call GetOpponentAbilityAfterMoldBreaker\n\tcp SOUNDPROOF\n\tret nz' in gates
    prankster = gates.split('\n.prankster\n', 1)[1]
    assert 'call CheckIfTargetIsDarkType\n\tret nz' in prankster
    assert 'SOUNDPROOF' not in prankster and 'CheckHiddenOpponent' not in gates
    priority = files['engine/battle/core.asm'].split('\nGetMovePriority:', 1)[1].split('\nINCLUDE', 1)[0]
    assert 'cp PRANKSTER\n\tjr nz, .no_priority' in priority
    assert 'cp STATUS\n\tjr nz, .no_priority\n\tinc b' in priority
    countdown = files['engine/battle/endturn.asm'].split('\nHandlePerishSong:', 1)[1].split('\nHandleTrickRoom:', 1)[0]
    assert 'and a\n\tret z\n\tdec [hl]' in countdown
    assert 'pop af\n\tret nz\n\n\tcall GetMaxHP\n\tpredef_jump SubtractHPFromUser' in countdown
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Ordinary singles Perish Song opponent ability/type gates and independent user countdown.',
        rules=['Start the user counter even when the opponent is immune.',
               'Bypass the general hit check and Fly/Dig hiding; retain the listed source accuracy.',
               'Never refresh an existing counter; check opponent immunity only if that opponent has no counter.',
               'Positive-priority Armor Tail immunity precedes the Prankster branch.',
               'Prankster checks Dark typing, including hidden targets, and returns before Soundproof.',
               'Other users check opponent Soundproof with Mold Breaker respected.'],
        countdown=dict(source_initial=4, displayed_counts=[3, 2, 1, 0], faint_on_turn_including_use=4,
            adapter='Keep the native timer, initialized to 3 and displayed before decrement; it produces the same four end-turn counts.'),
        adapter='Replace only the scoped per-target immunity decision and defer the global priority cancellation to it. Existing move-result flags feed the native counter setter. Retain target IDs, saves, counters and capacities.',
        controls=['both battle sides', 'Mold Breaker', 'Soundproof user', 'repeat failure and PP', 'switch/recast', 'Protect/Substitute/hiding', 'native profiles', 'active Terastal', 'doubles'],
        gaps=['Native animation/message scheduling is retained; source checks the user counter before displaying opponent immunity.',
              'Complete end-turn faint ordering, Baton Pass, ability changes during a turn, called moves, AI choice, dynamic move types, formats and gimmicks remain separate audits.'])
    return {source.ROOT/'tools/polished_port/manifests/perish_song.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Perish Song user/opponent gates, Prankster branch and countdown')
