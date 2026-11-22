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
    position = 0
    for token in tokens:
        found = text.find(token, position)
        assert found >= 0, f"Missing or reordered source token: {token}"
        position = found + len(token)


def block(text, start, end):
    return text.split(start, 1)[1].split(end, 1)[0]


def validate_source(files):
    commands = files['engine/battle/effect_commands.asm']['text']
    abilities = files['engine/battle/abilities.asm']['text']
    core = files['engine/battle/core.asm']['text']
    preaction = block(core, 'BattleTurn:', 'SafariBattleTurn:')
    ordered(preaction, ['call HandleBerserkGene', 'farcall CheckMirrorHerb',
        'call UpdateBattleMonInParty', 'farcall AIChooseMove', 'call BattleMenu', 'call DetermineMoveOrder'])
    gene = block(core, 'HandleBerserkGene:', 'CheckOpponentForfeit:')
    ordered(gene, ['call SetFastestTurn', 'call .do_it', 'call SwitchTurn',
        'predef GetUserItemAfterUnnerve', 'cp HELD_BERSERK_GENE',
        'ld b, $10 | ATTACK', 'farcall _ForceRaiseStat',
        'ld a, [wFailedMessage]', 'and a\n\tret nz', 'farcall UseStatItemText',
        'farcall ConsumeUserItem', 'call GetTrueUserAbility', 'cp OWN_TEMPO\n\tret z',
        'bit SUBSTATUS_CONFUSED, [hl]\n\tret nz', 'call BattleRandom', 'and 1\n\tadd 2'])
    assert not re.search(r'\b(?:SAFEGUARD|GUARD_SAFEGUARD)\b', gene), 'Gene bypasses Safeguard'
    sketch = files['engine/battle/move_effects/sketch.asm']['text']
    ordered(sketch, ['call ClearLastMove', 'ld a, [wLinkMode]', 'jr z, .not_linked',
        'call CheckSubstituteOpp', 'bit SUBSTATUS_TRANSFORMED',
        'ld a, BATTLE_VARS_LAST_COUNTER_MOVE_OPP', 'and a\n\tjr z, .fail',
        'inc a ; cp STRUGGLE', '.does_user_already_know_move', 'cp b\n\tjr z, .pop_hl_and_fail',
        'ld hl, Moves + MOVE_PP', 'farcall GetDisableEncoreMoves',
        'ld a, [wCurMoveNum]', 'ld [hl], b', 'ld [hl], c',
        'ld a, MON_MOVES', 'cp SKETCH\n\tjr nz, .finished_overwriting_sketch',
        'farcall SetDisableEncoreMoves'])
    ordered(sketch.split('cp SKETCH\n\tjr nz, .finished_overwriting_sketch', 1)[1],
        ['ld [hl], b', 'ld [hl], c', 'farcall SetDisableEncoreMoves'])
    assert 'INFILTRATOR' not in sketch, 'Sketch cannot bypass Substitute'
    locks = block(core, 'SetDisableEncoreMoves:', 'SwapBattleMoves:')
    assert 'Sets d to disabled move ID and e to encored/choiced move ID.' in core
    assert locks.count('farcall UserKnowsMove') == 2
    ordered(locks, ['.reset_disable\n', 'ld [bc], a', '.reset_encore\n', 'ld [hl], a'])
    used_text = files['engine/battle/used_move_text.asm']['text']
    ordered(used_text, ['dec a\n\tjr z, .charging', 'ld a, BATTLE_VARS_LAST_COUNTER_MOVE', 'ld [hl], a'])
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
    herbs = block(commands, 'CheckWhiteHerbEjectPack:', 'CheckMirrorHerb:')
    ordered(herbs, ['call SetFastestTurn', 'call HasUserFainted', '.stat_loop', 'cp BASE_STAT_LEVEL', 'jmp SetDeferredSwitch'])
    assert 'CheckAnyOtherAlive' not in herbs, 'Reserve Pack before testing replacement availability'
    stat_order = block(commands, 'CheckStatHerbs:', 'CheckThroatSpray:')
    ordered(stat_order, ['call CheckWhiteHerbEjectPack', 'call CheckMirrorHerb'])
    intimidate = block(abilities, 'IntimidateAbility:', 'DownloadAbility:')
    ordered(intimidate, ['NoIntimidateAbilities', '\n.intimidate_ok\n', 'ForceLowerOppStat', 'cp RATTLED', 'call EndAbility', 'farcall CheckMirrorHerb', 'farjp CheckStatHerbsAfterIntimidate'])
    entry_herbs = block(commands, 'CheckStatHerbsAfterIntimidate:', 'CheckEndMoveEffects:')
    ordered(entry_herbs, ['push af', 'ld [hl], 0', 'call CheckStatHerbs', 'call GetTrueUserAbility', 'call GetOpponentAbility', 'farcall DeferredSwitch', 'pop af', 'ld [hl], a'])
    assert entry_herbs.count('inc a\n\tjr z, .no_deferred_switch') == 2, 'Gas deactivation must prevent nested Pack switches'
    both_entry = block(core, 'RunBothEntryAbilities:', 'RunEntryAbilities:')
    ordered(both_entry.split('\n.no_double_gas\n', 1)[1], ['call SetFastestTurn', 'farcall RunEntryAbilitiesInner', 'call SwitchTurn', '\n.single_run\n'])
    assert files['data/abilities/no_intimidate_abilities.asm']['text'].splitlines()[1:] == [
        '\tdb INNER_FOCUS', '\tdb OWN_TEMPO', '\tdb OBLIVIOUS', '\tdb SCRAPPY', '\tdb -1']
    ordered(switching, ['; Do item consumption', 'cp SUCTION_CUPS', '; Figure out which side is switching'])
    # These ability blocks finish their own stat changes before copying, even
    # when invoked between hits or inside another stat command.
    for start, end in (('DownloadAbility:', 'ImposterAbility:'),
                       ('TanglingHairAbility:', 'EffectSporeAbility:'),
                       ('StatUpAbility:', 'WeakArmorAbility:'),
                       ('WeakArmorAbility:', 'FlashFireAbility:'),
                       ('MoodyAbility:', 'ApplyDamageAbilities_AfterTypeMatchup:'),
                       ('AngerPointAbility:', 'RunSwitchAbilities:')):
        ability_block = block(abilities, start, end)
        assert ability_block.rstrip().endswith('farjp CheckMirrorHerb'), start
        assert 'call EndAbility' in ability_block, start
    mirror = block(commands, 'CheckMirrorHerb:', 'EndMoveDamageChecks:')
    ordered(mirror, ['call HasUserFainted', 'call nz, HasOpponentFainted', 'ret z', 'call SetFastestTurn'])
    stats = files['engine/battle/stats.asm']['text']
    pending = block(stats, '.raise_loop\n', '.lower_loop\n')
    ordered(pending, ['cp MAX_STAT_LEVEL', 'inc [hl]', 'inc b', '\n.queue_mirror_herb\n', 'ld hl, wMirrorHerbPendingBoosts', 'add [hl]', 'jr nc, .no_overflow', 'or $f0'])
    assert 'GetUserItem' not in pending and 'GetOpponentItem' not in pending, 'Record actual increments independently of held items'
    ordered(mirror, ['and c', 'ld e, a', 'predef GetUserItemAfterUnnerve', 'call _RaiseStat', '\n.stat_raise_failed\n', 'ld [hl], e', 'call CheckAlreadyExecuted', 'call nz, ConsumeUserItem'])
    healing = block(files['engine/battle/endturn.asm']['text'], 'HandleHealingItems:', 'HandleStatusOrbs:')
    ordered(healing, ['call SetFastestTurn', 'farcall HandleStatBoostBerry', 'farcall CheckMirrorHerb', 'farjp UseConfusionHealingItem'])
    reset = block(core, 'BreakAttractionAndResetMirrorHerb:', 'SpikesDamageBoth:')
    ordered(reset, ['ResetMirrorHerb:', 'ld hl, wMirrorHerbPendingBoosts', 'ld bc, NUM_LEVEL_STATS - 1', 'rst ByteFill'])
    endturn = files['engine/battle/endturn.asm']['text']
    assert not re.search(r'HELD_(WHITE_HERB|EJECT_PACK)|CheckStatHerbs', endturn), 'No generic end-turn stat-herb pass'
    for name in ('baton_pass', 'roar', 'teleport'):
        text = files[f'engine/battle/move_effects/{name}.asm']['text']
        assert 'jmp SetDeferredSwitch' in text, name
    drain = block(commands, 'SapHealth:', 'BattleCommand_burntarget:')
    assert 'SubtractHPFromUser_OverrideFaintOrder' in drain and 'MAGIC_GUARD' not in drain


