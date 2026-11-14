#!/usr/bin/env python3
"""Verify pinned Foresight hidden gates, raw accuracy and identification stages."""
import argparse
import json

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    rows = [line for line in source.default_lines(files['data/moves/moves.asm']) if line.startswith('move FORESIGHT,')]
    assert len(rows) == 1
    row = [v.strip() for v in rows[0][5:].split(',')]
    assert row == ['FORESIGHT', 'EFFECT_FORESIGHT', '0', 'NORMAL', '-1', '40', '0', 'STATUS']
    assert 'db \\5 ; accuracy' in files['data/moves/moves.asm']
    accuracy = int(row[4]) & 255
    assert accuracy == 255
    effect = files['data/moves/effects.asm'].split('\nForesight:', 1)[1].split('\n\tendmove', 1)[0]
    order = ['\tcheckhit', '\tcheckpriority', '\tbounceback', '\tforesight']
    offsets = [effect.index(command) for command in order]
    assert offsets == sorted(offsets)
    command = files['engine/battle/move_effects/foresight.asm']
    assert 'ld a, [wAttackMissed]\n\tand a\n\tjr nz, .failed' in command
    assert 'call CheckHiddenOpponent\n\tjr nz, .failed' in command
    assert 'bit SUBSTATUS_IDENTIFIED, [hl]\n\tjr nz, .failed' in command
    assert 'set SUBSTATUS_IDENTIFIED, [hl]' in command
    commands = files['engine/battle/effect_commands.asm']
    assert 'and 1 << SUBSTATUS_FLYING | 1 << SUBSTATUS_UNDERGROUND\n\tret' in commands
    checkhit = commands.split('\nBattleCommand_checkhit:', 1)[1].split('\nBattleCommand_checkpriority:', 1)[0]
    assert checkhit.index('call .NoGuardCheck') < checkhit.index('call .FlyDigMoves') < checkhit.index('call .LockOn')
    assert 'bit SUBSTATUS_IDENTIFIED, a\n\tjr nz, .avoid_evasion_boost' in checkhit
    assert '.avoid_evasion_boost\n\tld a, c\n\tcp 7\n\tjr c, .check_opponent_unaware\n.reset_evasion\n\tld c, 7' in checkhit
    assert 'ld a, BATTLE_VARS_MOVE_ACCURACY\n\tcall GetBattleVar\n\tldh [hMultiplicand + 2], a' in checkhit
    assert 'call DoStatChangeMod\n\tadd $11\n\tcall MultiplyAndDivide' in checkhit
    assert 'ld a, 100\n\tcall BattleRandomRange\n\tcp b\n\tret c' in checkhit
    assert 'DoStatChangeMod:\n\tld a, b\n\tcp 7\n\tjr nc, .higher\n\tln a, 2, 9\n\tsub b' in commands
    assert '.higher\n\tln a, 1, 11\n\tadd b' in commands
    math = files['home/math.asm'].split('\nMultiplyAndDivide::', 1)[1]
    assert 'swap a\n\tand $f' in math and 'and $f\n\tldh [c], a\n\tld b, 4\n\tcall Divide' in math
    home = files['home/battle.asm'].split('\nGetFixedMoveStruct::', 1)[1].split('\nGetCurMoveFixedCategory::', 1)[0]
    assert 'call FarCopyBytes' in home and 'MOVE_ACCURACY' not in home
    skin = files['engine/battle/abilities.asm'].split('\nWonderSkinAbility:', 1)[1].split('\nSwiftSwimAbility:', 1)[0]
    assert 'ld a, BATTLE_VARS_MOVE_ACCURACY\n\tcall GetBattleVarAddr\n\tld [hl], 50' in skin
    assert '\tdb FORESIGHT\n' in files['data/moves/substitute_bypass_moves.asm']
    assert 'call CheckHiddenOpponent\n\tret nz' in files['engine/battle/move_effects/magic_bounce.asm']
    types = files['data/types/type_matchups.asm'].split('\tdb $fe', 1)[1]
    assert 'db NORMAL,    GHOST,     NO_EFFECT' in types and 'db FIGHTING,  GHOST,     NO_EFFECT' in types
    assert 'bit SUBSTATUS_IDENTIFIED, a\n\tjr nz, .end' in commands
    header = '#ifndef GUARD_POLISHED_FORESIGHT_H\n#define GUARD_POLISHED_FORESIGHT_H\n\n'
    header += '// Generated from pinned Polished Crystal; the listed -1 is stored as a byte.\n'
    header += f'#define POLISHED_FORESIGHT_RAW_ACCURACY {accuracy}\n\n#endif\n'
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Ordinary singles Foresight hit/effect gates and accuracy stage treatment after identification.',
        listed_accuracy=-1, raw_accuracy=accuracy, pp=40,
        rules=['Fly and Dig reject identification even after No Guard or Lock-On.',
               'The raw accuracy byte goes through checkhit; it is not an unconditional bypass in this pinned implementation.',
               'Wonder Skin replaces the raw base with 50 before stage modifiers.',
               'Identification ignores positive target evasion, preserves negative evasion and leaves user accuracy changes active.',
               'The combined accuracy-minus-evasion stage is capped to -6..6, then uses exact (3+n)/3 or 3/(3-n) fractions.',
               'Identification removes only Normal/Fighting versus Ghost immunity; repeat use fails.'],
        raw_stage_checks=[dict(stage=stage, base=base, chance=min(100, base*(3+max(0,stage))//(3+max(0,-stage))))
                          for base in [accuracy,50] for stage in range(-6,7)],
        adapter='Keep the public move-data sentinel, native IDs and volatile flag. Adapt the scoped runtime accuracy payload, stage fractions and hidden gate. Retain native profiles, doubles, active gimmicks and the optional Miracle Eye flag.',
        gaps=['Source failure text, animations and complete reflected/called-move ordering remain separate.',
              'Affection, other accuracy item/ability/weather modifiers, global accuracy/RNG, Baton Pass and AI accuracy estimates/choices remain separate audits.',
              'Extra formats, species, moves and gimmicks remain available without certifying them as source content.'])
    return {source.ROOT/'include/constants/polished_foresight.h': header,
            source.ROOT/'tools/polished_port/manifests/foresight.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Foresight hidden gates, raw accuracy and identification stage fractions')
