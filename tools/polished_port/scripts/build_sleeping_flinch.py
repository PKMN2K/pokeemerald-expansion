#!/usr/bin/env python3
"""Authenticate effective flinch before the pinned sleep action gate."""
import argparse
import json

import build_pokemon_learnsets as source
import build_frozen_flinch as frozen_flinch


def build():
    # Pins the full flinch block, active ability query and direct item/Stench path.
    frozen_flinch.build()
    files = source.sources()
    turn = files['engine/battle/effect_commands.asm'].split('\nBattleCommand_checkturn:', 1)[1].split('\nEndTurn:', 1)[0]
    assert turn.index('\n.no_recharge\n') < turn.index('\n.not_flinched\n') < turn.index('\n.not_asleep\n')
    sleep = turn.split('\n.not_flinched\n', 1)[1].split('\n.not_asleep\n', 1)[0]
    assert [line for line in source.default_lines(sleep) if line] == [
        'ld a, BATTLE_VARS_STATUS', 'call GetBattleVarAddr', 'ld a, [hl]',
        'and SLP_MASK', 'jr z, .not_asleep', 'call GetTrueUserAbility',
        'sub EARLY_BIRD', 'jr nz, .no_early_bird', 'dec [hl]', 'jr z, .woke_up',
        '.no_early_bird', 'dec [hl]', 'jr z, .woke_up', 'xor a',
        'ld [wNumHits], a', 'ld de, ANIM_SLP', 'call FarPlayBattleAnimation',
        'jr .fast_asleep', '.woke_up', 'and a', 'jr nz, .woke_up_no_early_bird',
        'farcall BeginAbility', 'farcall ShowAbilityActivation',
        '.woke_up_no_early_bird', 'ld hl, WokeUpText', 'call StdBattleTextbox',
        'farcall EndAbility', 'ldh a, [hBattleTurn]', 'and a', 'jr nz, .enemy1',
        'call UpdateBattleMonInParty', 'ld hl, UpdatePlayerHUD', 'jr .ok1',
        '.enemy1', 'call UpdateEnemyMonInParty', 'ld hl, UpdateEnemyHUD',
        '.ok1', 'call CallBattleCore', 'ld a, $1', 'ldh [hBGMapMode], a',
        'jr .not_asleep', '.fast_asleep', 'ld hl, FastAsleepText',
        'call StdBattleTextbox', 'ld a, BATTLE_VARS_MOVE_ANIM', 'call GetBattleVar',
        'cp SLEEP_TALK', 'jr z, .not_asleep', 'call CantMove', 'jmp EndTurn']
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Pending effective flinch on sleeping battlers in ordinary singles without either active gimmick.',
        rules=[
            'Effective flinch ends the action before either ordinary or Early Bird sleep decrement, waking, or Sleep Talk execution. Sleep status and PP remain unchanged.',
            'Active Steadfast still raises Speed when sleep would otherwise prevent acting.',
            'Active Inner Focus rejects the pending flinch before sleep is processed; the ability query accounts for Neutralizing Gas without the previous attacker\'s Mold Breaker bypass.',
            'The next non-flinched attempt resumes ordinary sleep progression. Repeated flinches do not consume sleep turns.',
            'The source move-effect sleep gate remains separate: direct item/Stench flinch can reach this action gate.'],
        adapter='Extend the existing frozen-battler early-flinch dispatch to sleep, before the shared sleep/freeze canceler. Reuse the existing active-ability, flinch and Steadfast handling; leave non-flinched sleep processing unchanged.',
        gaps=[
            'This closes flinch versus sleep ordering only. Sleep application/duration, rest, sleep clause, complete Sleep Talk selection, full item/Stench eligibility and probability, and original presentation remain separate audits.',
            'Healthy-battler ordering, recharge clearing, earlier native cancelers, the full source action sequence, RNG stream and AI remain separate work.',
            'Native-profile battles, doubles and either active gimmick retain native ordering. Optional moves can produce a flinch or be blocked by this general status rule; their own effects are not certified.'])
    return {source.ROOT/'tools/polished_port/manifests/sleeping_flinch.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned effective flinch precedes sleep decrement, waking and Sleep Talk')