def validate_remaining_source(files):
    def read(path): return files[path]['text']
    transform = read('engine/battle/move_effects/transform.asm')
    ordered(transform, ['call ClearLastMove', 'bit SUBSTATUS_TRANSFORMED', 'cp MEWTWO', 'cp ARMOR_SUIT', 'cp INFILTRATOR', 'call CheckHiddenOpponent', 'farcall GetDisableEncoreMoves', 'call ResetActorDisable', 'cp SKETCH', 'farcall SetDisableEncoreMoves', 'cp IMPOSTER', 'farjp RunEntryAbilitiesInner'])
    swap = read('engine/battle/move_effects/skill_swap.asm')
    ordered(swap, ['call CheckHiddenOpponent', 'call AbilityCanBeSkillSwapped', 'res SUBSTATUS_UNBURDEN', 'res SUBSTATUS_FLASH_FIRE', 'ld [wPlayerCudChewBerry], a', 'ld [wEnemyCudChewBerry], a', 'farcall RunEntryAbilitiesInner_SkillSwap', 'call SwitchTurn', 'farcall RunEntryAbilitiesInner_SkillSwap'])
    assert 'NoSkillSwapEntryAbilities:\n\tdb IMPOSTER\n\tdb CLOUD_NINE\n\tdb -1' in swap
    conversion = read('engine/battle/move_effects/conversion.asm')
    ordered(conversion, ['.choose_move\n', 'ChangeIntoTypeText', 'push de', 'farcall MoveSelectionScreen', 'pop de', 'ld [wCurMoveNum], a', '.validate_choice\n', 'cp UNKNOWN_T', '.invalid_selection\n', 'InvalidTypeChangeText'])
    assert 'PP' not in conversion and 'DISABLE' not in conversion
    thief = read('engine/battle/move_effects/thief.asm')
    ordered(thief, ['call CheckSubstituteOpp', 'call CanStealItem', 'call ReceiveBattleItem', 'ret nc', 'SetBackupItem'])
    ordered(thief.split('ReceiveBattleItem:', 1)[1], ['jr nz, .holding_item', 'ld [hl], c', '.holding_item\n', 'call ReceiveItem'])
    ordered(thief.split('CheckStickyHold:', 1)[1], ['call HasOpponentFainted', 'ret z', 'call GetOpponentAbilityAfterMoldBreaker'])
    bite = read('engine/battle/move_effects/bug_bite.asm')
    ordered(bite, ['GetOpponentAbilityAfterMoldBreaker', 'cp UNNERVE', 'call CheckStickyHold', 'cp HELD_NO_BUG_BITE', 'cp BERRIES', 'call CheckPendingItemLoss', 'StealHPHealingItem', 'ConsumeStolenOpponentItem'])
    core = read('engine/battle/core.asm')
    ordered(block(core, 'HandleUserHealingItems:', '_SubtractHP:'), ['UseHeldStatusHealingItem', 'UseConfusionHealingItem', 'bit SUBSTATUS_PENDING_ITEMLOSS', 'HandleHPHealingItem', 'HandleStatBoostBerry'])
    boss = block(core, 'BoostGiovannisArmoredMewtwo:', 'LoadWeatherIconSprite:')
    ordered(boss, ['cp GIOVANNI', 'cp GIOVANNI1', 'ld b, ATTACK', 'ld b, DEFENSE', 'ld b, SPEED', 'ld b, SP_ATTACK', 'ld b, SP_DEFENSE', 'CheckMirrorHerb'])
    bell = read('engine/battle/move_effects/heal_bell.asm')
    assert 'SOUNDPROOF' not in bell and 'ld d, PARTY_LENGTH' in bell
    chew = block(read('engine/battle/abilities.asm'), 'CudChewAbility:', 'HarvestAbility:')
    assert 'BATTLE_VARS_CUD_CHEW_BERRY' in chew and 'GetUsedItemAddr' not in chew
    ordered(chew, ['jr nc, .eat_berry', '.eat_berry:', 'ld [hl], a', 'ReconsumeHPHealingItem'])


