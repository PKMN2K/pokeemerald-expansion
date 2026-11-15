#!/usr/bin/env python3
"""Verify pinned Shed Shell and Ghost exemptions shared by player and AI switching."""
import argparse
import json

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    core = files['engine/battle/core.asm']
    switch = '\n'.join(source.default_lines(core.split('\nUserCanSwitch:', 1)[1].split('\nTryPlayerSwitch:', 1)[0]))
    gates = ['predef GetUserItemAfterUnnerve', 'cp HELD_SHED_SHELL\nret z',
             'cp RUN_AWAY\nret z', 'call CheckIfUserIsGhostType\nret z',
             'farcall CheckIfTrappedByAbility', 'ld a, [wPlayerWrapCount]',
             'ld a, BATTLE_VARS_SUBSTATUS2_OPP']
    offsets = [switch.index(gate) for gate in gates]
    assert offsets == sorted(offsets)
    ai = core.split('\nAI_UserCanSwitch:', 1)[1].split('\nUserCanSwitch:', 1)[0]
    assert 'farcall CheckAnyOtherAliveMons\n\tjr nz, UserCanSwitch\n\tor 1\n\tret' in ai
    player = core.split('\nTryPlayerSwitch:', 1)[1].split('\nBattleMenu_Run:', 1)[0]
    assert 'call SetPlayerTurn\n\tcall UserCanSwitch\n\tjr z, .try_switch' in player
    commands = files['engine/battle/effect_commands.asm']
    item = commands.split('\nGetUserItemAfterUnnerve::', 1)[1].split('\nNoItem:', 1)[0]
    assert 'call GetUserItem\n\tcall GetOpponentAbility\n\tcp UNNERVE\n\tret nz' in item
    assert 'ld hl, EdibleBerries\n\tcall IsInByteArray' in item and '\tret nc' in item
    assert 'SHED_SHELL' not in files['data/items/edible_berries.asm']
    assert '; SHED SHELL\n\titem_attribute 4000, HELD_SHED_SHELL, 0,' in files['data/items/attributes.asm']
    home = files['home/battle.asm']
    assert 'CheckIfUserIsGhostType::\n\tld a, GHOST\n\tjr CheckIfUserIsSomeType' in home
    type_check = home.split('\nCheckIfUserIsSomeType::', 1)[1].split('\nCheckPinch::', 1)[0]
    assert 'ld de, wEnemyMonType1' in type_check and 'ld de, wBattleMonType1' in type_check
    assert 'ld a, [de]\n\tinc de\n\tcp b\n\tret z\n\tld a, [de]\n\tcp b\n\tret' in type_check
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='AI voluntary switch eligibility for live Shed Shell and Ghost exemptions in ordinary singles without active gimmicks.',
        rules=['AI_UserCanSwitch requires another living party Pokemon and then uses the same UserCanSwitch rule as player selection.',
               'Shed Shell permits a switch before trapping abilities, Wrap or Mean Look are considered.',
               'Unnerve only suppresses edible berries; it does not disable Shed Shell.',
               'The Ghost test reads current battle types and permits a switch before those traps.',
               'Default-build Run Away remains another exemption; this suite preserves the earlier adapter.'],
        adapter='Extend the existing AI eligibility gate using live effective held items and types, retaining existing switch strategy, reserve validation, trap cleanup and player selection. Do not clear trap state or consume Shed Shell.',
        gaps=['Complete source AI strategy, scoring and estimates, and player/item eligibility under every optional suppression condition remain separate audits.',
              'General fleeing, forced switching, Pursuit and full switch timing remain separate; this exemption is not added to wild escape predicates.',
              'Native profiles, doubles, active gimmicks and optional Ingrain/Fairy Lock/Sky Drop/Commander/Battle Arena restrictions retain their native AI path.',
              'Klutz, Embargo, Trick-or-Treat and Soak are selected native extension controls, not certification of their complete source behavior.',
              'Optional moves sharing the permanent-trap field receive the same scoped exemption; their whole behavior is not certified.'])
    return {source.ROOT/'tools/polished_port/manifests/switch_exemptions.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned shared Shed Shell/Ghost switch exemptions, Unnerve and living-reserve gate')
