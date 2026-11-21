#!/usr/bin/env python3
"""Authenticate action ordering and retain the full Suite 2 command inventory."""
import argparse
import json
import re

import build_battle_suite as source
import build_attack_suite as attacks

ROOT = source.ROOT
MANIFEST = ROOT / 'tools/polished_port/manifests/action_resolution.json'


def ordered(text, tokens):
    positions = [text.index(token) for token in tokens]
    assert positions == sorted(set(positions)), f'Action order changed: {tokens}'


def block(text, start, end):
    return text.split(start, 1)[1].split(end, 1)[0]


def validate_source(files):
    commands = files['engine/battle/effect_commands.asm']['text']
    abilities = files['engine/battle/abilities.asm']['text']
    core = files['engine/battle/core.asm']['text']
    postfaint = block(commands, 'BattleCommand_postfainteffects:', 'BattleCommand_posthiteffects:')
    ordered(postfaint, ['call HasOpponentFainted', 'bit SUBSTATUS_DESTINY_BOND', 'predef SubtractHPFromUser', '\n.no_dbond\n', 'farjp RunFaintAbilities'])
    faint = block(abilities, 'RunFaintAbilities:', 'RunHitAbilities:')
    ordered(faint, ['call _RunFaintUserAbilities', 'call _RunFaintOpponentAbilities'])
    assert 'MAGIC_GUARD' not in faint
    assert 'cp DAMP\n\tret z' in faint and 'SubtractHPFromUser_OverrideFaintOrder' in faint
    posthit = block(commands, 'BattleCommand_posthiteffects:', 'CheckStatHerbsAfterIntimidate:')
    ordered(posthit, ['call nz, CheckSubstituteOpp', 'farcall RunHitAbilities', 'call CheckAirBalloon', 'HELD_ROCKY_HELMET'])
    assert 'cp MAGIC_GUARD' in posthit and 'SubtractHPFromUser_OverrideFaintOrder' in posthit
    endmove = block(commands, 'EndMoveDamageChecks:', 'BattleCommand_')
    ordered(endmove, ['call .EndMoveUserItems', 'call .EndMoveOpponentItems', '; Pickpocket'])
    deferred = block(commands, 'SetDeferredSwitch:', 'InvertDeferredSwitch:')
    assert 'jr nz, .done\n\tld [hl], a' in deferred, 'First reservation must win'
    switching = block(core, 'ForceDeferredSwitch:', 'PursuitSwitch:')
    ordered(switching, ['; Check if we can switch out', '; Do item consumption', '; Handle Pursuit', '; Regenerator'])
    drain = block(commands, 'SapHealth:', 'BattleCommand_burntarget:')
    assert 'SubtractHPFromUser_OverrideFaintOrder' in drain and 'MAGIC_GUARD' not in drain


def build():
    files = source.authenticated_sources()
    validate_source(files)
    move_files, moves, effects = attacks.inventory()
    used = sorted({m['source_effect'] for m in moves})
    command_names = sorted({line.split()[0] for name in used for line in effects[name]['commands']})
    assert len(moves) == 255 and len(used) == 133 and len(command_names) == 129
    relevant = []
    for name in used:
        commands = effects[name]['commands']
        names = [line.split()[0] for line in commands]
        if 'postfainteffects' in names:
            assert names.index('postfainteffects') < names.index('posthiteffects'), name
            if 'recoil' in names:
                assert names.index('recoil') < names.index('postfainteffects'), name
            relevant.append(name)
    runtime = (ROOT/'src/battle_move_resolution.c').read_text()
    constants = (ROOT/'include/constants/battle_move_resolution.h').read_text()
    ordered(constants, ['MOVEEND_POLISHED_POST_FAINT,', 'MOVEEND_POLISHED_CONTACT_ATTACKER,', 'MOVEEND_ABILITIES,', 'MOVEEND_ITEM_EFFECTS_TARGET,'])
    ordered(constants, ['MOVEEND_ITEM_ON_STAT_CHANGE,', 'MOVEEND_POLISHED_ACTION_CHECKPOINT,', 'MOVEEND_POLISHED_DEFERRED_SWITCH,', 'MOVEEND_SEND_OUT_REPLACEMENTS,'])
    assert '[MOVEEND_POLISHED_ACTION_CHECKPOINT] = MoveEndPolishedActionCheckpoint' in runtime
    paths = ['engine/battle/core.asm', 'engine/battle/effect_commands.asm', 'engine/battle/abilities.asm', 'data/moves/effects.asm']
    report = dict(source_commit=source.source.PIN, source_variant='default; FAITHFUL undefined', suite=2,
        complete_behavior_parity=False,
        source_anchors={p: files[p]['sha'] if p in files else source.git_hash('blob', move_files[p].encode()) for p in paths},
        inventory=dict(moves=255, effects=133, commands=command_names,
            postfaint_effect_families=relevant,
            limitation='Command sequences authenticate coverage obligations; they do not certify untested conditional branches.'),
        contracts=[
            'Recoil precedes postfainteffects; Destiny Bond replaces Moxie/Aftermath, otherwise Moxie precedes Aftermath.',
            'Source Aftermath and Liquid Ooze bypass Magic Guard; damage from Helmet and counter berries does not.',
            'Source post-hit items see an exhausted Substitute as broken; Air Balloon also requires a living holder.',
            'Faints wait for the action checkpoint; contact item/Aftermath/Ooze damage overrides first-faint order, ordinary recoil does not.',
            'U-turn reserves the single switch before end-move items; execute after item damage and faints. A Life Orb KO cancels the reservation without falling back to a switch item.',
            'Healing berries run after recoil/drain and Aftermath before the next contact effect can knock out the user.',
            'Destiny Bond persists through a cancelled repeat choice but clears before any different action; an executed repeat clears and fails.'],
        live_bindings=['src/battle_move_resolution.c', 'src/battle_end_turn.c', 'src/battle_util.c',
            'src/battle_hold_effects.c', 'src/battle_set_effect.c', 'src/battle_script_commands.c', 'data/battle_scripts_1.s'],
        regression_file='test/battle/polished_action_resolution.c',
        remaining=['All unverified effect branches and command interactions remain owned by Suite 2.',
            'Exact RNG stream and source presentation remain owned by Suite 3; native regression success is not source parity.'])
    return {MANIFEST: json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: authenticated action ordering; 255 moves, 133 effects and 129 commands retained')