def validate_curse_trick_source(files):
    curse = files['engine/battle/move_effects/curse.asm']['text']
    ordered(curse, ['call CheckIfUserIsGhostType', 'jr z, .ghost',
        'ld b, SPEED', 'call ForceLowerStat', 'ld b, ATTACK', 'call ForceRaiseStat',
        'ld b, DEFENSE', 'jmp ForceRaiseStat', '\n.ghost\n',
        'call CheckHiddenOpponent', 'jr nz, .failed', 'call CheckSubstituteOpp',
        'jr nz, .failed', 'BATTLE_VARS_SUBSTATUS1_OPP', 'bit SUBSTATUS_CURSE, [hl]', 'jr nz, .failed',
        'set SUBSTATUS_CURSE, [hl]', 'call GetHalfMaxHP',
        'predef SubtractHPFromUser', 'call UpdateUserInParty'])
    commands = files['engine/battle/effect_commands.asm']['text']
    hidden = block(commands, 'CheckHiddenOpponent:', 'GetPlayerItem:')
    assert 'and 1 << SUBSTATUS_FLYING | 1 << SUBSTATUS_UNDERGROUND' in hidden
    substitute = block(commands, 'CheckSubstituteOpp:', 'CheckUserMove:')
    ordered(substitute, ['call GetTrueUserAbility', 'cp INFILTRATOR', 'ret z',
        'BATTLE_VARS_SUBSTATUS4_OPP', 'bit SUBSTATUS_SUBSTITUTE, a'])
    home = files['home/battle.asm']['text']
    ordered(home, ['GetQuarterMaxHP::', 'call GetHalfMaxHP', 'jr HalveBC',
        'GetHalfMaxHP::', 'call GetMaxHP', 'HalveBC::', 'srl b', 'rr c',
        'FloorBC::', 'ld a, c', 'or b', 'ret nz', 'inc c'])
    residual = block(files['engine/battle/endturn.asm']['text'], 'HandleCurse:', 'HandleWrap:')
    ordered(residual, ['call SetFastestTurn', 'bit SUBSTATUS_CURSE, [hl]',
        'call nz, PreventEndturnDamage', 'ret z', 'call GetQuarterMaxHP', 'predef SubtractHPFromUser'])
    trick = files['engine/battle/move_effects/trick.asm']['text']
    ordered(trick, ['call CheckSubstituteOpp', 'cp STICKY_HOLD',
        'call .UserCanBeTricked', 'call .OpponentCanBeTricked',
        'ld [hl], a', 'ld a, b', 'ld [de], a', 'call StdBattleTextbox'])
    assert 'UNBURDEN' not in trick and 'ConsumeUserItem' not in trick, 'Trick is an exchange, not consumption'
    abilities = files['engine/battle/abilities.asm']['text']
    unburden = block(abilities, 'UnburdenAbility:', 'QuickFeetAbility:')
    ordered(unburden, ['bit SUBSTATUS_UNBURDEN, a', 'ret z', 'ln a, 2, 1'])
    assert 'GetUserItem' not in unburden, 'An earned boost survives receiving an item'


