#!/usr/bin/env python3
"""Verify default Polished Crystal Run Away switch eligibility and suppression."""
import argparse
import json

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    core = files['engine/battle/core.asm']
    raw = core.split('\nUserCanSwitch:', 1)[1].split('\nTryPlayerSwitch:', 1)[0]
    assert 'if !DEF(FAITHFUL)\n\tcall GetTrueUserAbility\n\tcp RUN_AWAY\n\tret z\nendc' in raw
    default = '\n'.join(source.default_lines(raw))
    gates = ['cp HELD_SHED_SHELL', 'call GetTrueUserAbility', 'cp RUN_AWAY',
             'call CheckIfUserIsGhostType', 'farcall CheckIfTrappedByAbility',
             'ld a, [wPlayerWrapCount]', 'ld a, BATTLE_VARS_SUBSTATUS2_OPP', 'bit SUBSTATUS_CANT_RUN, a']
    offsets = [default.index(gate) for gate in gates]
    assert offsets == sorted(offsets)
    assert 'cp RUN_AWAY\nret z' in default
    ai = core.split('\nAI_UserCanSwitch:', 1)[1].split('\nUserCanSwitch:', 1)[0]
    assert 'farcall CheckAnyOtherAliveMons\n\tjr nz, UserCanSwitch' in ai
    player = core.split('\nTryPlayerSwitch:', 1)[1].split('\nBattleMenu_Run:', 1)[0]
    assert 'call SetPlayerTurn\n\tcall UserCanSwitch\n\tjr z, .try_switch' in player
    home = files['home/battle.asm'].split('\nGetTrueUserAbility:', 1)[1].split('\nGetOpponentAbilityAfterMoldBreaker::', 1)[0]
    assert 'call StackCallOpponentTurn\nGetOpponentAbility::' in home
    assert 'cp NEUTRALIZING_GAS\n\tjr nz, .not_suppressed' in home
    assert 'farcall AbilityCanBeSuppressed' in home
    traps = files['engine/battle/effect_commands.asm'].split('\nCheckIfTrappedByAbility:', 1)[1].split('\nSetBattleDraw:', 1)[0]
    for ability in ['MAGNET_PULL', 'ARENA_TRAP', 'SHADOW_TAG']:
        assert 'cp ' + ability in traps
    assert 'jmp CheckIfUserIsSteelType' in traps and 'call CheckAirborne' in traps
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Ordinary singles Run Away voluntary switch eligibility, including the AI eligibility gate.',
        rules=['Run Away returns success before ability traps, wrapping and the opponent CANT_RUN flag.',
               'The source player and AI use the same UserCanSwitch rule; the AI additionally requires a living replacement.',
               'GetTrueUserAbility respects Neutralizing Gas; no ability popup or trap removal is part of the switch exemption.',
               'Wrap damage still occurs while the affected Pokemon remains in battle.'],
        adapter='Keep native trap state and party validation. Add a scoped ability exemption to the shared trap checks and the AI switch-eligibility gate; do not modify general AI estimates or switching priorities.',
        gaps=['Wild fleeing, forced switches, source AI decisions/estimates and full source trapping-ability interactions remain separate audits.',
              'Shed Shell, Ghost exemptions, item suppression and all non-Run Away switching rules are not certified here.',
              'Native profiles, doubles, active gimmicks and optional Ingrain/Fairy Lock/Sky Drop/Commander restrictions retain native behavior.',
              'Other optional moves using the native permanent-trap field share its switch exemption in scoped singles; this does not certify those moves as source content.'])
    return {source.ROOT/'tools/polished_port/manifests/run_away.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned default Run Away switch eligibility and suppression')
