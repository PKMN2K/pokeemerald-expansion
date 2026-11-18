#!/usr/bin/env python3
"""Pin effective flinch and active ability checks before source thawing."""
import argparse
import json

import build_pokemon_learnsets as source
import build_self_thaw as self_thaw
import build_flinch_hit as flinch_hit


def build():
    # Authenticates the complete flinch/freeze blocks and the separate item path.
    self_thaw.build()
    flinch_hit.build()
    files = source.sources()
    turn = files['engine/battle/effect_commands.asm'].split('\nBattleCommand_checkturn:', 1)[1].split('\nEndTurn:', 1)[0]
    flinch = turn.split('\n.no_recharge\n', 1)[1].split('\n.not_flinched\n', 1)[0]
    assert flinch.index('call GetTrueUserAbility') < flinch.index('cp INNER_FOCUS') < flinch.index('cp STEADFAST')
    assert flinch.endswith('\n.skip_steadfast\n\tcall CantMove\n\tjmp EndTurn\n')
    ability = files['home/battle.asm'].split('\nGetTrueUserAbility:', 1)[1].split('\nGetOpponentAbilityAfterMoldBreaker::', 1)[0]
    assert [line for line in source.default_lines(ability) if line] == [
        'farcall GetFutureSightUser', 'jr z, .not_external', 'xor a', 'ret',
        '.not_external', 'call StackCallOpponentTurn', 'GetOpponentAbility::',
        'ld a, BATTLE_VARS_ABILITY_OPP', 'call GetBattleVar', 'push af',
        'ld a, BATTLE_VARS_ABILITY', 'call GetBattleVar', 'cp NEUTRALIZING_GAS',
        'jr nz, .not_suppressed', 'pop af', 'push hl', 'farcall AbilityCanBeSuppressed',
        'pop hl', 'ret c', 'xor a', 'ret', '.not_suppressed', 'pop af', 'ret']
    steadfast = files['engine/battle/abilities.asm'].split('\nSteadfastAbility:\n', 1)[1].split('\nStatUpAbility:', 1)[0]
    assert [line for line in source.default_lines(steadfast) if line] == ['SpeedUpAbility:', 'ld b, SPEED']
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Pending flinch on a frozen battler in ordinary singles without either active gimmick.',
        rules=['The source resolves effective flinch before entering its freeze block, so neither the natural thaw roll nor self-thaw runs on that attempt.',
               'The flinch block calls CantMove and EndTurn; freeze and PP remain unchanged, while active Steadfast raises Speed.',
               'GetTrueUserAbility rechecks the acting battler after Neutralizing Gas, without applying the opponent\'s Mold Breaker bypass. Active Inner Focus discards the pending flinch and permits the freeze stage.',
               'King\'s Rock, other HELD_FLINCH_UP items and Stench can set the flag through their direct FlinchTarget path even while the target is frozen.',
               'A later non-flinched attempt still uses the ordinary source freeze roll or self-thaw rule.'],
        adapter='Before the existing sleep/freeze action gate, route a frozen source-profile battler with pending flinch through the existing flinch canceler. Clear a pending flinch protected by active Inner Focus so it cannot recur at the later native stage. Preserve the existing status/PP/Steadfast scripts and actual opposing-battler format gate.',
        gaps=['Only the frozen-battler branch is moved. Sleep, healthy-battler cancellation order, recharge clearing, full item/Stench eligibility and probability, source RNG stream, AI and presentation remain separate audits.',
              'Earlier native cancellation stages and the complete source action sequence are not certified. Existing freeze eligibility, incoming thaw attacks and optional suppression/ability combinations remain separate work.',
              'Doubles and either active gimmick retain native ordering. Optional flinch producers in ordinary source singles inherit this general frozen-status ordering; their move effects are not otherwise certified.'])
    return {source.ROOT/'tools/polished_port/manifests/frozen_flinch.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned effective flinch precedes thawing and rechecks active Inner Focus')