def validate_item_unburden_source(files):
    for name in ('thief', 'knock_off'):
        text = files[f'engine/battle/move_effects/{name}.asm']['text']
        assert 'UNBURDEN' not in text, f'{name} preserves both Unburden flags'
        assert not re.search(r'\bConsume\w*Item\b', text), f'{name} does not consume an item'
    thief = files['engine/battle/move_effects/thief.asm']['text']
    ordered(thief, ['call ReceiveBattleItem', 'ret nc', 'xor a', 'ld [bc], a'])
    knock = files['engine/battle/move_effects/knock_off.asm']['text']
    ordered(knock, ['call CheckPendingItemLoss', 'ret z', 'call GetOpponentItem', 'xor a', 'ld [hl], a'])
    commands = files['engine/battle/effect_commands.asm']['text']
    pickpocket = block(commands, '; Pickpocket', '.EndMoveUserItems:')
    ordered(pickpocket, ['call HasOpponentFainted', 'call GetOpponentAbilityAfterMoldBreaker',
        'cp PICKPOCKET', 'call CheckContactMove', 'call SwitchTurn', 'call CanStealItem',
        'call BattleCommand_thief', 'jmp SwitchTurn'])
    stolen = block(commands, 'ConsumeStolenOpponentItem::', 'ConsumeUserItem::')
    ordered(stolen, ['call StackCallOpponentTurn', 'call GetConsumedItemVars', 'jr _ConsumeUserItem'])
    consume = block(commands, '_ConsumeUserItem::', 'GetConsumedItemVars::')
    ordered(consume, ['xor a', 'ld [de], a', 'ld [hl], a', '.apply_unburden',
        'call GetTrueUserAbility', 'cp UNBURDEN', 'ret nz', 'set SUBSTATUS_UNBURDEN, [hl]'])
    air_balloon = block(commands, '; Burst air balloons', '\n.air_balloon_done\n')
    ordered(air_balloon, ['call CheckAirBalloon', 'call ConsumeOpponentItem'])


