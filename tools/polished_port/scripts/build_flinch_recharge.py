#!/usr/bin/env python3
"""Pin source move-effect flinches clearing recharge before the action check."""
import argparse
import json

import build_pokemon_learnsets as source
import build_flinch_hit as flinch_hit
import build_flinch_focus as flinch_focus


def lines(text):
    return [line for line in source.default_lines(text) if line]


def build():
    # Authenticate all thirteen move mappings, scripts and flinchtarget gates,
    # and the independent action-time true-ability check before CantMove.
    flinch_hit.build()
    flinch_focus.build()
    files = source.sources()
    commands = files['engine/battle/effect_commands.asm']
    assert lines(commands.split('\nFlinchTarget:\n', 1)[1].split('\nHasOpponentDamagedUs:', 1)[0]) == [
        'ld a, BATTLE_VARS_SUBSTATUS4_OPP', 'call GetBattleVarAddr',
        'set SUBSTATUS_FLINCHED, [hl]', 'jmp EndRechargeOpp']
    assert lines(commands.split('\nEndRechargeOpp:\n', 1)[1].split('\nBattleCommand_resetstats:', 1)[0]) == [
        'push hl', 'ld a, BATTLE_VARS_SUBSTATUS3_OPP', 'call GetBattleVarAddr',
        'res SUBSTATUS_RECHARGE, [hl]', 'pop hl', 'ret']
    assert lines(commands.split('\nBattleCommand_rechargenextturn:\n', 1)[1].split('\nEndRechargeOpp:', 1)[0]) == [
        'ld a, BATTLE_VARS_SUBSTATUS3', 'call GetBattleVarAddr',
        'set SUBSTATUS_RECHARGE, [hl]', 'ret']
    chance = commands.split('\nBattleCommand_effectchance:', 1)[1].split('\nBattleCommand_lowersub:', 1)[0]
    assert lines(chance) == [
        'push bc', 'push hl', 'xor a', 'ld [wEffectFailed], a',
        'call HasOpponentFainted', 'jr z, EffectChanceFailed',
        'call CheckSubHit', 'jr nz, EffectChanceFailed',
        'call GetOpponentAbilityAfterMoldBreaker', 'cp SHIELD_DUST',
        'jr z, EffectChanceFailed', 'call GetOpponentItemAfterUnnerve',
        'ld a, b', 'cp HELD_COVERT_CLOAK', 'jr z, EffectChanceFailed',
        'jr _CheckEffectChance', 'BattleCommand_selfeffectchance:',
        'push bc', 'push hl', 'xor a', 'ld [wEffectFailed], a',
        '_CheckEffectChance:', 'ld hl, wPlayerMoveStruct + MOVE_CHANCE',
        'ldh a, [hBattleTurn]', 'and a', 'jr z, .got_move_chance',
        'ld hl, wEnemyMoveStruct + MOVE_CHANCE', '.got_move_chance',
        'ld a, [hl]', 'ld b, a', 'call GetTrueUserAbility', 'cp SHEER_FORCE',
        'jr z, EffectChanceFailed', 'cp SERENE_GRACE', 'jr nz, .skip_serene_grace',
        'sla b', 'jr c, EffectChanceEnd', '.skip_serene_grace',
        'ld a, 100', 'call BattleRandomRange', 'cp b', 'jr c, EffectChanceEnd',
        'EffectChanceFailed:', 'ld a, 1', 'ld [wEffectFailed], a', 'and a',
        'EffectChanceEnd:', 'pop hl', 'pop bc', 'ret']
    rows = [line[5:].split(',') for line in lines(files['data/moves/moves.asm']) if line.startswith('move ')]
    recharge = [row[0].strip() for row in rows if row[1].strip() == 'EFFECT_HYPER_BEAM']
    assert recharge == ['HYPER_BEAM', 'GIGA_IMPACT']
    effects = [line.split()[1] for line in lines(files['constants/move_effect_constants.asm']) if line.startswith('const EFFECT_')]
    pointers = [line.split()[1] for line in lines(files['data/moves/effects_pointers.asm']) if line.startswith('dw ')]
    assert dict(zip(effects, pointers, strict=True))['EFFECT_HYPER_BEAM'] == 'HyperBeam'
    assert lines(files['data/moves/effects.asm'].split('\nHyperBeam:\n', 1)[1].split('\nRage:', 1)[0]) == [
        'checkobedience', 'usedmovetext', 'doturn', 'hastarget', 'checkhit',
        'checkpriority', 'critical', 'damagestats', 'damagecalc', 'stab',
        'damagevariation', 'moveanim', 'failuretext', 'applydamage',
        'criticaltext', 'supereffectivetext', 'rechargenextturn',
        'postfainteffects', 'posthiteffects', 'endmove']
    core = files['engine/battle/core.asm']
    assert lines(core.split('\nCheckLockedIn:\n', 1)[1].split('\nParsePlayerAction:', 1)[0]) == [
        'ld a, BATTLE_VARS_SUBSTATUS3', 'call GetBattleVar',
        'and 1 << SUBSTATUS_RECHARGE | 1 << SUBSTATUS_CHARGED | 1 << SUBSTATUS_RAMPAGE | 1 << SUBSTATUS_ROLLOUT', 'ret']
    assert 'call CheckLockedIn\n\tjr nz, .skip_ai_move\n\tfarcall AIChooseMove' in core
    assert 'call CheckLockedIn\n\tjr nz, .skip_iteration\n.loop1\n\tcall BattleMenu' in core
    player = core.split('\nParsePlayerAction:', 1)[1].split('\nTryEnemyFlee:', 1)[0]
    assert 'call CheckLockedIn\n\tjr nz, .locked_in' in player
    assert lines(player.split('\n.locked_in\n', 1)[1].split('\n.reset_rage', 1)[0]) == [
        'xor a', 'ld [wPlayerProtectCount], a', 'ld hl, wPlayerSubStatus4',
        'res SUBSTATUS_RAGE, [hl]', '.continue_protect', 'call ParseEnemyAction', 'xor a', 'ret']
    enemy = core.split('\nParseEnemyAction:', 1)[1].split('\n.no_linkswitch', 1)[0]
    assert 'call SetEnemyTurn\n\tcall CheckLockedIn\n\tret nz' in enemy
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Pending recharge cleared by the thirteen mapped source FlinchHit/Stomp moves, in ordinary singles without either active gimmick.',
        source_recharge_moves=recharge,
        rules=[
            'After its Substitute, sleep/freeze, acted-turn and effect-failure gates, flinchtarget sets the flinch flag and calls EndRechargeOpp.',
            'EndRechargeOpp clears recharge before CheckTurn. Neither this helper nor effectchance checks Inner Focus; active Inner Focus is checked later by the acting battler.',
            'Unprotected targets flinch instead of displaying recharge, including Steadfast activation. Active Inner Focus rejects flinch and allows the previously selected move to proceed through remaining action checks.',
            'Both sides skip move selection while recharge is pending at turn start. Clearing recharge during the other action does not request a new choice; a repeated Hyper Beam or Giga Impact consumes PP and sets recharge again.',
            'Failed secondary effects, a hit on Substitute (even when it breaks), sleep/freeze and targets that already acted must not clear recharge through flinchtarget.'],
        adapter='At mapped move-effect execution, preserve the existing sleep/freeze gate, then clear pending recharge and set flinch for a target that has not acted. Defer Inner Focus to the authenticated action-time recheck. Reuse the engine selected-move slot, PP and recharge handlers.',
        gaps=[
            'Direct item/Stench FlinchTarget calls remain a separate recharge-clearing audit, including their different eligibility and after-action behavior.',
            'This does not certify general recharge application, fainted targets, all flinch eligibility, probabilities/RNG stream, complete cancellation order, AI or original presentation.',
            'Optional recharge producers inherit this pending-state adapter; their own effects are not certified. Optional flinch producers, native-profile battles, doubles and either active gimmick retain native behavior.'])
    return {source.ROOT/'tools/polished_port/manifests/flinch_recharge.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned move-effect flinches clear recharge before the active Inner Focus check')
