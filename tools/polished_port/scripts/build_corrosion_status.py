#!/usr/bin/env python3
"""Verify source Corrosion's status-ability bypass and immediate healing order."""
import argparse
import json

import build_pokemon_learnsets as source
import build_corrosion_substitute as substitute


def build():
    # The same authenticated two-move branch also bypasses ability prevention.
    substitute.build()
    files = source.sources()
    commands = files['engine/battle/effect_commands.asm']
    status = commands.split('\nCanStatusTarget:', 1)[1].split('\nBattleCommand_draintarget:', 1)[0]
    ability = status.split('\n.got_ability\n', 1)[1].split('\n.immunities_done\n', 1)[0]
    assert [line for line in source.default_lines(ability) if line] == [
        'cp VITAL_SPIRIT', 'ld e, INSOMNIA', 'jr z, .ability_replace',
        'cp PASTEL_VEIL', 'ld e, IMMUNITY', 'jr nz, .replace_done',
        '.ability_replace', 'ld a, e', '.replace_done', 'cp d',
        'jr z, .cant_ability', 'call DoLeafGuardCheck', 'jr z, .cant_ability']
    assert 'lb de, IMMUNITY, HELD_PREVENT_POISON' in commands.split('\nCanPoisonTarget:', 1)[1].split('\nCanBurnTarget:', 1)[0]
    leaf = commands.split('\nDoLeafGuardCheck:', 1)[1].split('\nPostStatusWithSynchronize:', 1)[0]
    assert [line for line in source.default_lines(leaf) if line] == [
        'cp LEAF_GUARD', 'ret nz', 'call GetWeatherAfterOpponentUmbrella', 'cp WEATHER_SUN', 'ret']
    post = commands.split('\nPostStatusWithSynchronize:', 1)[1].split('\nBattleCommand_sleep:', 1)[0]
    assert [line for line in source.default_lines(post) if line] == [
        'farcall RunEnemySynchronizeAbility', 'PostStatus:',
        'farcall UseOpponentHeldStatusHealingItem', 'farjp RunEnemyStatusHealAbilities']
    verbose = commands.split('\nStatusTargetVerbose:', 1)[1].split('\n.failed_ineffective\n', 1)[0]
    assert 'ld [hl], a\n\tcall DisplayStatusProblem\n\tcall UpdateOpponentInParty\n\tcall UpdateBattleHuds\n\tcall PostStatusWithSynchronize' in verbose
    abilities = files['engine/battle/abilities.asm']
    heal_table = abilities.split('\nStatusHealAbilities:', 1)[1].split('\nCloudNineAbility:', 1)[0]
    assert 'dbw IMMUNITY, ImmunityAbility' in heal_table
    assert 'dbw PASTEL_VEIL, PastelVeilAbility' in heal_table
    assert 'LEAF_GUARD' not in heal_table
    assert '\nImmunityAbility:\nPastelVeilAbility:\n\tld a, 1 << PSN\n\tjr HealStatusAbility' in abilities
    heal = abilities.split('\nHealStatusAbility:', 1)[1].split('\nOwnTempoAbility:', 1)[0]
    assert [line for line in source.default_lines(heal) if line] == [
        'ld b, a', 'ld a, BATTLE_VARS_STATUS', 'call GetBattleVar', 'and b', 'ret z',
        'call BeginAbility', 'call ShowAbilityActivation', 'ld a, BATTLE_VARS_STATUS',
        'call GetBattleVarAddr', 'xor a', 'ld [hl], a', 'ld hl, BecameHealthyText',
        'call StdBattleTextbox', 'call EndAbility', 'ldh a, [hBattleTurn]', 'and a',
        'jmp z, UpdateBattleMonInParty', 'jmp UpdateEnemyMonInParty']
    core = files['engine/battle/core.asm']
    item = core.split('\nUseOpponentHeldStatusHealingItem:', 1)[1].split('\nReconsumeConfusionHealingItem:', 1)[0]
    assert 'call StackCallOpponentTurn\nUseHeldStatusHealingItem:\n\tpredef GetUserItemAfterUnnerve\n\tcall _HeldStatusHealingItem\n\tret z\n\tjmp UseBattleItem' in item
    assert 'ld a, b\n\tcp HELD_HEAL_STATUS' in item
    assert 'ld a, c\n\tand [hl]\n\tjr z, .skip_status\n\txor a\n\tld [hl], a' in item
    attrs = files['data/items/attributes.asm']
    assert '; PECHA BERRY\n\titem_attribute 10, HELD_HEAL_STATUS, 1 << PSN, BERRIES, ITEMMENU_PARTY, ITEMMENU_PARTY' in attrs
    assert '; LUM BERRY\n\titem_attribute 10, HELD_HEAL_STATUS, -1, BERRIES, ITEMMENU_PARTY, ITEMMENU_PARTY' in attrs
    assert 'HELD_PREVENT_POISON' not in attrs
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Actual Corrosion using Poison-type Toxic or PoisonPowder in singles without active gimmicks.',
        moves=['POISONPOWDER', 'TOXIC'], prevention_bypassed=['IMMUNITY', 'PASTEL_VEIL', 'LEAF_GUARD'],
        rules=['The direct-user Corrosion branch skips the status ability checks as well as Substitute.',
               'Poison is applied and displayed before post-status healing. Immunity and Pastel Veil then cure it; Leaf Guard does not cure an existing status.',
               'The post-status path checks Synchronize, then held status healing, then status-healing abilities.',
               'Pecha and Lum Berries are consumed before Immunity or Pastel Veil can cure the poison.',
               'Protect, Safeguard, accuracy and existing major status gates remain; the source has no live item using HELD_PREVENT_POISON.'],
        adapter='Reuse the execution-only Corrosion status scope at RUN_SCRIPT ability checks. At the existing move-end immunity phase, offer poison-healing berries first for eligible poisoned Immunity/Pastel Veil targets; resume ordinary ability healing afterward. Shared AI/status estimates retain native behavior.',
        gaps=['Toxic Orb, broader damaging-poison and hazard behavior, general status/Synchronize rules, full RNG, AI decisions and presentation remain separate audits.',
              'Optional Magic Coat returns, doubles, active gimmicks, retyped moves and non-source ability/terrain barriers retain native paths. Broader optional combinations are not certified.'])
    return {source.ROOT/'tools/polished_port/manifests/corrosion_status.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned Corrosion status ability bypass, immediate cures and berry-before-ability order')
