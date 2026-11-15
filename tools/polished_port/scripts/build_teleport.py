#!/usr/bin/env python3
"""Verify pinned Teleport's wild escape gates and trainer switch distinction."""
import argparse
import json

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    teleport = files['engine/battle/move_effects/teleport.asm']
    assert 'ld a, [wBattleType]\n\tcp BATTLETYPE_TRAP' in teleport
    assert 'jr nc, .failed' in teleport
    assert 'ld a, [wBattleMode]\n\tdec a\n\tjr nz, .trainer_battle' in teleport
    wild = teleport.split('\n.trainer_battle', 1)[0]
    gates = ['call GetTrueUserAbility', 'cp RUN_AWAY\n\tjr z, .run_away',
             'ld a, BATTLE_VARS_SUBSTATUS2_OPP', 'bit SUBSTATUS_CANT_RUN, a\n\tjr nz, .failed',
             'call CheckIfTrappedByAbility\n\tjr z, .failed', '.run_away',
             'call UpdateBattleMonInParty', 'ld [wBattleEnded], a', 'call SetBattleDraw']
    offsets = [wild.index(gate) for gate in gates]
    # Match the label, not earlier branches naming it.
    offsets[5] = wild.index('\n.run_away\n')
    assert offsets == sorted(offsets)
    assert 'WrapCount' not in wild and 'GetUserItem' not in wild and 'GetOpponentItem' not in wild
    assert 'FledFromBattleText' in wild
    trainer = teleport.split('\n.trainer_battle\n', 1)[1].split('\n.failed\n', 1)[0]
    assert 'call CheckAnyOtherAliveMons\n\tjr z, .failed' in trainer
    assert 'ld a, 1 << SWITCH_DEFERRED\n\tjmp SetDeferredSwitch' in trainer
    assert 'CheckIfTrappedByAbility' not in trainer and 'SWITCH_BATON_PASS' not in trainer
    assert 'call AnimateFailedMove\n\tjmp PrintButItFailed' in teleport
    true_ability = files['home/battle.asm'].split('\nGetTrueUserAbility:', 1)[1].split('\nGetOpponentAbilityAfterMoldBreaker::', 1)[0]
    assert 'NEUTRALIZING_GAS' in true_ability
    traps = files['engine/battle/effect_commands.asm'].split('\nCheckIfTrappedByAbility:', 1)[1].split('\nSetBattleDraw:', 1)[0]
    assert 'call .CheckTrap\n\tret nz\n\tcall .CheckOpponentTrap\n\tjr z, .not_trapped' in traps
    assert 'call CheckIfUserIsGhostType' in traps
    types = files['constants/battle_constants.asm'].split('\tconst BATTLETYPE_NORMAL', 1)[1].split('; BattleVarPairs', 1)[0]
    assert types.index('BATTLETYPE_TRAP') < types.index('BATTLETYPE_FORCEITEM') < types.index('BATTLETYPE_NEVER_SHINY') < types.index('BATTLETYPE_LEGENDARY')
    description = files['data/moves/descriptions.asm'].split('\nTeleportDescription:', 1)[1].split('\n\tdone', 1)[0]
    assert description == '\n\ttext "A move for fleeing"\n\tnext "from battle."'
    header = '// Generated from the pinned Teleport description.\n#ifndef GUARD_POLISHED_TELEPORT_H\n#define GUARD_POLISHED_TELEPORT_H\n#define POLISHED_TELEPORT_DESCRIPTION "A move for fleeing\\nfrom battle."\n#endif\n'
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Ordinary wild singles Teleport execution, including recordings, without active gimmicks or optional field restrictions.',
        rules=['Wild Teleport ends the battle by fleeing on either side; it does not require a replacement or compare levels.',
               'After the special battle-type prohibition, unsuppressed Run Away bypasses move and ability traps.',
               'Without Run Away, an existing Mean Look flag blocks escape before checking trapping abilities.',
               'Ability eligibility includes the source type/grounding and mutual-cancellation rules.',
               'Wrap and escape items do not participate in the Teleport eligibility check. Smoke Ball and Shed Shell do not bypass Mean Look or trapping abilities.',
               'Trainer Teleport switches if another living party Pokemon exists; it does not check those traps and does not Baton Pass.'],
        adapter='Route scoped player wild Teleport to the existing flee script and supply source eligibility on both sides. Reuse native move cancellation, party persistence and teleport outcomes; retain trainer and optional-format paths.',
        gaps=['Complete source special encounter classification and its TRAP/FORCEITEM/NEVER_SHINY/LEGENDARY prohibition remain separate. Native encounter flags and the optional no-running guard remain intact.',
              'Source trainer AI choices, full switching/party persistence audits, Pursuit timing and source flee presentation remain separate.',
              'The selected native trainer Gen8 switch path is verified; overriding optional legacy trainer generation presets is not part of this adapter.',
              'Doubles, link/facility/special formats, active gimmicks, Ingrain, Fairy Lock, Commander and Sky Drop retain native execution. Trick-or-Treat is used only to exercise an existing trap after a live type change.'])
    return {source.ROOT/'tools/polished_port/manifests/teleport.json': json.dumps(report, indent=2)+'\n',
            source.ROOT/'include/constants/polished_teleport.h': header}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Teleport wild escape ordering, item/Wrap exclusions and trainer switch distinction')
