#!/usr/bin/env python3
"""Pin the source action-time Inner Focus check for any pending flinch."""
import argparse
import json

import build_pokemon_learnsets as source
import build_frozen_flinch as frozen_flinch


def build():
    # Authenticates the entire flinch block, true-user ability query and
    # Steadfast branch, plus all thirteen source move identities and item path.
    frozen_flinch.build()
    files = source.sources()
    commands = files['engine/battle/effect_commands.asm']
    turn = commands.split('\nBattleCommand_checkturn:', 1)[1].split('\nEndTurn:', 1)[0]
    flinch = turn.split('\n.no_recharge\n', 1)[1].split('\n.not_flinched\n', 1)[0]
    assert flinch.index('res SUBSTATUS_FLINCHED, [hl]') < flinch.index('call GetTrueUserAbility')
    assert 'cp INNER_FOCUS\n\tjr z, .not_flinched' in flinch
    assert 'BATTLE_VARS_STATUS' not in flinch
    assert turn.index('\n.not_flinched\n') < turn.index('and SLP_MASK') < turn.index('bit FRZ, [hl]')
    assert 'cp STEADFAST' in flinch and 'call CantMove' in flinch
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='The active Inner Focus recheck for pending flinch when the acting battler reaches the flinch gate, in ordinary singles without either active gimmick.',
        rules=[
            'The source clears the pending flinch flag and checks GetTrueUserAbility before calling CantMove. This check has no sleep/freeze or other nonvolatile-status prerequisite.',
            'Active Inner Focus rejects the flinch even if the preceding hit used Mold Breaker; the acting-battler query does not apply that bypass.',
            'Neutralizing Gas can suppress Inner Focus. Without active protection, the existing flinch cancellation and Steadfast response apply.',
            'Rejecting flinch lets later action checks run and does not cancel a charged move through the flinch path. It does not guarantee that another blocker will allow the move.'],
        adapter='Centralize the active Inner Focus recheck in the flinch canceler. The existing early sleep/freeze dispatch calls that canceler and continues status processing on success; other battlers use their existing flinch stage. Clear only a protected pending flinch before cancellation.',
        gaps=[
            'Full flinch application/eligibility, recharge clearing, source probability and RNG stream, earlier native action cancelers, the complete cancellation order, AI and original presentation remain separate audits.',
            'Native-profile battles, doubles and either active gimmick retain native behavior. Optional flinch producers inherit this general acting-battler check without certifying their own effects.',
            'Optional Gastro Acid and Ability Shield retain the engine\'s existing suppression/protection behavior. Their source parity is not asserted; broader ability/item combinations remain separate work.'])
    return {source.ROOT/'tools/polished_port/manifests/flinch_focus.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned active Inner Focus rejects pending flinch independently of nonvolatile status')