def validate_recovery_source(files):
    abilities = files['engine/battle/abilities.asm']['text']
    harvest = block(abilities, 'HarvestAbility:', 'PickupAbility:')
    pickup = block(abilities, 'PickupAbility:', 'RegainItemByAbility:')
    regain = block(abilities, 'RegainItemByAbility:', 'GetCappedStats:')
    for name, text in (('Harvest', harvest), ('Pickup', pickup), ('Regain', regain)):
        assert 'UNBURDEN' not in text, f'{name} preserves the Unburden volatile'
        assert not re.search(r'\bConsume\w*Item\b', text), f'{name} does not immediately consume recovery'
    ordered(harvest, ['call GetWeatherAfterUserUmbrella', 'cp WEATHER_SUN', 'jr z, .ok',
        'call BattleRandom', 'add a', 'ret c', 'farcall GetUserItem', 'ret nz',
        'call GetUsedItemAddr', 'farcall CheckItemPocket', 'cp BERRIES', 'ret nz',
        'xor a', 'ld [hl], a', 'ld a, b', 'ld [de], a', 'call RegainItemByAbility'])
    ordered(pickup, ['farcall GetUserItem', 'ret nz', 'ldh a, [hBattleTurn]', 'jr z, .player',
        'ld a, [wBattleMode]', 'dec a', 'ret z', 'call GetOpponentUsedItemAddr',
        'ld a, [hl]', 'and a', 'ret z', 'ld [de], a', 'xor a', 'ld [hl], a',
        'call RegainItemByAbility'])
    assert 'call GetUsedItemAddr' not in pickup, 'Pickup never recovers its own history'
    history = block(files['home/battle.asm']['text'], 'GetOpponentUsedItemAddr::', 'BattleJumptable::')
    ordered(history, ['call StackCallOpponentTurn', 'GetUsedItemAddr::', 'ld hl, wPartyUsedItems',
        'ld a, [wCurBattleMon]', 'ld hl, wOTPartyUsedItems', 'ld a, [wCurOTMon]'])
    endturn = files['engine/battle/endturn.asm']['text']
    ordered(block(endturn, 'HandleBetweenTurnEffects:', '.endturn_loop'),
        ['call HandleEndturnBlockB', 'call HandleLeppaBerry', 'call HandleHealingItems'])
    ordered(re.split(r'^[A-Za-z_]\w*:', endturn.split('HandleEndturnBlockB:', 1)[1], maxsplit=1, flags=re.M)[0],
        ['call SetFastestTurn', 'call .do_it', 'call SwitchTurn', 'farcall EndturnAbilitiesB'])


