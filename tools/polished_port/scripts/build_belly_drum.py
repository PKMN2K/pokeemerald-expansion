#!/usr/bin/env python3
"""Verify pinned Belly Drum HP costs and ability-aware Attack-limit behavior."""
import argparse
import json
import re

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    effect = files['data/moves/effects.asm'].split('\nBellyDrum:', 1)[1].split('\n\tendmove', 1)[0]
    assert effect.strip().splitlines() == ['checkobedience', '\tusedmovetext', '\tdoturn', '\tbellydrum']
    command = files['engine/battle/move_effects/belly_drum.asm']
    assert 'call GetHalfMaxHP\n\tcall CompareHP\n\tjr c, .failed\n\tjr z, .failed' in command
    amount = re.search(r'ld b, \$([0-9a-f]+) \| ATTACK\n\tld a, STAT_SKIPTEXT\n\tcall _ForceRaiseStat', command)
    assert amount
    force_stages = (int(amount[1], 16) >> 4) + 1
    assert 'ld a, [wFailedMessage]\n\tand a\n\tjr nz, .failed' in command
    assert command.index('call _ForceRaiseStat') < command.index('ld a, [wFailedMessage]') < command.index('predef SubtractHPFromUser')
    assert 'call GetHalfMaxHP\n\tpredef SubtractHPFromUser\n\tcall UpdateUserInParty' in command
    assert 'call GetTrueUserAbility\n\tcp CONTRARY\n\tld hl, BellyDrumContraryText' in command
    commands = files['engine/battle/effect_commands.asm']
    assert '_ForceRaiseStat: ; no-optimize stub jump\n\tjr ChangeStat' in commands
    assert 'ChangeStat:\n; b contains stat to alter, or -1 if it should be read from the move script\n\tfarjp FarChangeStat' in commands
    stats = files['engine/battle/stats.asm']
    assert 'call GetTrueUserAbility\n\tcp CONTRARY\n\tjmp nz, .perform_change\n\tld a, b\n\txor STAT_LOWER' in stats
    assert '; Perform the stat change\n\tld a, [wChangedStat]\n\tand $f0\n\tswap a\n\tinc a' in stats
    assert '.raise_loop\n\tld a, [hl]\n\tcp MAX_STAT_LEVEL\n\tjr z, .queue_mirror_herb' in stats
    assert '.lower_loop\n\tld a, [hl]\n\tdec a\n\tjr z, .stat_change_done' in stats
    assert '.stat_change_done\n\t; Amount of stat levels changed is in b\n\tld a, b\n\tand a\n\tjr z, .stat_change_failed' in stats
    assert '.stat_change_failed\n\tld a, 1\n\tld [wFailedMessage], a' in stats
    constants = files['constants/battle_constants.asm']
    neutral = int(re.search(r'DEF BASE_STAT_LEVEL EQU (\d+)', constants)[1])
    maximum = int(re.search(r'DEF MAX_STAT_LEVEL EQU (\d+)', constants)[1])
    assert (neutral, maximum, force_stages) == (7, 13, 16)
    home = files['home/battle.asm']
    assert 'GetHalfMaxHP::\n\tcall GetMaxHP\nHalveBC::\n\tsrl b\n\trr c\nFloorBC::\n\tld a, c\n\tor b\n\tret nz\n\tinc c' in home
    assert 'CompareHP::\n; return c if HP<bc, z if HP=bc, nc+nz if HP>bc' in home
    assert 'GetTrueUserAbility:\n; Get true user ability after Neutralizing Gas.' in home
    assert 'cp NEUTRALIZING_GAS\n\tjr nz, .not_suppressed\n\tpop af\n\tpush hl\n\tfarcall AbilityCanBeSuppressed\n\tpop hl\n\tret c\n\txor a\n\tret' in home
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Ordinary Belly Drum Attack-limit eligibility, effective Contrary, HP boundary/cost and selected reset regressions.',
        hp_cost='max(1, floor(maxHP / 2)); current HP must be strictly greater; no HP is spent when no stat change is possible.',
        source_stat_levels=dict(minimum=1, neutral=neutral, maximum=maximum), source_forced_stages=force_stages,
        stage_cases=[dict(starting_stage=n-neutral, contrary=contrary,
                         succeeds=n != (1 if contrary else maximum),
                         ending_stage=(1 if contrary else maximum)-neutral)
                     for contrary in (False, True) for n in range(1, maximum+1)],
        adapter='Use the effective ability when checking the existing upper-limit comparator, which reverses the bound for Contrary. Keep the existing target max-stat effect and HP arithmetic; no new state or identifiers. Native profiles and active gimmicks retain the native gate.',
        gaps=['Original Contrary success/failure text, animation and HP-versus-stat/item-event ordering remain native and need a separate presentation/ordering audit.',
              'Complete held-item/ability interactions, called/reflected moves, disobedience, global stat/damage/AI rules and source RNG remain separate audits.',
              'Expansion-only formats and gimmicks remain available without certifying them as source behavior.'])
    return {source.ROOT/'tools/polished_port/manifests/belly_drum.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Belly Drum effective-ability stat limits and HP-cost contract')