def build():
    files = source.authenticated_sources()
    validate_source(files)
    validate_remaining_source(files)
    validate_curse_trick_source(files)
    validate_item_unburden_source(files)
    validate_recovery_source(files)
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
    ordered(constants, ['MOVEEND_LIFE_ORB_SHELL_BELL,', 'MOVEEND_POLISHED_SWITCH_ITEMS,', 'MOVEEND_PICKPOCKET,', 'MOVEEND_POLISHED_STAT_HERBS,', 'MOVEEND_SPRAY_LEPPA_BLUNDER,'])
    assert '[MOVEEND_POLISHED_ACTION_CHECKPOINT] = MoveEndPolishedActionCheckpoint' in runtime
    paths = ['engine/battle/move_effects/sketch.asm', 'engine/battle/used_move_text.asm', 'engine/battle/core.asm', 'engine/battle/effect_commands.asm', 'engine/battle/abilities.asm', 'engine/battle/stats.asm', 'engine/battle/endturn.asm', 'data/abilities/no_intimidate_abilities.asm', 'data/moves/effects.asm', 'engine/battle/move_effects/baton_pass.asm', 'engine/battle/move_effects/roar.asm', 'engine/battle/move_effects/teleport.asm', 'data/items/attributes.asm']
    paths += ['engine/battle/move_effects/'+name+'.asm' for name in ('transform', 'skill_swap', 'conversion', 'thief', 'knock_off', 'bug_bite', 'heal_bell', 'curse', 'trick')]
    paths += ['home/battle.asm']
    report = dict(source_commit=source.source.PIN, source_variant='default; FAITHFUL undefined', suite=2,
        complete_behavior_parity=False,
        source_anchors={p: files[p]['sha'] if p in files else source.git_hash('blob', move_files[p].encode()) for p in paths},
        inventory=dict(moves=255, effects=133, commands=command_names,
            postfaint_effect_families=relevant,
            limitation='Command sequences authenticate coverage obligations; they do not certify untested conditional branches.'),
        contracts=[
            'Source Pickup recovers only the active opponent party slot history, including earlier turns and switch returns, clears that history once, refuses an occupied recipient and cannot be used by wild opponents. Harvest and Pickup preserve earned Unburden flags after ability changes. Recovery precedes the final healing pass; optional Recycle, doubles and active gimmicks retain native behavior. Post-battle backup restoration and exact RNG consumption remain open.',
            'Source Thief, Pickpocket and Knock Off leave Unburden state unchanged; receiving through Thief preserves an earned boost. Bug Bite consumes the victim berry and activates the victim ability, not the eater. Ordinary consumption and Air Balloon activate effective Unburden; Neutralizing Gas prevents earning the boost and temporarily suppresses an existing boost. Optional Covet, Sticky Barb, Magician, doubles and active gimmicks retain native transfer behavior.',
            'Ghost Curse selects the opponent after type changes, rejects Fly/Dig even with No Guard, and respects Substitute unless the effective user ability is Infiltrator. Rejection spends PP but no HP.',
            'Curse costs floor(max HP / 2), at least one, may faint its user, and does not charge again for an already cursed target. Source residual checks use floor(max HP / 4), at least one, subject to Magic Guard.',
            'Non-Ghost Curse lowers Speed before raising Attack and Defense with ordinary cap/Contrary/Simple rules. Ordinary switching clears Curse; Baton Pass transfers it.',
            'Source Trick does not activate Unburden through an exchange and does not erase a consumption-earned boost on receiving an item. Optional Switcheroo, doubles and active gimmicks retain native behavior. Other item-transfer and Choice-lock branches remain open.',
            'Transform copies raw ability and activates its entry routine after the message except Imposter; repeated transformation is allowed, hidden/transformed/Armored Mewtwo targets and non-Infiltrator Substitute are rejected.',
            'Transform preserves party identity, HP/item/status, remaps surviving move locks and zeroes the Disable countdown; copied PP is five except Sketch (one) and empty slots (zero).',
            'Skill Swap rejects hidden targets and source blacklist abilities, resets Unburden/Flash Fire/Cud Chew, and runs user entry before target entry while excluding Imposter and Cloud Nine.',
            'Cud Chew stores the consumed berry independently of Recycle history, delays one end-turn check, and clears its pending berry on Skill Swap; a successful Bug Bite effect can arm it.',
            'Conversion lets the player choose any known valid move type regardless of PP, retries invalid selections, and preserves the selected action slot; wild selection samples eligible slots. Smart AI/link transport remain Suite 3 obligations.',
            'Equal-priority voluntary switches resolve by current Speed/Trick Room and Quick Claw/Custap/Lagging Tail. Entries are sequential, not a shared simultaneous-entry pass.',
            'Wild player Thief/Pickpocket holds the stolen item when empty or sends it to the bag when already holding and space exists. Wild opponents never steal; living Sticky Hold blocks theft, fainted Sticky Hold does not.',
            'Knock Off and Bug Bite mark pending item loss before damage, suppressing HP/stat berry triggers but retaining status/confusion cures. Bug Bite respects Unnerve after Mold Breaker and the six EV berries.',
            'Heal Bell heals active and benched party status without a Soundproof exclusion.',
            'Giovanni1 has one initial Armored Mewtwo stat-boost checkpoint before entry abilities and followed by Mirror Herb; appended trainer/item IDs preserve existing identifiers.',
            'Sketch clears user history before any failure, uses the opponent counter-move record, and rejects link mode, Substitute (including Infiltrator), transformed users, absent/Struggle history and any already-known move.',
            'Sketch writes base PP and clears only the replaced slot PP-Up bits. Only a party slot actually containing Sketch is changed permanently; other temporary battle slots and party PP remain separate.',
            'Sketch remaps Disable/Encore/Choice by move identity, retaining duration when still known and clearing locks whose move was overwritten. Called-move text cannot repopulate cleared source history.',
            'Recoil precedes postfainteffects; Destiny Bond replaces Moxie/Aftermath, otherwise Moxie precedes Aftermath.',
            'Source Aftermath and Liquid Ooze bypass Magic Guard; damage from Helmet and counter berries does not.',
            'Source post-hit items see an exhausted Substitute as broken; Air Balloon also requires a living holder.',
            'Faints wait for the action checkpoint; contact item/Aftermath/Ooze damage overrides first-faint order, ordinary recoil does not.',
            'U-turn reserves the single switch before end-move items; execute after item damage and faints. A Life Orb KO cancels the reservation without falling back to a switch item.',
            'Baton Pass, trainer Teleport and trainer Roar reserve before end-move effects and execute after the faint/outcome checkpoint.',
            'Button/Card reserve after Life Orb and before Pickpocket/stat herbs; an ineligible first reservation still excludes a later Pack.',
            'Eject Pack checks current negative stages on living holders in current Speed/Trick Room order; White Herb/Pack checks finish before Mirror Herb, then Throat Spray.',
            'Opponent-item switches consume the item before checking the original move user for Suction Cups; own Pack has no opponent-item check.',
            'Intimidate checks Mirror Herb before White Herb/Pack and a second Mirror Herb pass; its private switch reservation preserves the surrounding action.',
            'Entry Pack switches finish immediately, including replacement hazards and abilities; the interrupted two-entry loop still visits its remaining side.',
            'Gas deactivation runs the Intimidate herb checks but suppresses nested Pack execution. Inner Focus/Own Tempo/Oblivious/Scrappy return before those checks.',
            'Source Download, Tangling Hair, StatUpAbility, Weak Armor, Moody and Anger Point check Mirror Herb immediately; nested stat commands resume after the copy.',
            'Mirror Herb requires both battlers alive; switching clears pending copies on both sides before replacement abilities.',
            'Mirror pending boosts record actual positive increments independent of held items or recipient caps, saturate at 15, and discard copied/failed/itemless changes at each checkpoint.',
            'Mirror Herb checks current effective items and caps, consumes only after a successful stat change, and cannot trigger an opposing Mirror Herb. Native Opportunist keeps its own queue.',
            'Each source healing-item sweep checks both battlers for Mirror Herb after its stat berry; skipped move-end checks preserve pending boosts until a later source checkpoint.',
            'Berserk Gene checks both sides in current Speed/Trick Room order before AI and player action selection, followed by a separate Mirror Herb pass. Mid-turn acquisitions and entrants wait until the next turn.',
            'Berserk Gene consumes only after a successful two-stage Attack change (including Contrary/caps); then it checks effective Own Tempo and existing confusion. Safeguard is bypassed; new confusion counters are uniformly 2 or 3.',
            'Source entry and end turn have no general White Herb/Pack sweep: inherited or Moody negative stages wait for a move or Intimidate checkpoint.',
            'Healing berries run after recoil/drain and Aftermath before the next contact effect can knock out the user.',
            'Destiny Bond persists through a cancelled repeat choice but clears before any different action; an executed repeat clears and fails.'],
        live_bindings=['src/battle_move_resolution.c', 'src/battle_end_turn.c', 'src/battle_util.c',
            'src/battle_hold_effects.c', 'src/battle_stat_change.c', 'src/battle_main.c', 'src/battle_set_effect.c', 'src/battle_script_commands.c', 'src/battle_switch_in.c', 'data/battle_scripts_1.s'],
        regression_file='test/battle/polished_action_resolution.c',
        additional_regression_files=['test/battle/polished_deferred_switch.c', 'test/battle/polished_entry_items.c', 'test/battle/polished_stat_items.c', 'test/battle/polished_mirror_ledger.c', 'test/battle/polished_berserk_gene.c', 'test/battle/move_effect/polished_sketch.c', 'test/battle/move_effect/polished_suite2_copy.c', 'test/battle/move_effect/polished_suite2_conversion.c', 'test/battle/move_effect/polished_suite2_items.c', 'test/battle/polished_suite2_boss.c', 'test/battle/polished_suite2_switching.c'],
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
