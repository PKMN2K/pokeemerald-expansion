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
    # Interpret the called helper as well as the command body: Sketch has no
    # local INFILTRATOR token, but CheckSubstituteOpp explicitly permits it.
    substitute = block(commands, 'CheckSubstituteOpp:', 'CheckUserMove:')
    ordered(substitute, ['call GetTrueUserAbility', 'cp INFILTRATOR', 'ret z',
        'call GetFutureSightUser', 'ld a, [wInAbility]', 'SoundMoves',
        'bit SUBSTATUS_SUBSTITUTE, a'])
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


def validate_persistence_choice_source(files):
    home = files['home/battle.asm']['text']
    backup = block(home, 'SetBackupItem::', 'BackupBattleItems::')
    ordered(backup, ['ldh a, [hBattleTurn]', 'and a\n\tret nz', 'call GetBackupItemAddr',
        'ld a, [hl]', 'and a\n\tret nz', 'ld [hl], b'])
    restore = block(home, 'RestoreBattleItems::', 'GetOpponentUsedItemAddr::')
    ordered(restore, ['ld c, 1', 'ld b, 7', 'ld hl, wPartyMon1Item', 'ld de, wPartyBackupItems',
        '.restore\n', 'ld a, [de]', 'ld [hl], a'])
    core = files['engine/battle/core.asm']['text']
    ordered(core, ['DoBattle:', 'call BackupBattleItems'])
    post = core.split('PostBattleTasks:', 1)[1]
    ordered(post, ['call RestoreBattleItems'])
    consume = block(files['engine/battle/effect_commands.asm']['text'], '_ConsumeUserItem::', '\n.apply_unburden\n')
    ordered(consume, ['cp BERRIES', 'jr nz, .apply_unburden', 'call GetBackupItemAddr',
        'ld a, [hl]', 'cp d', 'jr nz, .apply_unburden', 'xor a', 'ld [hl], a'])
    trick = files['engine/battle/move_effects/trick.asm']['text']
    ordered(trick, ['ld a, [wBattleMode]', 'dec a\n\tret nz', 'call GetBackupItemAddr',
        'ld a, [wBattleMonItem]', 'ld [hl], a'])
    harvest = block(files['engine/battle/abilities.asm']['text'], 'HarvestAbility:', 'PickupAbility:')
    assert harvest.rstrip().endswith('jmp SetBackupItem')
    pickup = block(files['engine/battle/abilities.asm']['text'], 'PickupAbility:', 'RegainItemByAbility:')
    ordered(pickup, ['call RegainItemByAbility', 'ld a, [wBattleMode]', 'dec a', 'ret nz', 'jmp SetBackupItem'])
    caught = files['engine/items/item_effects.asm']['text']
    ordered(caught, ['ld hl, wPartyBackupItems', 'ld a, [wEnemyMonItem]', 'ld [hl], a'])
    choice = block(core, 'SetChoiceLock:', 'GetDisableEncoreMoves:')
    ordered(choice, ['ld a, [hl]', 'and $f', 'ld b, a', 'ld a, c', 'inc a', 'swap a', 'or b', 'ld [hl], a'])
    assert not re.search(r'HELD_|GetUserItem|GetTrueUserAbility', choice)
    assert core.count('call SetChoiceLock') == 3
    encore = files['engine/battle/move_effects/encore_disable.asm']['text']
    ordered(encore, ['BATTLE_VARS_LAST_COUNTER_MOVE_OPP', 'cp STRUGGLE', 'cp ENCORE',
        'call UserKnowsMove', 'and $3f', 'jr z, .failed', 'inc c', 'swap c', 'or c', 'ld [de], a', 'jmp CheckOpponentMentalHerb'])
    cure = files['engine/battle/move_effects/attract.asm']['text'].split('CheckMentalHerb:', 1)[1]
    ordered(cure, ['ld hl, wPlayerEncoreCount', 'and $f', 'and $f0', 'ld [hl], a'])
    expiry = block(files['engine/battle/endturn.asm']['text'], 'HandleEncore:', 'HandleDisable:')
    assert 'and $f0' in expiry


def validate_obedience_entry_source(files):
    commands = files['engine/battle/effect_commands.asm']['text']
    check = block(commands, 'CheckTurn:', '\nEndTurn:')
    ordered(check, ['bit SUBSTATUS_RECHARGE', 'bit SUBSTATUS_FLINCHED', 'and SLP_MASK',
        'bit FRZ', 'bit SUBSTATUS_CONFUSED', 'bit SUBSTATUS_ATTRACT', '.not_infatuated',
        'call VerifyChosenMove', '.not_disabled', 'bit PAR'])
    cant = block(commands, 'CantMove:', 'VerifyChosenMove:')
    ordered(cant, ['res SUBSTATUS_DESTINY_BOND', 'call CheckRampageStatusAndGetRolloutCount',
        'dec a', 'call z, HandleRampage_ConfuseUser'])
    obey = block(commands, 'BattleCommand_checkobedience:', 'BattleCommand_usedmovetext:')
    ordered(obey, ['ldh a, [hBattleTurn]', 'call CheckUserIsCharging', 'SUBSTATUS_RAMPAGE',
        'ld a, [wLinkMode]', 'ld a, [wInBattleTowerBattle]', 'ld a, [wInitialOptions]',
        'bit TRADED_AS_OT_OPT, a', 'ret nz',
        'ld a, MON_ID', 'ld a, [wPlayerID]', 'ld a, [wPlayerID + 1]',
        'bit RISINGBADGE', 'MAX_LEVEL + 1', 'bit MINERALBADGE', 'ld a, 70',
        'bit FOGBADGE', 'ld a, 50', 'bit HIVEBADGE', 'ld a, 30', 'bit ZEPHYRBADGE', 'ld a, 20', 'ld a, 10',
        'cp d\n\tret nc', '.rand1', 'cp b', 'cp c\n\tret c', 'call IgnoreSleepOnly',
        '.rand2', 'cp c', 'jr c, .UseInstead', 'sub c', 'sub b', 'jr c, .Nap', 'cp b', 'jr nc, .DoNothing'])
    nap = block(obey, '.Nap:', '.DoNothing:')
    ordered(nap, ['call BattleRandom', 'add a', 'swap a', 'and SLP_MASK', 'jr z, .Nap', 'ld [wBattleMonStatus], a'])
    assert 'Ability' not in nap and 'Safeguard' not in nap
    alternate = block(obey, '.UseInstead:', '.EndDisobedience:')
    ordered(alternate, ['ld a, [wBattleMonMoves + 1]', 'ld a, [wPlayerDisableCount]',
        'jr nz, .DoNothing', 'and $3f', 'cp b', 'jr z, .DoNothing', '.RandomMove',
        'and %11', 'cp c', 'jr z, .RandomMove', 'and $3f', 'jr z, .RandomMove', 'call DoMove'])
    assert 'CheckUsableMove' not in alternate
    ordered(obey.split('.EndDisobedience:', 1)[1], ['ld [wLastPlayerMove], a',
        'ld [wLastPlayerCounterMove], a', 'ld [wPlayerEncoreCount], a'])
    abilities = files['engine/battle/abilities.asm']['text']
    forewarn = block(abilities, 'ForewarnAbility:', 'FriskAbility:')
    ordered(forewarn, ['ld c, 120', 'cp EFFECT_COUNTER', 'jr z, .compare_power',
        'ld hl, Moves + MOVE_POWER', 'call GetMoveProperty', 'and a', 'jr z, .loop',
        '.randomize', 'call BattleRandomRange', '.done', 'and a\n\tret z'])
    anticipation = block(abilities, 'AnticipationAbility:', 'ForewarnAbility:')
    ordered(anticipation, ['call GetFixedMoveStruct', 'BATTLE_VARS_MOVE_CATEGORY',
        'cp STATUS', 'jr z, .end_of_loop', 'farcall BattleCheckTypeMatchup'])
    items = files['engine/items/item_effects.asm']['text']
    xitem = block(items, 'XItemEffect:', 'XItemHappiness:')
    ordered(xitem, ['call CheckItemParam', 'farcall _ForceRaiseStat', 'and a',
        'jmp nz, WontHaveAnyEffect_NotUsedMessage', 'farcall ResetMirrorHerb'])
    attributes = files['data/items/attributes.asm']['text']
    for stat in ('ATTACK', 'DEFENSE', 'SPEED', 'SP_ATTACK', 'SP_DEFENSE', 'ACCURACY'):
        assert f'0, $10 | {stat}, ITEM, ITEMMENU_NOUSE, ITEMMENU_CLOSE' in attributes


def validate_action_boundaries(files):
    commands = files['engine/battle/effect_commands.asm']['text']
    rampage = block(commands, 'BattleCommand_rampage:', 'CheckRampageStatusAndGetRolloutCount:')
    ordered(rampage, ['and SLP_MASK', 'ret nz', 'and %00000001', 'inc a',
        '.already_rampaging', 'dec a', 'HandleRampage:', 'call HasUserFainted',
        'ret z', 'call GetFutureSightUser', 'ret nc', 'HandleRampage_ConfuseUser:',
        'bit SUBSTATUS_CONFUSED', 'call GetTrueUserAbility', 'cp OWN_TEMPO',
        'and %11', 'inc a', 'inc a', 'HandleRampage_CheckMiss:', 'res SUBSTATUS_RAMPAGE'])
    cant = block(commands, 'CantMove:', 'VerifyChosenMove:')
    ordered(cant, ['res SUBSTATUS_DESTINY_BOND', 'call CheckRampageStatusAndGetRolloutCount',
        'dec a', 'call z, HandleRampage_ConfuseUser', 'and [hl]'])
    explosion = files['engine/battle/move_effects/explosion.asm']['text']
    ordered(explosion, ['call GetOpponentAbilityAfterMoldBreaker', 'cp DAMP', 'ret z',
        'wWhichMonFaintedFirst', 'BATTLE_VARS_STATUS', 'call UpdateUserInParty',
        'cp UNNERVE', 'ld [hl], NO_ABILITY', 'farcall HandleLeppaBerry',
        'farcall HandleHealingItems', 'res SUBSTATUS_LEECH_SEED', 'res SUBSTATUS_DESTINY_BOND'])
    rest = block(commands, 'BattleCommand_heal:', 'BattleSideCopy:')
    ordered(rest, ['farcall CheckFullHP', 'and SLP_MASK', 'cp INSOMNIA',
        'cp VITAL_SPIRIT', 'call IsOpponentLeafGuardActive',
        'ld [hl], REST_SLEEP_TURNS + 1', 'call GetMaxHP', 'farcall RestoreHP',
        'call UpdateUserInParty', 'call SwitchTurn', 'call PostStatus'])
    leaf = block(commands, 'IsOpponentLeafGuardActive:', 'PostStatusWithSynchronize:')
    ordered(leaf, ['call GetTrueUserAbility', 'cp LEAF_GUARD',
        'call GetWeatherAfterOpponentUmbrella', 'cp WEATHER_SUN'])
    counter = files['engine/battle/move_effects/counter.asm']['text']
    ordered(counter, ['call BattleCommand_resettypematchup', 'ret z',
        'call HasOpponentDamagedUs', 'ret z', 'ld hl, wCurDamage', 'ret z',
        'bit SUBSTATUS_IN_ABILITY', 'jr nz, .got_damage', 'add a', 'adc a',
        'jr nc, .got_damage', 'ld a, $ff'])
    herb = block(commands, 'CheckPowerHerb:', '\nResetTurn:')
    ordered(herb, ['call HasUserFainted', 'ret z', 'bit SUBSTATUS_CHARGED',
        'ret z', 'cp EFFECT_SOLAR_BEAM', 'call GetWeatherAfterUserUmbrella',
        'cp WEATHER_SUN', 'jr z, .chargeup', 'cp HELD_POWER_HERB',
        'call ConsumeUserItem', '.chargeup'])
    evolve = files['engine/battle/evolve_battle.asm']['text']
    ordered(evolve, ['.load_mon_data', 'inc hl ; skip Item', 'rst CopyBytes ; copy Moves'])
    learn = files['engine/pokemon/learn.asm']['text']
    ordered(learn, ['call ForgetMove', 'wPlayerDisableCount', 'swap a', 'xor c',
        'ld [wPlayerDisableCount], a', 'wPlayerEncoreCount', 'swap a', 'xor c',
        'ld [wPlayerEncoreCount], a'])


def validate_move_state_source(files):
    commands = files['engine/battle/effect_commands.asm']['text']
    core = files['engine/battle/core.asm']['text']
    streak = block(commands, 'IncreaseMetronomeCount:', 'CheckPowerHerb:')
    ordered(streak, ['call CheckUserIsCharging', 'ret nz', 'call GetFutureSightUser',
        'ret nc', 'wPlayerSelectedMove', 'wPlayerMetronomeCount',
        'wEnemySelectedMove', 'wEnemyMetronomeCount', 'cp b', 'jr nz, .reset',
        'cp 5', 'ret nc', 'inc [hl]', 'inc a ; cp STRUGGLE',
        'jr z, .done_update_selected_move', 'ld [de], a', 'ld [hl], a'])
    assert 'GetUserItem' not in streak, 'Source streak history does not require holding Metronome'
    pp = block(commands, 'BattleConsumePP:', 'BattleCommand_')
    ordered(pp, ['call CheckUserIsCharging', 'ret nz', 'inc a ; cp STRUGGLE',
        'SUBSTATUS_IN_LOOP', 'SUBSTATUS_RAMPAGE', 'SUBSTATUS_CHARGED',
        'SUBSTATUS_ROLLOUT', 'dec [hl]'])
    reset = block(commands, 'ResetTurn:', 'MoveDisabled:')
    ordered(reset, ['ld a, 1', 'wPlayerCharging', 'wEnemyCharging',
        'ld [hl], a', 'call DoMove', 'jmp EndMoveEffect'])
    alternate = block(commands, '.UseInstead:\n', '.EndDisobedience:\n')
    ordered(alternate, ['call UpdateMoveData', 'call DoMove', 'ld [wCurMoveNum], a'])
    selection = block(core, 'MoveSelectionScreen:', 'MoveInfoBox:')
    ordered(selection, ['wBattleMonMoves', 'wPlayerSelectedMove',
        '.sanity_check_done\n', 'xor a', 'ld [wPlayerSelectedMove], a'])
    ordered(block(core, '.continue_rage\n', '.reset_bide\n'),
        ['cp EFFECT_PROTECT', 'cp EFFECT_ENDURE', 'ld [wPlayerProtectCount], a'])
    ordered(block(core, '.no_rage\n', '.struggle\n'),
        ['cp EFFECT_PROTECT', 'cp EFFECT_ENDURE', 'ld [wEnemyProtectCount], a'])
    cant = block(commands, 'CantMove:', 'VerifyChosenMove:')
    assert 'ProtectCount' not in cant, 'Source action interruption retains the Protect chain'
    protect = files['engine/battle/move_effects/protect.asm']['text']
    ordered(protect, ['ProtectChance:', 'call CheckOpponentWentFirst',
        'jr nz, .failed', 'cp 7', 'jr z, .no_overflow', 'ld a, 3',
        'call BattleRandomRange', '.failed\n', 'ld [de], a'])
    roost = files['engine/battle/move_effects/roost.asm']['text']
    ordered(roost, ['farcall CheckFullHP', 'ret z', 'call CheckIfUserIsFlyingType',
        'lb bc, FLYING, UNKNOWN_T', 'ld [hl], c', '.normalize\n', 'ld a, NORMAL',
        'set SUBSTATUS_ROOST'])
    transform = files['engine/battle/move_effects/transform.asm']['text']
    ordered(transform, ['ld bc, wBattleMonStats - wBattleMonPP',
        'ld bc, wBattleMonStructEnd - wBattleMonStats', 'rst CopyBytes'])
    bounce = files['engine/battle/move_effects/magic_bounce.asm']['text']
    ordered(bounce, ['call SwitchTurn', 'set SUBSTATUS_IN_ABILITY',
        'call ResetTurn', 'res SUBSTATUS_IN_ABILITY', 'call UpdateMoveData', 'jmp SwitchTurn'])
    assert 'Charging' not in bounce, 'Bounce restores the move, not source ResetTurn state'
    send = block(core, '.reset_used_moves_done\n', '.volatile_done\n\t; Switch active mon')
    ordered(send, ['bit SWITCH_BATON_PASS', 'jr nz, .volatile_done', 'call NewEnemyMonStatus'])
    enemy_send = block(core, 'Function_SetEnemyPkmnAndSendOutAnimation:', 'BattleAnimateFrontpic:')
    assert 'wEnemySelectedMove' not in enemy_send
    player_send = block(core, 'SendOutPlayerMon:', 'NewBattleMonStatus:')
    assert 'ld [wPlayerSelectedMove], a' in player_send


def validate_switch_boundaries_source(files):
    core = files['engine/battle/core.asm']['text']
    forced = block(core, 'ForceDeferredSwitch:', 'GetBothSwitchTarget:')
    ordered(forced, ['call HasUserFainted', 'farcall CheckAnyOtherAliveMons',
        'call HasOpponentFainted', 'farcall CheckAnyOtherAliveOpponentMons',
        '; Do item consumption', 'cp SUCTION_CUPS', '; Handle Pursuit',
        'call nz, PursuitSwitch', 'call HasUserFainted', 'farcall RunSwitchAbilities',
        'call GetUserSwitchTarget', 'call SendInUserPkmn',
        'cp 1 << SWITCH_DEFERRED | 1 << SWITCH_TARGET | 1 << SWITCH_FORCED',
        'jr nz, .regular_spikes', 'call SpikesDamage_CheckMoldBreaker',
        '.regular_spikes\n', 'call SpikesDamage', 'call RunEntryAbilities'])
    assert 'wBattleMode' not in forced, 'Red Card uses reserve selection in wild battles too'
    faint = block(core, 'FaintUserPokemon:', 'SuppressUserAbilities:')
    ordered(faint, ['call BreakAttractionAndResetMirrorHerb', 'ld [wPlayerWrapCount], a',
        'ld [wEnemyWrapCount], a', 'BATTLE_VARS_STATUS', 'set SUBSTATUS_FAINTED'])
    assert 'SUBSTATUS_CANT_RUN' not in faint, 'Mean Look is released at replacement, not at faint'
    fresh = block(core, 'NewBattleMonStatus:', 'BreakAttractionAndResetMirrorHerb:')
    ordered(fresh, ['ld hl, wEnemySubStatus2', 'res SUBSTATUS_CANT_RUN'])
    send = block(core, 'SendInUserPkmn:', '\n\t; substatus3')
    ordered(send, ['; substatus2', 'xor a', 'ld [hli], a'])


def validate_ability_survival_source(files):
    abilities = files['engine/battle/abilities.asm']['text']
    commands = files['engine/battle/effect_commands.asm']['text']
    healing = block(abilities, 'HydrationAbility:', 'NaturalCureAbility:')
    ordered(healing, ['call GetWeatherAfterUserUmbrella', 'cp WEATHER_RAIN',
        'ret nz', 'jr HealAllStatusAbility', 'ShedSkinAbility:',
        'call BattleRandom', 'cp 1 + (30 percent)', 'ret nc'])
    assert 'DEF percent EQUS "* $ff / 100"' in files['macros/data.asm']['text']
    weather = block(abilities, 'IceBodyAbility:', 'EndturnAbilitiesA:')
    ordered(weather, ['WEATHER_HAIL', 'WEATHER_RAIN', 'call GetWeatherAfterUserUmbrella',
        'farcall CheckFullHP', 'cp DRY_SKIN', 'call GetSixteenthMaxHP',
        'call GetEighthMaxHP', 'farcall RestoreHP'])
    assert not re.search(r'SUBSTATUS_(?:FLYING|UNDERGROUND)', weather)
    stat = block(abilities, 'SelectRandomStat:', 'MoodyAbility:')
    ordered(stat, ['ld a, 5', 'call BattleRandomRange'])
    nullify = block(abilities, 'CheckNullificationAbilities:', 'RunEnemyNullificationAbilities:')
    ordered(nullify, ['call GetOpponentAbilityAfterMoldBreaker', 'cp DAMP',
        'cp SOUNDPROOF', 'cp BULLETPROOF', 'cp FLASH_FIRE',
        'cp LIGHTNING_ROD', 'jr z, .check_others', 'BATTLE_VARS_MOVE_CATEGORY',
        'cp STATUS', 'ret z', '.check_others\n'])
    ordered(nullify, ['.check_forcedmiss\n', 'ld a, [wAttackMissed]',
        'dec a ; cp ATKFAIL_MISSED', 'ret z', 'dec a ; cp ATKFAIL_PROTECT', 'ret z'])
    matchup = block(commands, 'CheckTypeMatchup:', '_CheckTypeMatchup:')
    ordered(matchup, ['call _CheckTypeMatchup', 'ld a, [wTypeMatchup]', 'and a',
        'jr z, .end', 'farcall CheckNullificationAbilities'])
    breakable = files['data/abilities/mold_breaker_suppressed_abilities.asm']['text']
    assert '\tdb STURDY\n' in breakable and '\tdb SOUNDPROOF\n' in breakable
    assert 'BULLETPROOF' not in breakable
    weak = block(abilities, 'WeakArmorAbility:', 'FlashFireAbility:')
    ordered(weak, ['and a ; cp PHYSICAL', 'ret nz', 'ld b, DEFENSE',
        'farcall LowerStat', 'ld b, $10 | SPEED', 'farcall RaiseStat'])
    survival = block(commands, 'BattleCommand_applydamage:', 'GetFailureResultText:')
    ordered(survival, ['bit SUBSTATUS_ENDURE', '.cont\n', 'call GetOpponentItem',
        'cp HELD_FOCUS_BAND', 'jr z, .focus_band', 'cp HELD_FOCUS_SASH',
        'jr z, .sturdy', 'call GetOpponentAbilityAfterMoldBreaker', 'cp STURDY',
        '.sturdy\n', 'farcall CheckOpponentFullHP', 'jr nz, .no_endure',
        '.focus_band\n', 'call BattleRandom', 'cp c', 'jr nc, .no_endure',
        '.enduring\n', 'call BattleCommand_falseswipe'])
    attributes = files['data/items/attributes.asm']['text']
    assert 'item_attribute 4000, HELD_FOCUS_BAND, 30, ITEM, ITEMMENU_NOUSE, ITEMMENU_NOUSE' in attributes


def validate_status_pp_cleanup_source(files):
    commands = files['engine/battle/effect_commands.asm']['text']
    core = files['engine/battle/core.asm']['text']
    status = block(commands, 'CanParalyzeTarget:', 'CanSleepTarget:')
    ordered(status, ['lb bc, ELECTRIC, ELECTRIC', 'lb de, LIMBER, HELD_PREVENT_PARALYZE'])
    status = block(commands, 'CanStatusTarget:', 'BattleCommand_draintarget:')
    ordered(status, ['and h', 'jmp nz, .already_statused', '.not_corrosive\n',
        'call CheckIfTargetIsSomeType', 'call GetOpponentItemAfterUnnerve',
        'call SafeCheckSafeguard', 'call CheckSubstituteOpp',
        'call GetOpponentAbilityAfterMoldBreaker', 'call DoLeafGuardCheck'])
    safeguard = files['engine/battle/move_effects/safeguard.asm']['text']
    ordered(safeguard, ['SafeCheckSafeguard:', 'call GetTrueUserAbility', 'cp INFILTRATOR',
        'BattleCommand_checksafeguard:', 'call SafeCheckSafeguard', 'ret z', 'jmp EndMoveEffect'])
    prepare = block(core, 'PreparePPRestore:', 'GetZeroPPMove:')
    ordered(prepare, ['ld hl, wBattleMonMoves', 'ld hl, wBattleMonPP',
        'ld hl, wEnemyMonMoves', 'ld hl, wEnemyMonPP', 'ld de, wTempMonPP',
        'ld de, wTempMonMoves'])
    nonfull = block(core, 'GetNonfullPPMove:', 'ReconsumeLeppaBerry:')
    ordered(nonfull, ['ld bc, 0', 'ld hl, wTempMonMoves', 'and a',
        'jr z, .all_moves_full', 'ld e, [hl]', 'farcall GetMaxPPOfMove', 'cp e',
        'jr nz, .got_nonfull_pp', 'inc bc'])
    assert 'and $3f' not in nonfull and 'and PP_MASK' not in nonfull, 'Forced Leppa compares the unmasked PP byte'
    zero = block(core, 'GetZeroPPMove:', 'GetNonfullPPMove:')
    ordered(zero, ['ld a, [hli]', 'and $3f', 'jr z, .got_zero_pp'])
    leppa = block(core, 'ReconsumeLeppaBerry:', 'DealDamageToOpponent:')
    ordered(leppa, ['call GetNonfullPPMove', 'StealLeppaBerry:',
        'call GetNonfullPPMove', 'farcall ConsumeStolenOpponentItem', 'LeppaRestorePP:',
        'add 10', 'farcall GetMaxPPOfMove', 'ld [hl], d',
        'bit SUBSTATUS_TRANSFORMED, a', 'ret nz', 'ld hl, wPartyMon1PP', 'ld [hl], d'])
    restore = block(core, 'LeppaRestorePP:', '\n.got_pp_to_restore\n')
    ordered(restore, ['ld a, [hl]', 'add 10', 'ld d, a', 'farcall GetMaxPPOfMove',
        'cp d', 'jr nc, .got_pp_to_restore', 'ld d, a'])
    assert 'and $3f' not in restore and 'and PP_MASK' not in restore, 'Leppa adds to packed PP and clears PP Ups when writing the clamped byte'
    balls = files['engine/items/item_effects.asm']['text']
    capture = block(balls, 'PokeBallEffect:', '.SendToPC:')
    ordered(capture, ['ld a, [wPartyCount]', 'cp PARTY_LENGTH', 'jr z, .check_room',
        'ld hl, wPartyBackupItems', 'ld a, [wEnemyMonItem]', 'ld [hl], a',
        'ld a, [wPartyCount]', 'cp PARTY_LENGTH', 'jmp z, .SendToPC'])
    postbattle = block(core, 'PostBattleTasks::', 'RunBothEntryAbilities:')
    ordered(postbattle, ['call RestoreBattleItems', 'farcall UpdatePkmnStats', 'res TOX, [hl]'])
    exit_battle = core.split('ExitBattle:', 1)[1]
    ordered(exit_battle, ['call PostBattleTasks', 'call .HandleEndOfBattle'])
    whiteout = files['engine/events/whiteout.asm']['text']
    ordered(whiteout, ['Script_Whiteout:', 'special HealParty', 'special WarpToSpawnPoint'])
    payday = files['engine/battle/move_effects/pay_day.asm']['text']
    ordered(payday, ['call CheckSubstituteOpp', 'ret nz', 'ld a, [wBattleMonLevel]',
        'ld a, [wEnemyMonLevel]', 'ld b, 5', 'ld hl, wPayDayMoney + 2',
        'add [hl]', 'inc [hl]', 'dec hl', 'inc [hl]', 'dec b', 'jr nz, .loop'])
    assert 'MAX_MONEY' not in payday and 'cp $ff' not in payday, 'Pay Day wraps its three-byte accumulator'
    rollout = files['engine/battle/move_effects/rollout.asm']['text']
    ordered(rollout, ['BATTLE_VARS_STATUS', 'and SLP_MASK', 'ret nz',
        'call CheckRampageStatusAndGetRolloutCount', 'bit SUBSTATUS_CURLED'])
    abilities = files['engine/battle/abilities.asm']['text']
    absorb = block(abilities, 'DrySkinAbility:', 'AftermathAbility:')
    ordered(absorb, ['call ShowAbilityActivation', 'farcall CheckFullHP',
        'jr z, .full_hp', 'call GetQuarterMaxHP', '.full_hp\n', 'ld hl, HPIsFullText'])


def validate_closure_followup_source(files, effects):
    commands = files['engine/battle/effect_commands.asm']['text']
    abilities = files['engine/battle/abilities.asm']['text']
    core = files['engine/battle/core.asm']['text']
    ordered('\n'.join(effects['EFFECT_MINIMIZE']['commands']),
        ['forceraisestat $10 | EVASION', 'minimize', 'endmove'])
    minimize = files['engine/battle/move_effects/minimize.asm']['text']
    assert 'set SUBSTATUS_MINIMIZED, [hl]' in minimize
    focus = files['engine/battle/move_effects/focus_energy.asm']['text']
    ordered(focus, ['bit SUBSTATUS_FOCUS_ENERGY', 'jr nz, .already_pumped', 'set SUBSTATUS_FOCUS_ENERGY'])
    focus_critical = block(commands, '\n.FocusEnergy:\n', '\n.CheckCritical:\n')
    ordered(focus_critical, ['bit SUBSTATUS_FOCUS_ENERGY', 'inc c', 'inc c'])
    for effect, command in [('EFFECT_BUG_BITE', 'bugbite'), ('EFFECT_KNOCK_OFF', 'knockoff')]:
        ordered('\n'.join(effects[effect]['commands']),
            ['failuretext', command + ' FALSE', 'applydamage', 'postfainteffects',
             'posthiteffects', command + ' TRUE', 'endmove'])
    bug_bite = files['engine/battle/move_effects/bug_bite.asm']['text']
    ordered(bug_bite, ['cp UNNERVE', 'CheckStickyHold', 'HELD_NO_BUG_BITE', 'BERRIES', 'CheckPendingItemLoss'])
    for token in ['CheckSubstituteOpp', 'CheckSubHit', 'wEffectFailed']:
        assert token not in bug_bite, 'Bug Bite has no Substitute or secondary failure gate'
    knock_off = files['engine/battle/move_effects/knock_off.asm']['text']
    ordered(knock_off, ['wAttackMissed', 'CheckSubstituteOpp', 'CheckStickyHold', 'OpponentCanLoseItem'])
    thief = files['engine/battle/move_effects/thief.asm']['text']
    ordered(thief, ['call CheckSubstituteOpp', 'call CanStealItem'])
    ordered('\n'.join(effects['EFFECT_THIEF']['commands']), ['posthiteffects', 'thief'])
    critical_text = block(commands, 'BattleCommand_criticaltext:', 'BattleCommand_startloop:')
    ordered(critical_text, ['call CheckCrit', 'call ResetCrit', 'call HasOpponentFainted',
        'cp ANGER_POINT', 'farcall AngerPointAbility'])
    assert 'CheckSubstituteOpp' not in critical_text and 'CheckSubHit' not in critical_text
    broken_sub = block(commands, 'SelfInflictDamageToSubstitute:', 'UpdateMoveData:')
    ordered(broken_sub, ['\n.broke\n', 'res SUBSTATUS_SUBSTITUTE, [hl]'])
    berserk = block(abilities, 'ResolveOpponentBerserk:', 'RunFaintAbilities:')
    ordered(berserk, ['bit SUBSTATUS_IN_ABILITY', 'ret z', 'res SUBSTATUS_IN_ABILITY',
        'cp BERSERK', 'farcall CheckSheerForceNegation', 'ret z', 'ld b, SP_ATTACK', 'call StatUpAbility'])
    berserk_damage = block(core, '\n.berserk\n', 'SubtractHPFromUser_OverrideFaintOrder:')
    ordered(berserk_damage, ['set SUBSTATUS_IN_ABILITY', 'call SubtractHPFromUser_SkipItems',
        'call CheckEnigmaBerry', 'call CompareHP', 'res SUBSTATUS_IN_ABILITY',
        'farcall ResolveOpponentBerserk_CheckMultihit', 'call HandleUserHealingItems'])
    cud = block(abilities, 'CudChewAbility:', 'HarvestAbility:')
    ordered(cud, ['.eat_berry:\n', 'ld [wCurItem], a', 'xor a', 'ld [hl], a', 'call BeginAbility',
        'farcall ReconsumeConfusionHealingItem', 'farcall ReconsumeHeldStatusHealingItem',
        'farcall ReconsumeHPHealingItem', 'farcall ReconsumeStatBoostBerry',
        'farcall ReconsumeDefendHitBerry', 'farcall ReconsumeLeppaBerry', 'jmp EndAbility'])
    assert 'ShowAbilityActivation' not in cud
    assert 'ShowAbilityActivation' not in block(abilities, 'BeginAbility:', 'EndAbility:')
    hp_retry = block(core, 'ReconsumeHPHealingItem:', 'ReconsumeBattleItem:')
    ordered(hp_retry, ['call CheckFullHP', 'ret z', '_HeldHPHealingItem'])
    status_retry = block(core, '_HeldStatusHealingItem:', 'ReconsumeConfusionHealingItem:')
    ordered(status_retry, ['\n.skip_status\n', 'ld a, b', 'and a', 'ret z', 'farcall ShowPotentialAbilityActivation'])
    contact = files['engine/battle/misc.asm']['text']
    ordered(contact, ['_CheckContactMove::', 'cp HELD_PROTECTIVE_PADS', 'jr z, .protective_pads',
        'cp HELD_PUNCHING_GLOVE', '\n.protective_pads\n', 'ccf', 'ret'])
    claws = block(abilities, 'ToughClawsAbility:', 'MegaLauncherAbility:')
    ordered(claws, ['call CheckContactMove', 'ret c', 'ln a, 13, 10'])
    pinch = block(abilities, 'PinchAbility:', 'RivalryAbility:')
    ordered(pinch, ['call CheckPinch', 'jr z, TypeDependentAbility', 'SteelySpiritAbility:',
        'ld b, STEEL', 'TypeDependentAbility:', 'cp b', 'ret nz', 'ln a, 3, 2'])
    rivalry = block(abilities, 'RivalryAbility:', 'SheerForceAbility:')
    ordered(rivalry, ['farcall CheckOppositeGender', 'ret c', 'ln a, 5, 4', 'ln a, 3, 4'])
    guts = block(abilities, 'GutsAbility:', 'PixilateAbility:')
    ordered(guts, ['farcall GetFutureSightUser', 'ld a, MON_STATUS', 'call TrueUserPartyAttr',
        'ld a, BATTLE_VARS_STATUS', 'and a', 'ret z', 'ln a, 3, 2', 'jmp ApplyPhysicalAttackDamageMod'])
    analytic = block(abilities, 'AnalyticAbility:', 'TintedLensAbility:')
    ordered(analytic, ['farcall GetFutureSightUser', 'ret nc', 'ld a, [wEnemyGoesFirst]',
        'ldh a, [hBattleTurn]', 'xor b', 'ret z', 'ln a, 13, 10'])


def validate_identity_cancellation_source(files):
    commands = files['engine/battle/effect_commands.asm']['text']
    core = files['engine/battle/core.asm']['text']
    attract = files['engine/battle/move_effects/attract.asm']['text']
    ordered(attract, ['call CheckOppositeGender', 'jr c, .failed', 'jr z, .failed',
        'call CheckHiddenOpponent', 'bit SUBSTATUS_IN_LOVE', 'jr nz, .failed',
        'call GetOpponentAbilityAfterMoldBreaker', 'cp OBLIVIOUS'])
    gender = attract.split('CheckOppositeGender:', 1)[1]
    ordered(gender, ['ld a, MON_SPECIES', 'call TrueUserPartyAttr', 'ld a, MON_GENDER',
        'call TrueUserPartyAttr', 'farcall GetGender', 'ret c',
        'ld a, MON_SPECIES', 'call OpponentPartyAttr', 'ld a, MON_GENDER',
        'call OpponentPartyAttr', 'farcall GetGender', 'ret c', 'xor b'])
    foresight = files['engine/battle/move_effects/foresight.asm']['text']
    ordered(foresight, ['call CheckHiddenOpponent', 'bit SUBSTATUS_IDENTIFIED, [hl]',
        'jr nz, .failed', 'set SUBSTATUS_IDENTIFIED, [hl]'])
    essential = block(commands, 'UserCanLoseItem:', 'CheckAirBalloon:')
    ordered(essential, ['call ItemIsMail', 'call .CompareUserSpecies', 'call SwitchTurn',
        'call .CompareUserSpecies', '.CompareUserSpecies:', 'ld a, MON_SPECIES',
        'call UserPartyAttr', 'ld a, MON_FORM', 'call UserPartyAttr',
        'jmp CompareSpeciesForm', '.EssentialItemTable:',
        'species_battle_item ARMOR_SUIT, MEWTWO, MEWTWO_ARMORED_FORM'])
    form = block(files['home/pokemon.asm']['text'], 'CompareSpeciesForm:', 'GetCosmeticSpeciesAndFormIndex::')
    ordered(form, ['xor b', 'jr z, .done', 'cp (FORM_MASK + 1)',
        'jr nc, .done', 'xor b', 'and FORM_MASK'])
    assert 'ext_const MEWTWO_ARMORED_FORM ; 15d (2)' in files['constants/pokemon_constants.asm']['text']
    turn = block(commands, 'DoTurn:', 'DoMove:')
    ordered(turn, ['call CheckTurn', 'ld a, [wMoveState]', 'bit 7, a', 'ret nz'])
    confused = block(commands, '\n.confused\n', '\n.not_confused\n')
    ordered(confused, ['call HitConfusion', 'call CantMove', 'jr EndTurn'])
    self_hit = block(commands, 'HitConfusion:', 'BattleCommand_checkobedience:')
    ordered(self_hit, ['call HitSelfInConfusion', 'call ConfusedDamageCalc', 'call TakeOpponentDamage'])
    self_damage = block(commands, 'TakeOpponentDamage:', 'TakeDamage:')
    assert 'farcall SubtractHPFromUser_SkipItems' in self_damage
    skip_items = block(core, 'SubtractHPFromUser_SkipItems:', 'SubtractHPFromUser:')
    assert 'ld de, _SubtractHP' in skip_items
    assert 'HandleUserHealingItems' not in skip_items
    after_action = block(core, 'PerformMove:', 'ResolveFaints:')
    assert 'HandleHealingItems' not in after_action and 'HandleUserHealingItems' not in after_action
    offer = block(core, 'OfferSwitch:', 'Function_SetEnemyPkmnAndSendOutAnimation:')
    ordered(offer, ['call YesNoBox', 'call PickSwitchMonInBattle', 'jr c, OfferSwitch',
        'ld a, 1 << SWITCH_EXPLICIT', 'ld [wDeferredSwitch], a', 'jmp ForceDeferredSwitch'])
    replacement = files['engine/battle/endturn.asm']['text']
    ordered(replacement, ['farcall CheckAnyOtherAlivePartyMons', '1 << BATTLE_SWITCH | 1 << BATTLE_PREDICT',
        'farcall OfferSwitch', '.player_set_mode\n', 'call SetFastestTurn',
        'farcall SendInUserPkmn', 'call SwitchTurn', 'farcall SendInUserPkmn',
        'farcall SpikesDamageBoth', 'farcall RunBothEntryAbilities'])
    assert 'CANT_RUN' not in offer, 'The optional offer ignores ordinary escape traps'
    category = block(files['home/battle.asm']['text'], 'GetFixedCategory::', 'ApplyPhysicalDefenseDamageMod::')
    ordered(category, ['ld bc, MOVE_CATEGORY', 'cp STATUS', 'ret z', 'bit PSS_OPT, a',
        'ret nz', 'ld bc, MOVE_TYPE', 'call GetFarByte', 'cp SPECIAL_TYPES',
        'ld a, PHYSICAL', 'ret c', 'inc a ; SPECIAL'])


def validate_medicine_crash_source(files):
    commands = files['engine/battle/effect_commands.asm']['text']
    crash = block(commands, 'GetFailureResultText:', 'FailText_CheckOpponentProtect:')
    ordered(crash, ['call FailText_CheckOpponentProtect', 'call ResetCrit',
        'cp EFFECT_JUMP_KICK', 'ret nz', 'call GetHalfMaxHP',
        'ld hl, CrashedText', 'jmp TakeOpponentDamage'])
    assert 'MAGIC_GUARD' not in crash and 'GetTrueUserAbility' not in crash
    items = files['engine/items/item_effects.asm']['text']
    cure = block(items, 'UseStatusHealer:', 'BattlemonRestoreHealth:')
    ordered(cure, ['call IsMonFainted', 'jr z, .no_good', 'call GetItemHealingAction',
        'and c', 'jr nz, .good', 'call IsItemUsedOnBattleMon',
        'bit SUBSTATUS_CONFUSED, a', 'inc a ; cp $ff', 'jr nz, .no_good',
        'call HealStatus', 'call UseDisposableItem'])
    heal = block(items, '\nHealStatus:', 'GetItemHealingAction:')
    ordered(heal, ['call IsItemUsedOnBattleMon', 'ret nc', 'ld [wBattleMonStatus], a',
        'call GetItemHealingAction', 'cp -1', 'ret nz', 'res SUBSTATUS_CONFUSED, [hl]'])
    assert 'IN_LOVE' not in heal, 'All-status medicine does not cure source attraction'
    persim = block(items, 'PersimBerry:', 'EnergyPowder:')
    ordered(persim, ['bit SUBSTATUS_CONFUSED, [hl]', 'jmp z, WontHaveAnyEffectMessage',
        'res SUBSTATUS_CONFUSED, [hl]', 'call UseItemText'])
    disposable = block(items, 'UseItemText:', 'UseBallInTrainerBattle:')
    ordered(disposable, ['call WaitPressAorB_BlinkCursor', '; fallthrough',
        'UseDisposableItem:', 'ld a, 1', 'jmp TossItem'])
    assert not re.search(r'^\s*ret\s*$', disposable, flags=re.M)
    restore = block(items, 'BattleRestorePP:', 'Not_PP_Up:')
    ordered(restore, ['bit SUBSTATUS_TRANSFORMED, a', 'call z, .UpdateBattleMonPP',
        '.UpdateBattleMonPP:', 'ld de, wBattleMonMoves', 'cp [hl]',
        'jr nz, .next', 'ld [de], a'])
    pp = block(items, '\nRestorePP:', 'RaiseThePPOfWhichMoveText:')
    ordered(pp, ['call GetMaxPPOfMove', 'and PP_MASK', 'cp b', 'jr nc, .dont_restore',
        'cp MAX_ELIXIR', 'cp MAX_ETHER', 'add 10', '.restore_all',
        'and 3 << 6', 'or b', 'ld [hl], a'])


def validate_room_screen_source(files, effects):
    room = files['engine/battle/move_effects/trick_room.asm']['text']
    ordered(room, ['ld hl, wTrickRoom', 'ld [hl], 5', 'jr z, .new_trick_room',
        'ld [hl], 0', '\n.new_trick_room\n', 'call HandleRoomService',
        'HandleRoomService:', 'call SetFastestTurn', 'call .do_it', 'call SwitchTurn',
        '\n.do_it\n', 'GetUserItemAfterUnnerve', 'cp HELD_ROOM_SERVICE', 'ret nz',
        'RoomServiceItem:', 'ld a, STAT_SKIPTEXT', 'call _ForceLowerStat',
        'ld a, [wFailedMessage]', 'ret nz', 'UseStatItemText', 'jmp ConsumeUserItem'])
    ordered('\n'.join(effects['EFFECT_BRICK_BREAK']['commands']), ['moveanim',
        'failuretext', 'applydamage', 'criticaltext', 'brickbreak',
        'supereffectivetext', 'postfainteffects', 'posthiteffects', 'endmove'])
    breaker = files['engine/battle/move_effects/brick_break.asm']['text']
    ordered(breaker, ['ld hl, wEnemyScreens', 'ld hl, wPlayerScreens',
        'ld a, [hl]', 'and a', 'ret z', 'ld [hl], 0', 'and SCREENS_REFLECT',
        'ld hl, BrokeReflectText', 'and SCREENS_LIGHT_SCREEN', 'ld hl, BrokeLightScreenText'])
    assert 'HasOpponentFainted' not in breaker
    screens = block(files['engine/battle/effect_commands.asm']['text'],
        'GetOpponentActiveScreens:', 'TruncateHL_BC:')
    ordered(screens, ['xor EFFECT_BRICK_BREAK', 'jr nz, .no_brick_break',
        'call .GetScreen', 'ld [wBattleAnimParam], a', 'xor a', 'ret',
        '\n.no_brick_break\n', 'xor INFILTRATOR'])


def validate_affection_source(files):
    commands = files['engine/battle/effect_commands.asm']['text']
    home = files['home/battle.asm']['text']
    endturn = files['engine/battle/endturn.asm']['text']
    level = block(commands, 'CheckAffection:', 'OpponentAffectionText:')
    ordered(level, ['ld a, [wInitialOptions]', 'bit AFFECTION_OPT, a',
        'jr z, .no_affection', 'ld a, [wLinkMode]', 'and a',
        'jr nz, .no_affection', 'ld a, [wInBattleTowerBattle]',
        'and a', 'jr z, .cont', '.no_affection\n\txor a\n\tret',
        'ld b, 3', 'ld a, MON_HAPPINESS', 'call TrueUserPartyAttr',
        'ld hl, .AffectionThresholds', 'cp [hl]', 'jr nc, .done',
        'inc hl', 'dec b', '.AffectionThresholds:\n\tdb 255\n\tdb 220\n\tdb 180\n\tdb 0'])
    party = block(home, 'TrueUserPartyAttr::', 'UserPartyAttr::')
    ordered(party, ['ldh a, [hBattleTurn]', 'ld hl, wPartyMons',
        'jr z, .got_partymons', 'ld hl, wOTPartyMons',
        'farcall GetFutureSightUser', 'call GetPartyLocation', 'ld a, [hl]'])
    critical = block(commands, 'BattleCommand_critical:', 'SetCrit:')
    ordered(critical, ['ld a, c', 'cp 3', 'jr nc, .guranteed_crit',
        'ld hl, CriticalHitChances', 'ld b, [hl]', 'call CheckAffection',
        'cp 3', 'jr c, .no_affection_boost', 'sla b',
        'ld a, 24', 'call BattleRandomRange', 'cp b', 'ret nc'])
    chances = files['data/battle/critical_hit_chances.asm']['text']
    assert [line.split(';')[0].strip() for line in chances.splitlines()[1:]] == [
        'db 1', 'db 3', 'db 12', 'db 24']
    accuracy = block(commands, 'BattleCommand_checkhit:', '\n.got_acc_eva\n')
    ordered(accuracy, ['bit SUBSTATUS_IN_ABILITY, a', 'ret nz',
        'call .PursuitCheck', 'ret z', 'call .PoisonTypeUsingToxic', 'ret z',
        'call .NoGuardCheck', 'ret z', 'call .FlyDigMoves',
        'call .LockOn', 'ret nz', 'call .WeatherAccCheck', 'ret z',
        'call .AntiMinimize', 'ret z', 'cp EFFECT_ALWAYS_HIT', 'ret z',
        'cp EFFECT_ROAR', 'ret z', 'cp EFFECT_COUNTER', 'ret z',
        'inc a ; cp STRUGGLE', 'ret z', 'ld a, [wTypeModifier]',
        'and a', 'ret z', 'call CheckOpponentAffection', 'cp 3',
        'jr c, .no_affection_evasion', 'ld a, 10', 'call BattleRandomRange',
        'and a', 'ld a, ATKFAIL_AFFECTION', 'jmp z, .Miss_skipset',
        '.no_affection_evasion\n', 'ld a, [wPlayerAccLevel]'])
    miss = block(commands, 'GetFailureResultText:', 'BattleCommand_criticaltext:')
    ordered(miss, ['PlayerAffectionEvasionText', 'EnemyAffectionEvasionText',
        'call OpponentAffectionText', 'jr .cont_atkmiss',
        '.cont_atkmiss\n', 'predef GetUserItemAfterUnnerve', 'cp HELD_BLUNDER_POLICY'])
    survival = block(commands, 'BattleCommand_applydamage:', '\n.damage_taken\n')
    ordered(survival, ['bit SUBSTATUS_ENDURE, a', 'jr z, .not_enduring',
        'ld b, $1', 'jr .enduring', '.not_enduring\n',
        'call CheckOpponentAffection', 'jr z, .cont', 'inc a', 'ld b, a',
        'ld a, 20', 'call BattleRandomRange', 'cp b', 'ld b, $5',
        'jr c, .enduring', '.cont\n', 'call GetOpponentItem',
        'cp HELD_FOCUS_BAND', 'jr z, .focus_band', 'cp HELD_FOCUS_SASH',
        'jr z, .sturdy', 'call GetOpponentAbilityAfterMoldBreaker', 'cp STURDY',
        'farcall CheckOpponentFullHP', '.focus_band\n', 'call BattleRandom',
        'cp c', 'jr nc, .no_endure', '.enduring\n',
        'call BattleCommand_falseswipe', 'call TakeDamage', 'cp 5',
        'PlayerAffectionEndureText', 'EnemyAffectionEndureText',
        'jmp z, OpponentAffectionText'])
    phases = block(endturn, 'HandleBetweenTurnEffects:', 'HandleAffectionSelfCure:')
    ordered(phases, ['call HandleWeather', 'call CheckFaint',
        'call HandleAffectionSelfCure', 'call HandleFutureSight',
        'call HandleEndturnBlockA', 'call HandlePoison'])
    cure = block(endturn, 'HandleAffectionSelfCure:', 'HandleFutureSight:')
    ordered(cure, ['call SetFastestTurn', 'call .do_it', 'call SwitchTurn',
        'farcall CheckAffection', 'cp 2', 'ret c', 'ld a, 5',
        'call BattleRandomRange', 'and a', 'ret nz',
        'ld a, BATTLE_VARS_STATUS', 'call GetBattleVarAddr',
        'and a', 'ret z', 'ld [hl], 0', 'farcall AffectionText',
        'call UpdateBattleMonInParty', 'jmp UpdateEnemyMonInParty'])



def weakness_policy_phases(moves, effects):
    """Zero means no source command, one is early, two follows hit effects."""
    absent_damage = {'EFFECT_COUNTER', 'EFFECT_LEVEL_DAMAGE',
        'EFFECT_STATIC_DAMAGE', 'EFFECT_SUPER_FANG'}
    late_effects = {'EFFECT_DOUBLE_HIT', 'EFFECT_MULTI_HIT'}
    phases = {}
    for move in moves:
        effect = move['source_effect']
        names = [command.split()[0] for command in effects[effect]['commands']]
        phase = (0 if move['source_category'] == 'STATUS' or effect in absent_damage
            else 2 if effect in late_effects else 1)
        assert names.count('supereffectivetext') == bool(phase), move['target_move']
        if phase == 1:
            ordered('\n'.join(names), ['applydamage', 'supereffectivetext',
                'postfainteffects', 'posthiteffects', 'endmove'])
        elif phase == 2:
            ordered('\n'.join(names), ['applydamage', 'postfainteffects',
                'posthiteffects', 'supereffectivetext', 'endloop', 'endmove'])
        phases[move['target_move']] = phase
    assert {move for move, phase in phases.items() if phase == 2} == {
        'MOVE_DOUBLE_KICK', 'MOVE_BONEMERANG', 'MOVE_DOUBLE_SLAP',
        'MOVE_PIN_MISSILE', 'MOVE_ROCK_BLAST', 'MOVE_ICICLE_SPEAR',
        'MOVE_POLISHED_FURY_STRIKES'}
    assert [list(phases.values()).count(phase) for phase in (0, 1, 2)] == [86, 162, 7]
    return phases


def validate_weakness_policy_timing_source(files, moves, effects):
    phases = weakness_policy_phases(moves, effects)
    commands = files['engine/battle/effect_commands.asm']['text']
    policy = block(commands, 'BattleCommand_supereffectivetext:', 'ConsumeStolenOpponentItem::')
    ordered(policy, ['call StdBattleTextbox', 'pop af', 'ret c',
        'call HasOpponentFainted\n\tret z', 'call GetOpponentItemAfterUnnerve',
        'cp HELD_WEAKNESS_POLICY', 'call SwitchTurn',
        'ld b, $10 | ATTACK', 'call _RaiseStat', 'ld b, $10 | SP_ATTACK',
        'call _RaiseStat', 'call nz, ConsumeUserItem', 'jmp SwitchTurn'])
    assert not re.search(r'\b(?:wCurDamage|wDamageTaken|CheckDamage|CheckSubHit)\b', policy), \
        'The source effectiveness command needs a living holder, not a positive HP write'
    posthit = block(commands, 'BattleCommand_posthiteffects:', 'CheckStatHerbsAfterIntimidate:')
    ordered(posthit, ['call HasUserFainted\n\tjr nz, .check_parental_bond\n\tcall EndMoveEffect',
        '\n.check_parental_bond\n'])

    # False Swipe can leave zero damage while retaining the effectiveness
    # command. The low-byte HP decrement must remain a decrement at one HP.
    swipe = files['engine/battle/move_effects/false_swipe.asm']['text']
    ordered(swipe, ['ld de, wCurDamage', 'call StringCmp', 'jr c, .done',
        'ld a, [hli]', 'ld [de], a', 'inc de',
        'ld a, [hl]\n\tdec a\n\tld [de], a', 'jr nz, .okay'])
    assert phases['MOVE_FALSE_SWIPE'] == 1

    # A Future Sight setup ends before the damage script; its impact resumes
    # after futuresight and reaches this same effectiveness command.
    future = files['engine/battle/move_effects/future_sight.asm']['text']
    resume = block(future, 'BattleCommand_checkfuturesight:', 'BattleCommand_futuresight:')
    ordered(resume, ['call GetFutureSightUser', 'ret c', 'ld b, futuresight_command',
        'jmp SkipToBattleCommandAfter'])
    setup = block(future, 'BattleCommand_futuresight:', 'GetFutureSightUser::')
    ordered(setup, ['ld hl, ForesawAttackText', 'call StdBattleTextbox\n\tjmp EndMoveEffect'])
    assert phases['MOVE_FUTURE_SIGHT'] == 1

    # Policy's actual increments are already pending when a later hit ability
    # performs its immediate Mirror Herb checkpoint, even if that ability caps.
    abilities = files['engine/battle/abilities.asm']['text']
    stat_ability = block(abilities, 'StatUpAbility:', 'WeakArmorAbility:')
    ordered(stat_ability, ['farcall _ForceRaiseStat', 'ld a, [wFailedMessage]',
        '\n.done\n', 'call EndAbility', 'farjp CheckMirrorHerb'])
    armor = block(abilities, 'WeakArmorAbility:', 'FlashFireAbility:')
    ordered(armor, ['farcall LowerStat', 'ld b, $10 | SPEED', 'farcall RaiseStat',
        'call EndAbility', 'farjp CheckMirrorHerb'])
    stages = block(files['engine/battle/stats.asm']['text'], 'DoChangeStat:', '\n.lower_loop\n')
    ordered(stages, ['cp MAX_STAT_LEVEL', 'jr z, .queue_mirror_herb',
        'inc [hl]\n\tinc b', '\n.queue_mirror_herb\n',
        'ld hl, wMirrorHerbPendingBoosts', 'ld a, b', 'swap a', 'add [hl]'])
    return phases


def validate_item_closure_source(files):
    """Guard the reviewed sun, pinch, random-stat and reactive-item source paths.

    ``files`` is the authenticated_sources() mapping.  This function is standalone
    so the parent can integrate it without importing this scratch module.
    """
    def read(path):
        return files[path]['text']

    def section(text, start, end):
        assert start in text, f'Missing source label: {start}'
        tail = text.split(start, 1)[1]
        assert end in tail, f'Missing source boundary: {start} -> {end}'
        return tail.split(end, 1)[0]

    def sequence(text, *tokens):
        position = 0
        for token in tokens:
            found = text.find(token, position)
            assert found >= 0, f'Missing or reordered item-closure source token: {token}'
            position = found + len(token)

    abilities = read('engine/battle/abilities.asm')
    core = read('engine/battle/core.asm')
    commands = read('engine/battle/effect_commands.asm')
    home = read('home/battle.asm')

    # Both damaging sun abilities reach ordinary SubtractHP, whose item pass is
    # immediate, rather than waiting for the other battler's weather callback.
    sun = section(abilities, 'DrySkinWeatherAbility:', 'IceBodyAbility:')
    sequence(sun, 'call RainRecoveryAbility', 'SolarPowerWeatherAbility:',
             'call GetWeatherAfterUserUmbrella', 'cp WEATHER_SUN', 'ret nz',
             'call BeginAbility', 'call ShowAbilityActivation',
             'call GetEighthMaxHP', 'predef SubtractHPFromUser',
             'ld hl, IsHurtText', 'call StdBattleTextbox', 'jmp EndAbility')
    assert 'MAGIC_GUARD' not in sun, 'Sun ability damage has no Magic Guard gate'
    normal_hp = section(core, 'SubtractHPFromUser:\n', 'DoSubtractHPFromUser:')
    assert 'ld de, SubtractHP\n' in normal_hp, 'Sun must use the item-aware HP writer'
    healing = section(core, '\nSubtractHP:\n', '\n_SubtractHP:\n')
    sequence(healing, 'call _SubtractHP', 'HandleUserHealingItems:',
             'call HasUserFainted', 'ret z', 'call UseHeldStatusHealingItem',
             'call UseConfusionHealingItem', 'bit SUBSTATUS_PENDING_ITEMLOSS, a',
             'ret nz', 'call HandleHPHealingItem', 'call HandleStatBoostBerry')

    # Source quarter/half HP arithmetic floors to at least one, and CompareHP's
    # equality case is eligible.  Force-consumption enters after this threshold.
    pinch = section(core, 'QuarterPinchOrGluttony::', 'HandleStatBoostBerry:')
    sequence(pinch, 'call GetQuarterMaxHP', 'call GetTrueUserAbility',
             'cp GLUTTONY', 'call z, GetHalfMaxHP', 'call CompareHP',
             'ret nc', 'xor a', 'ret')
    hp_floor = section(home, 'GetQuarterMaxHP::', 'GetMaxHP::')
    sequence(hp_floor, 'call GetHalfMaxHP', 'jr HalveBC', 'GetHalfMaxHP::',
             'call GetMaxHP', 'HalveBC::', 'srl b', 'rr c', 'FloorBC::',
             'ld a, c', 'or b', 'ret nz', 'inc c')
    held_pinch = section(core, 'HandleStatBoostBerry:', 'ReconsumeStatBoostBerry:')
    sequence(held_pinch, 'call QuarterPinchOrGluttony', 'ret nz',
             'predef GetUserItemAfterUnnerve', 'call _HeldStatBoostBerry',
             'ret nz', 'farjp ConsumeUserItem')

    # Starf selects from raw nonmaxed Attack/Defense/Speed/SpAtk/SpDef stages;
    # Contrary is applied only by the selected stat operation, not to this mask.
    caps = section(abilities, 'GetCappedStats:', 'SelectRandomLowerStat:')
    sequence(caps, 'lb bc, 0, 0', 'lb de, 1, 0', 'ld a, [hl]', 'cp 13',
             'jr z, .maxed', 'or d', 'ld b, a', 'cp 5', 'jr c, .loop1',
             'SelectRandomRaiseStat:', 'ld a, b', 'and a', 'ret z',
             'ld h, b', 'call SelectRandomStat')
    assert 'CONTRARY' not in caps, 'Candidate caps are raw stages'
    selection = section(abilities, 'SelectRandomStat:', 'MoodyAbility:')
    sequence(selection, 'ld a, 5', 'call BattleRandomRange',
             'ld a, h', 'and d', 'jr z, .loop1')
    stat_berry = section(core, '_HeldStatBoostBerry:', 'ReconsumeHPHealingItem:')
    sequence(stat_berry, 'cp HELD_RAISE_STAT', 'cp HELD_RAISE_CRIT',
             'cp MULTIPLE_STATS', 'farcall GetCappedStats',
             'farcall SelectRandomRaiseStat', 'jr z, .failed',
             'ld a, $10', 'or e', 'ld b, a', '.got_stat\n',
             'ld a, STAT_SKIPTEXT | STAT_SILENT', 'farcall _ForceRaiseStat',
             'ld a, [wFailedMessage]', 'and a', 'ret nz',
             'farcall ShowPotentialAbilityActivation', 'farcall UseStatItemText')

    # Policy is attached to effective-text, through a doll, on the last ordinary
    # multi-hit strike but first Parental Bond strike.  One actual boost is needed.
    policy = section(commands, 'BattleCommand_supereffectivetext:',
                     'ConsumeStolenOpponentItem::')
    sequence(policy, 'bit SUBSTATUS_IN_ABILITY, a', 'ret nz',
             'bit SUBSTATUS_IN_LOOP, a', 'jr z, .continue',
             'ld a, [hl]', 'dec a', 'ret nz', '.continue\n',
             'ld a, [wTypeModifier]', 'cp EFFECTIVE', 'ret z',
             'call StdBattleTextbox', 'pop af', 'ret c',
             'call HasOpponentFainted', 'ret z',
             'call GetOpponentItemAfterUnnerve', 'cp HELD_WEAKNESS_POLICY',
             'ld b, $10 | ATTACK', 'call _RaiseStat',
             'ld a, [wFailedMessage]', 'jr nz, .atk_done',
             'inc b', 'ld b, $10 | SP_ATTACK', 'call _RaiseStat',
             'ld a, [wFailedMessage]', 'jr nz, .spatk_done',
             'inc b', '.spatk_done\n', 'ld a, b', 'and a',
             'call nz, ConsumeUserItem')
    assert 'CheckSub' not in policy, 'Weakness Policy does not reject a doll hit'

    # Kee/Maranga run at move end on any recorded damage, including Substitute.
    end_checks = section(commands, 'CheckEndMoveEffects:', 'CheckStatHerbs:')
    sequence(end_checks, 'call GetFutureSightUser', 'ret nz',
             'call HandleRampage', 'call CheckSheerForceNegation', 'ret z',
             'ld a, [wDamageTaken]', 'ld b, a',
             'ld a, [wDamageTaken + 1]', 'or b', 'call nz, EndMoveDamageChecks')
    defend = section(commands, '.EndMoveOpponentItems:', 'RaiseStatWithItem:')
    sequence(defend, 'call HasOpponentFainted', 'ret z',
             'call GetOpponentItemAfterUnnerve', 'cp HELD_DEFEND_HIT',
             'jr z, .defend_hit', '.defend_hit\n', 'ld a, c', 'cp PHYSICAL',
             'ld b, DEFENSE', 'jr z, .got_stat', 'ld b, SP_DEFENSE',
             'BATTLE_VARS_MOVE_CATEGORY', 'call GetBattleVar', 'cp c',
             'ret nz', 'call SwitchTurn', 'call RaiseStatWithItem')
    assert 'CheckSub' not in end_checks + defend, 'DefendHit berries accept doll damage'
    raise_item = section(commands, 'RaiseStatWithItem:', 'DittoMetalPowder:')
    sequence(raise_item, 'ld a, STAT_SKIPTEXT', 'call _RaiseStat',
             'ld a, [wFailedMessage]', 'and a', 'ret nz',
             'farcall UseStatItemText', 'jmp ConsumeUserItem')

    # Jaboca/Rowap have a distinct post-hit path: surviving dolls suppress it,
    # broken dolls permit it; Magic Guard and category gate retaliation.  Unlike
    # DefendHit, neither Sheer Force nor a fainted holder removes this branch.
    posthit = section(commands, 'BattleCommand_posthiteffects:',
                      'CheckStatHerbsAfterIntimidate:')
    sequence(posthit, 'call HasOpponentFainted', 'call nz, CheckSubstituteOpp',
             'ret nz', 'ld a, [wAttackMissed]', 'ret nz', 'farcall RunHitAbilities')
    hurt = section(posthit, '\n.rage_done\n', '\n.rocky_helmet_done\n')
    sequence(hurt, 'call HasUserFainted', 'jr z, .rocky_helmet_done',
             'call GetFutureSightUser', 'jr nz, .rocky_helmet_done',
             'call GetOpponentItemAfterUnnerve', 'cp HELD_OFFEND_HIT',
             'jr z, .offend_hit', '.offend_hit\n', 'call GetFutureSightUser',
             'jr nc, .rocky_helmet_done', 'BATTLE_VARS_MOVE_CATEGORY',
             'call GetBattleVar', 'cp c', 'jr nz, .rocky_helmet_done',
             'call GetEighthMaxHP', '.got_hurt_item_damage\n',
             'call GetTrueUserAbility', 'cp MAGIC_GUARD',
             'jr z, .rocky_helmet_done',
             'farcall SubtractHPFromUser_OverrideFaintOrder',
             'call nz, ConsumeOpponentItem')
    assert 'HasOpponentFainted' not in hurt, 'Retaliation can follow the holder KO'
    assert 'CheckSheerForceNegation' not in posthit, 'Post-hit retaliation survives Sheer Force'

    # Throat Spray checks the source miss latch rather than generic success.
    # Perish Song's own counter can succeed while Soundproof sets that latch;
    # repeating existing timers instead prints failure without setting it.
    spray = section(commands, 'CheckThroatSpray:', 'CheckWhiteHerbEjectPack:')
    sequence(spray, 'ld a, [wBattleEnded]', 'ret nz',
             'ld a, [wAttackMissed]', 'ret nz', 'call HasUserFainted', 'ret z',
             'predef GetUserItemAfterUnnerve', 'cp HELD_THROAT_SPRAY',
             'BATTLE_VARS_MOVE_ANIM', 'ld hl, SoundMoves',
             'call IsInByteArray', 'ret nc', 'ld b, c', 'jmp RaiseStatWithItem')
    perish = read('engine/battle/move_effects/perish_song.asm')
    sequence(perish, 'ld b, 1', '; Handle user Perish', 'ld a, [hl]',
             'jr nz, .user_done', 'ld [hl], 4', 'inc b', '.user_done\n',
             'ld a, [de]', 'jr nz, .opponent_done',
             'call BattleCommand_checkpriority', 'ld a, [wAttackMissed]',
             'jr nz, .immune', 'ld [de], a', '.opponent_done\n',
             'dec b', 'jr z, .failed', '.failed\n',
             'call AnimateFailedMove', 'jmp PrintButItFailed')
    assert 'wAttackMissed' not in perish.split('\n.failed\n', 1)[1]
    failed_text = section(commands, '\nPrintButItFailed:', '\nFailAttract:')
    sequence(failed_text, 'ld hl, ButItFailedText', 'jmp StdBattleTextbox')
    assert 'wAttackMissed' not in failed_text, 'Failure text does not set the source miss latch'


def validate_damage_healing_checkpoint_source(files):
    core = files['engine/battle/core.asm']['text']
    damage = block(core, 'DealDamageToOpponent:', 'SubtractHPFromUser_OverrideFaintOrder:')
    ordinary, berserk = damage.split('\n.berserk\n', 1)
    ordered(ordinary, ['cp BERSERK', 'jr z, .berserk', 'call SwitchTurn',
        'call SubtractHPFromUser', 'call CheckEnigmaBerry', 'jmp SwitchTurn'])
    ordered(berserk, ['set SUBSTATUS_IN_ABILITY, [hl]',
        'call SubtractHPFromUser_SkipItems', 'call CheckEnigmaBerry',
        'call GetHalfMaxHP', 'call CompareHP', 'res SUBSTATUS_IN_ABILITY, [hl]',
        'farcall ResolveOpponentBerserk_CheckMultihit', 'call HandleUserHealingItems'])
    healing = block(core, '\nSubtractHP:\n', '\n_SubtractHP:\n')
    ordered(healing, ['call _SubtractHP', 'HandleUserHealingItems:',
        'call HasUserFainted', 'ret z', 'call UseHeldStatusHealingItem',
        'call UseConfusionHealingItem', 'bit SUBSTATUS_PENDING_ITEMLOSS, a',
        'ret nz', 'call HandleHPHealingItem', 'call HandleStatBoostBerry'])
    enigma = block(core, 'CheckEnigmaBerry:', '\nSubtractHP:\n')
    ordered(enigma, ['ld a, [wTypeModifier]', 'cp EFFECTIVE + 1', 'ret c',
        'predef GetUserItemAfterUnnerve', 'cp HELD_ENIGMA_BERRY', 'ret nz',
        'call CheckFullHP', 'ret z', 'ld b, HELD_BERRY',
        'call _HeldHPHealingItem', 'ret nz', 'jmp UseBattleItem'])
    assert 'HasUserFainted' not in enigma and 'PENDING_ITEMLOSS' not in enigma
    restore = block(core, 'RestoreHP:', 'UpdateHPBarBattleHuds:')
    ordered(restore, ['ld hl, wBattleMonMaxHP', 'ld a, [hl]', 'add c',
        'ld [hld], a', 'adc b', 'ld [hli], a'])
    assert 'HasUserFainted' not in restore, 'Enigma can restore zero HP before faint processing'


def validate_future_sight_repeat_source(files, effects):
    # Future Sight reaches the same post-hit Parental Bond loop as an ordinary
    # attack. Only an external original user has no effective ability.
    ordered('\n'.join(effects['EFFECT_FUTURE_SIGHT']['commands']),
        ['checkhit', 'applydamage', 'criticaltext', 'postfainteffects',
         'posthiteffects', 'endmove'])
    commands = files['engine/battle/effect_commands.asm']['text']
    repeat = block(commands, '\n.check_parental_bond\n', '\n.flinch_up:')
    ordered(repeat, ['call HasOpponentFainted', 'ret z',
        'call GetTrueUserAbility', 'cp PARENTAL_BOND', 'ret nz',
        'bit SUBSTATUS_IN_LOOP, [hl]', 'ret nz',
        'bit SUBSTATUS_IN_ABILITY, [hl]', 'res SUBSTATUS_IN_ABILITY, [hl]',
        'jr nz, .resolve_berserk', 'set SUBSTATUS_IN_ABILITY, [hl]',
        'ld b, checkhit_command', 'jmp SkipToBattleCommandBackwards',
        '.resolve_berserk', 'farjp ResolveOpponentBerserk'])
    ability = block(files['home/battle.asm']['text'],
        'GetTrueUserAbility:', 'GetOpponentAbility::')
    ordered(ability, ['farcall GetFutureSightUser', 'jr z, .not_external',
        'xor a', 'ret', '.not_external', 'call StackCallOpponentTurn'])


def validate_primary_stat_source(files, moves, effects):
    target_stat_commands = {'loweroppstat', 'raiseoppstat'}
    primary_moves = {}
    for move in moves:
        commands = effects[move['source_effect']]['commands']
        names = [command.split()[0] for command in commands]
        if 'checkhit' in names and target_stat_commands.intersection(names):
            primary_moves[move['target_move']] = names
    assert set(primary_moves) == {'MOVE_LEER', 'MOVE_GROWL', 'MOVE_STRING_SHOT',
        'MOVE_SCREECH', 'MOVE_SMOKESCREEN', 'MOVE_FLASH', 'MOVE_SCARY_FACE',
        'MOVE_CHARM', 'MOVE_SWAGGER'}
    for names in primary_moves.values():
        stat_command = next(name for name in names if name in target_stat_commands)
        ordered('\n'.join(names), ['checkhit', 'checkpriority', 'bounceback', stat_command, 'endmove'])
    stats = files['engine/battle/stats.asm']['text']
    change = block(stats, 'FarChangeStat:', 'PrintStatChange:')
    ordered(change, ['bit STAT_CANMISS_F, b', 'ld a, [wAttackMissed]',
        'farjp GetFailureResultText', '\n.no_miss\n', 'bit STAT_SECONDARY_F, b',
        '\n.is_target\n', 'farcall CheckSubstituteOpp', '\n.check_lowering\n',
        'and GUARD_MIST', 'jr nz, .mist_or_item_fail', '\n.check_ability\n',
        'cp KEEN_EYE', 'ld c, ACCURACY', 'cp ILLUMINATE', 'jr z, .ability_check',
        '\n.check_item\n', '\n.perform_change\n', 'call DoRaiseStat', 'call DoLowerStat'])
    lowering = block(change, '\n.check_lowering\n', '\n.check_ability\n')
    assert 'INFILTRATOR' not in lowering, 'Guard Spec does not have an Infiltrator exception'
    item = block(files['engine/items/item_effects.asm']['text'], 'GuardSpec:', 'DireHit:')
    ordered(item, ['ld a, [wPlayerGuards]', 'and GUARD_MIST',
        'jmp nz, WontHaveAnyEffect_NotUsedMessage', 'ld a, 5 << 4',
        'ld hl, wPlayerGuards', 'or [hl]', 'ld [hl], a'])
    enemy = block(files['engine/battle/ai/items.asm']['text'], 'EnemyUsedGuardSpec:', 'EnemyUsedDireHit:')
    ordered(enemy, ['ld a, 5 << 4', 'ld hl, wEnemyGuards', 'or [hl]', 'ld [hl], a'])
    commands = files['engine/battle/effect_commands.asm']['text']
    checkhit = block(commands, 'BattleCommand_checkhit:', 'BattleCommand_checkpriority:')
    doll = block(checkhit, '\n.Substitute:\n', '\n.LockOn:\n')
    ordered(doll, ['call CheckSubstituteOpp', 'jr z, .not_blocked', 'cp SWAGGER',
        'jr z, .blocked', 'cp EFFECT_TRAP', 'jr z, .blocked'])
    assert 'GROWL' not in doll and 'EFFECT_ATTACK_DOWN' not in doll
    priority = block(commands, 'BattleCommand_checkpriority:', 'BattleCommand_effectchance:')
    ordered(priority, ['cp ARMOR_TAIL', 'cp PRANKSTER', 'cp SOUNDPROOF',
        '\n.attack_fails\n', 'ld hl, wAttackMissed', 'or [hl]',
        'jmp nz, BattleCommand_failuretext', 'ld [hl], b'])
    bounce = files['engine/battle/move_effects/magic_bounce.asm']['text']
    ordered(bounce, ['ld a, [wAttackMissed]', 'cp ATKFAIL_PROTECT', 'ret z',
        'call CheckHiddenOpponent', 'ret nz', 'call CheckSubstituteOpp', 'ret nz',
        'call UpdateMoveData', 'call BattleCommand_lowersub', 'call ResetTurn'])
    assert 'cp ATKFAIL_MISS' not in bounce
    ordered(checkhit, ['bit SUBSTATUS_IN_ABILITY, a', 'ret nz', 'call .DreamEater',
        'call .Protect', 'call .Substitute'])
    reset = block(commands, 'ResetTurn:', 'MoveDisabled:')
    do_move = block(commands, 'DoMove:', 'ReadMoveScriptByte:')
    assert 'wAttackMissed' not in reset + do_move, 'Reflected reentry preserves the original source miss'
    ordered(reset, ['ld [hl], a', 'ld [wAlreadyDisobeyed], a', 'call DoMove', 'jmp EndMoveEffect'])


def validate_miss_routing_source(files, moves, effects):
    """Bind the source commands that route a shared checkhit result."""
    commands = files['engine/battle/effect_commands.asm']['text']
    abilities = files['engine/battle/abilities.asm']['text']
    by_move = {move['target_move']: move for move in moves}

    def script(target):
        return [command.split()[0] for command in effects[by_move[target]['source_effect']]['commands']]

    # These late effect commands do not use the failure helper that consumes
    # Blunder Policy. Their earlier checkpriority remains a distinct route.
    manual = {
        'MOVE_LEECH_SEED': ('EFFECT_LEECH_SEED', 'leechseed'),
        'MOVE_ATTRACT': ('EFFECT_ATTRACT', 'attract'),
        'MOVE_FORESIGHT': ('EFFECT_FORESIGHT', 'foresight'),
        'MOVE_SUPERSONIC': ('EFFECT_CONFUSE', 'confuse'),
        'MOVE_CONFUSE_RAY': ('EFFECT_CONFUSE', 'confuse'),
        'MOVE_SWEET_KISS': ('EFFECT_CONFUSE', 'confuse'),
        'MOVE_TRICK': ('EFFECT_TRICK', 'trick'),
    }
    for target, (effect, command) in manual.items():
        assert by_move[target]['source_effect'] == effect, target
        names = script(target)
        ordered('\n'.join(names), ['checkhit', 'checkpriority', command, 'endmove'])
        assert 'failuretext' not in names, target

    seed = files['engine/battle/move_effects/leech_seed.asm']['text']
    ordered(seed, ['ld a, [wAttackMissed]\n\tand a\n\tjr nz, .evaded', '\n.evaded\n',
        'call AnimateFailedMove', 'ld hl, EvadedText', 'jmp StdBattleTextbox'])
    attract = block(files['engine/battle/move_effects/attract.asm']['text'],
        'BattleCommand_attract:', 'CheckOpponentMentalHerb:')
    ordered(attract, ['ld a, [wAttackMissed]', 'jr nz, .failed', '\n.failed\n', 'jmp FailAttract'])
    foresight = files['engine/battle/move_effects/foresight.asm']['text']
    ordered(foresight, ['ld a, [wAttackMissed]', 'jr nz, .failed', '\n.failed\n', 'jmp FailForesight'])
    confuse = block(commands, 'BattleCommand_confuse:', 'BattleCommand_rechargenextturn:')
    ordered(confuse, ['ld a, [wAttackMissed]', 'jr nz, Confuse_CheckSwagger_ConfuseHit',
        'Confuse_CheckSwagger_ConfuseHit:', 'cp EFFECT_CONFUSE_HIT', 'ret z',
        'cp EFFECT_SWAGGER', 'ret z', 'jmp PrintDidntAffect2'])
    trick = block(files['engine/battle/move_effects/trick.asm']['text'],
        'BattleCommand_trick:', '.OpponentCanBeTricked:')
    ordered(trick, ['ld a, [wAttackMissed]\n\tand a\n\tjr nz, .failed', '\n.failed\n',
        'call AnimateFailedMove', 'jmp PrintButItFailed'])
    direct_text = block(commands, 'PrintButItFailed:', 'CheckSubstituteOpp:')
    ordered(direct_text, ['ld hl, ButItFailedText', 'jmp StdBattleTextbox',
        'FailAttract:', 'FailForesight:', 'PrintDidntAffect2:',
        'call AnimateFailedMove', 'ld hl, DidntAffectText', 'jmp StdBattleTextbox'])
    for body in (seed, attract, foresight, confuse, trick, direct_text):
        assert not re.search(r'\b(?:GetFailureResultText|FailText_CheckOpponentProtect|'
            r'BattleCommand_failuretext|HELD_BLUNDER_POLICY)\b', body), 'Manual miss gained a Policy route'

    policy = block(commands, 'FailText_CheckOpponentProtect:', 'BattleCommand_criticaltext:')
    ordered(policy, ['\n.cont_atkmiss\n', 'predef GetUserItemAfterUnnerve',
        'cp HELD_BLUNDER_POLICY', 'ret nz', 'call _ForceRaiseStat',
        'ld a, [wFailedMessage]', 'jr nz, .policy_done',
        'farcall UseStatItemText', 'call ConsumeUserItem'])

    # Sing's earlier checkpowder can overwrite an accuracy miss. The sound
    # stat/confusion scripts instead reach checkpriority with that miss intact.
    ordered('\n'.join(script('MOVE_SING')), ['checkhit', 'checkpowder', 'checkpriority', 'bounceback'])
    for target in ('MOVE_GROWL', 'MOVE_SCREECH', 'MOVE_SUPERSONIC'):
        names = script(target)
        ordered('\n'.join(names), ['checkhit', 'checkpriority', 'bounceback'])
        assert 'checkpowder' not in names, target
    powder = block(commands, 'BattleCommand_checkpowder:', 'BattleCommand_resettypematchup:')
    ordered(powder, ['cp SING', 'jr nz, .not_sing', 'farcall CheckNullificationAbilities',
        'ld a, [wTypeMatchup]', 'ret nz', 'ld [wTypeModifier], a', '\n.not_sing\n',
        'cp THUNDER_WAVE', 'jr z, BattleCommand_resettypematchup',
        'cp TOXIC', '\n.check_corrosion\n', 'call GetTrueUserAbility', 'cp CORROSION',
        'ret z', 'cp TOXIC', 'jr z, BattleCommand_resettypematchup'])
    nullification = block(abilities, 'CheckNullificationAbilities:',
        'INCLUDE "data/abilities/type_nullification_abilities.asm"')
    sound = block(nullification, '\n.soundproof\n', '\n.flash_fire\n')
    ordered(sound, ['ld hl, SoundMoves', 'ld a, BATTLE_VARS_MOVE',
        'call IsInByteArray', 'jr c, .ability_ok'])
    assert 'wAttackMissed' not in sound, 'Soundproof nullification does not preserve an accuracy miss'
    forced = nullification.split('\n.check_forcedmiss\n', 1)[1]
    ordered(forced, ['ld a, [wAttackMissed]', 'dec a ; cp ATKFAIL_MISSED', 'ret z',
        'dec a ; cp ATKFAIL_PROTECT', 'ret z', '\n.ability_ok\n',
        'ld a, ATKFAIL_ABILITY', 'ld [wAttackMissed], a', 'ld [wTypeMatchup], a'])
    assert 'ATKFAIL_ACCMISS' not in forced, 'Ordinary accuracy misses do not defeat ability nullification'
    priority = block(commands, 'BattleCommand_checkpriority:', 'BattleCommand_effectchance:')
    ordered(priority, ['\n.attack_fails\n', 'ld hl, wAttackMissed', 'or [hl]',
        'jmp nz, BattleCommand_failuretext', 'ld [hl], b', 'jmp BattleCommand_failuretext'])

    # Type immunity is established before reflection and only replaces a hit.
    # A preexisting accuracy failure survives resettypematchup unchanged.
    for target in ('MOVE_TOXIC', 'MOVE_THUNDER_WAVE'):
        ordered('\n'.join(script(target)), ['checkhit', 'checkpowder', 'checkpriority', 'bounceback'])
    matchup = block(commands, 'BattleCommand_resettypematchup:', 'INCLUDE "data/moves/powder_moves.asm"')
    ordered(matchup, ['call BattleCheckTypeMatchup', 'ld a, [wTypeMatchup]',
        'jr nz, .reset', 'call ResetDamage', 'ld [wTypeModifier], a',
        'ld a, [wAttackMissed]', 'and a', 'ret nz', 'ld a, ATKFAIL_IMMUNE',
        'ld [wAttackMissed], a', '\n.reset\n', 'ld [wTypeModifier], a'])
    assert 'wAttackMissed' not in matchup.split('\n.reset\n', 1)[1]
    immunity = block(commands, '\n.RingTarget:\n', 'BattleCommand_checkpowder:')
    ordered(immunity, ['call GetOpponentItemAfterUnnerve', 'cp HELD_RING_TARGET',
        'jr z, .TypesLoop', '\n.Immune:\n', 'xor a', 'ld [wTypeMatchup], a'])

    for target, effect, command in (('MOVE_TRICK', 'EFFECT_TRICK', 'trick'),
            ('MOVE_PAIN_SPLIT', 'EFFECT_PAIN_SPLIT', 'painsplit')):
        assert by_move[target]['source_effect'] == effect
        names = script(target)
        ordered('\n'.join(names), ['checkhit', 'checkpriority', command, 'endmove'])
        assert 'bounceback' not in names and 'failuretext' not in names, target
    split = files['engine/battle/move_effects/pain_split.asm']['text']
    ordered(split, ['call CheckHiddenOpponent', 'jr nz, .failed', 'call AnimateCurrentMove',
        'predef SubtractHPFromUser', 'farcall RestoreHP', 'ld hl, SharedPainText',
        '\n.failed\n', 'call AnimateFailedMove', 'jmp PrintButItFailed'])
    assert 'wAttackMissed' not in split and 'GetFailureResultText' not in split

    # Every default damaging script except Counter/Mirror Coat performs its
    # hit check before priority nullification. Sucker Punch can exit earlier.
    exceptions = set()
    for move in moves:
        if move['source_category'] == 'STATUS':
            continue
        names = script(move['target_move'])
        if 'checkpriority' not in names:
            exceptions.add(move['target_move'])
            assert move['source_effect'] == 'EFFECT_COUNTER'
            ordered('\n'.join(names), ['checkhit', 'counter', 'failuretext', 'applydamage'])
        else:
            ordered('\n'.join(names), ['checkhit', 'checkpriority'])
    assert exceptions == {'MOVE_COUNTER', 'MOVE_MIRROR_COAT'}
    ordered('\n'.join(script('MOVE_SUCKER_PUNCH')), ['hastarget', 'suckerpunch', 'checkhit', 'checkpriority'])
    sucker = files['engine/battle/move_effects/sucker_punch.asm']['text']
    ordered(sucker, ['call CheckOpponentWentFirst', 'jr nz, .failed',
        'call SwitchTurn', 'ld a, BATTLE_VARS_MOVE_CATEGORY', 'call SwitchTurn',
        'cp STATUS', 'ret nz', '\n.failed\n', 'call AnimateFailedMove',
        'call TryPrintButItFailed', 'jmp EndMoveEffect'])


def validate_incoming_thaw_posthit_source(files, moves, effects):
    commands = files['engine/battle/effect_commands.asm']['text']
    burn_moves = {move['target_move'] for move in moves
        if 'burntarget' in effects[move['source_effect']]['commands']}
    assert burn_moves == {'MOVE_FIRE_PUNCH', 'MOVE_FLARE_BLITZ', 'MOVE_EMBER',
        'MOVE_FLAMETHROWER', 'MOVE_SCALD', 'MOVE_FIRE_BLAST', 'MOVE_SACRED_FIRE'}
    for move in moves:
        if move['target_move'] in burn_moves:
            ordered('\n'.join(effects[move['source_effect']]['commands']),
                ['applydamage', 'criticaltext', 'postfainteffects', 'effectchance',
                 'burntarget', 'posthiteffects', 'endmove'])
    critical = block(commands, 'BattleCommand_criticaltext:', 'BattleCommand_startloop:')
    ordered(critical, ['call ResetCrit', 'call HasOpponentFainted', 'jr z, .wait',
        'call GetOpponentAbilityAfterMoldBreaker', 'cp ANGER_POINT', 'call SwitchTurn',
        'farcall AngerPointAbility', 'call SwitchTurn'])
    burn = block(commands, 'BattleCommand_burntarget:', 'BurnTarget:')
    ordered(burn, ['call CheckSubHit', 'ret nz', 'ld a, BATTLE_VARS_STATUS_OPP',
        'and a', 'jr z, BurnTarget', 'call CheckSheerForceNegation', 'ret z',
        'jr DefrostOpponent'])
    assert not any(token in burn for token in ('wEffectFailed', 'HasOpponentFainted', 'BATTLE_VARS_MOVE_TYPE'))
    defrost = block(commands, 'DefrostOpponent:', 'CheckAlreadyExecuted:')
    ordered(defrost, ['call StackCallOpponentTurn', 'DefrostUser:',
        'BATTLE_VARS_STATUS', 'bit FRZ, [hl]', 'ret z', 'res FRZ, [hl]',
        'call UpdateUserInParty'])
    assert 'HasOpponentFainted' not in defrost and 'HasUserFainted' not in defrost
    sheer = block(commands, 'CheckSheerForceNegation:', 'HasBattleCommand:')
    ordered(sheer, ['call GetTrueUserAbility', 'cp SHEER_FORCE', 'ret nz',
        'ld b, effectchance_command', 'call HasBattleCommand', 'ret z',
        'ld b, selfeffectchance_command'])
    posthit = block(commands, 'BattleCommand_posthiteffects:', '\n.rocky_helmet_done\n')
    ordered(posthit, ['call HasOpponentFainted', 'call nz, CheckSubstituteOpp', 'ret nz',
        'ld a, [wAttackMissed]', 'ret nz', 'farcall RunHitAbilities',
        'call HasOpponentFainted', 'jr z, .defrost_done', 'call CheckSubHit',
        'jr nz, .defrost_done', 'BATTLE_VARS_MOVE_TYPE', 'cp FIRE',
        'call z, DefrostOpponent', '.defrost_done', 'call HasOpponentFainted',
        'call CheckAirBalloon', 'call ConsumeOpponentItem', '.air_balloon_done',
        'call HasOpponentFainted', 'SUBSTATUS_RAGE', 'call SwitchTurn', 'STAT_SKIPTEXT',
        'call _RaiseStat', 'wFailedMessage', 'RageBuildingText', 'farcall PrintStatChange',
        '.rage_done', 'call HasUserFainted', 'call GetFutureSightUser',
        'call GetOpponentItemAfterUnnerve'])
    broken_sub = block(commands, 'SelfInflictDamageToSubstitute:', 'UpdateMoveData:')
    ordered(broken_sub, ['call SetSubHit', 'res SUBSTATUS_SUBSTITUTE, [hl]'])
    tri_attack = files['engine/battle/move_effects/tri_attack.asm']['text']
    ordered(tri_attack, ['ld a, 3', 'call BattleRandomRange',
        'dw BattleCommand_paralyzetarget', 'dw BattleCommand_freezetarget',
        'dw BurnTarget ; burn, no defrosting'])
    assert 'BattleCommand_burntarget' not in tri_attack


def build():
    files = source.authenticated_sources()
    validate_source(files)
    validate_remaining_source(files)
    validate_curse_trick_source(files)
    validate_item_unburden_source(files)
    validate_recovery_source(files)
    validate_persistence_choice_source(files)
    validate_obedience_entry_source(files)
    validate_action_boundaries(files)
    validate_move_state_source(files)
    validate_switch_boundaries_source(files)
    validate_ability_survival_source(files)
    validate_status_pp_cleanup_source(files)
    move_files, moves, effects = attacks.inventory()
    validate_closure_followup_source(files, effects)
    validate_identity_cancellation_source(files)
    validate_medicine_crash_source(files)
    validate_room_screen_source(files, effects)
    validate_affection_source(files)
    validate_item_closure_source(files)
    policy_phases = validate_weakness_policy_timing_source(files, moves, effects)
    validate_damage_healing_checkpoint_source(files)
    validate_future_sight_repeat_source(files, effects)
    validate_primary_stat_source(files, moves, effects)
    validate_miss_routing_source(files, moves, effects)
    validate_incoming_thaw_posthit_source(files, moves, effects)
    used = sorted({m['source_effect'] for m in moves})
    command_names = sorted({line.split()[0] for name in used for line in effects[name]['commands']})
    assert len(moves) == 255 and len(used) == 133 and len(command_names) == 129
    for name in ('EFFECT_METRONOME', 'EFFECT_SLEEP_TALK'):
        caller = [line.split()[0] for line in effects[name]['commands']]
        assert 'doturn' in caller and 'hastarget' not in caller, name
    ordered('\n'.join(effects['EFFECT_FALSE_SWIPE']['commands']),
        ['damagevariation', 'falseswipe', 'moveanim', 'failuretext', 'applydamage'])
    for effect, command in [('EFFECT_SLEEP', 'sleep'), ('EFFECT_POISON', 'poison'),
            ('EFFECT_TOXIC', 'toxic'), ('EFFECT_PARALYZE', 'paralyze'), ('EFFECT_BURN', 'burn')]:
        ordered('\n'.join(effects[effect]['commands']), ['checksafeguard', command])
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
    ordered(constants, ['MOVEEND_POLISHED_POST_FAINT,', 'MOVEEND_POLISHED_CONTACT_ATTACKER,', 'MOVEEND_ABILITIES,', 'MOVEEND_POLISHED_POST_HIT,', 'MOVEEND_ITEM_EFFECTS_TARGET,'])
    ordered(constants, ['MOVEEND_ITEM_ON_STAT_CHANGE,', 'MOVEEND_POLISHED_ACTION_CHECKPOINT,', 'MOVEEND_POLISHED_DEFERRED_SWITCH,', 'MOVEEND_SEND_OUT_REPLACEMENTS,'])
    ordered(constants, ['MOVEEND_LIFE_ORB_SHELL_BELL,', 'MOVEEND_POLISHED_SWITCH_ITEMS,', 'MOVEEND_PICKPOCKET,', 'MOVEEND_POLISHED_STAT_HERBS,', 'MOVEEND_SPRAY_LEPPA_BLUNDER,'])
    assert '[MOVEEND_POLISHED_ACTION_CHECKPOINT] = MoveEndPolishedActionCheckpoint' in runtime
    paths = ['engine/battle/move_effects/sketch.asm', 'engine/battle/used_move_text.asm', 'engine/battle/core.asm', 'engine/battle/effect_commands.asm', 'engine/battle/abilities.asm', 'engine/battle/stats.asm', 'engine/battle/endturn.asm', 'data/abilities/no_intimidate_abilities.asm', 'data/moves/effects.asm', 'engine/battle/move_effects/baton_pass.asm', 'engine/battle/move_effects/roar.asm', 'engine/battle/move_effects/teleport.asm', 'data/items/attributes.asm']
    paths += ['engine/battle/move_effects/'+name+'.asm' for name in ('transform', 'skill_swap', 'conversion', 'thief', 'knock_off', 'bug_bite', 'heal_bell', 'curse', 'trick')]
    paths += ['data/moves/moves.asm', 'home/battle.asm', 'engine/items/item_effects.asm', 'engine/battle/move_effects/encore_disable.asm', 'engine/battle/move_effects/attract.asm']
    paths += ['engine/pokemon/learn.asm', 'engine/battle/evolve_battle.asm', 'engine/battle/move_effects/explosion.asm', 'engine/battle/move_effects/counter.asm']
    paths += ['engine/battle/move_effects/protect.asm', 'engine/battle/move_effects/roost.asm', 'engine/battle/move_effects/magic_bounce.asm',
        'data/abilities/mold_breaker_suppressed_abilities.asm',
        'data/abilities/type_nullification_abilities.asm', 'macros/data.asm']
    paths += ['engine/battle/move_effects/safeguard.asm', 'engine/battle/move_effects/pay_day.asm',
        'engine/battle/move_effects/rollout.asm', 'engine/events/whiteout.asm']
    paths += ['engine/battle/move_effects/minimize.asm', 'engine/battle/move_effects/focus_energy.asm',
        'engine/battle/misc.asm', 'engine/battle/move_effects/foresight.asm',
        'home/pokemon.asm', 'constants/pokemon_constants.asm']
    paths += ['engine/battle/move_effects/trick_room.asm', 'engine/battle/move_effects/brick_break.asm']
    paths += ['data/battle/critical_hit_chances.asm', 'engine/battle/move_effects/perish_song.asm', 'engine/battle/move_effects/tri_attack.asm', 'engine/battle/ai/items.asm']
    paths += ['engine/battle/move_effects/leech_seed.asm', 'engine/battle/move_effects/pain_split.asm', 'engine/battle/move_effects/sucker_punch.asm']
    paths += ['engine/battle/move_effects/false_swipe.asm', 'engine/battle/move_effects/future_sight.asm']
    report = dict(source_commit=source.source.PIN, source_variant='default; FAITHFUL undefined', suite=2,
        complete_behavior_parity=False,
        source_anchors={p: files[p]['sha'] if p in files else source.git_hash('blob', move_files[p].encode()) for p in paths},
        inventory=dict(moves=255, effects=133, commands=command_names,
            postfaint_effect_families=relevant,
            limitation='Command sequences authenticate coverage obligations; they do not certify untested conditional branches.'),
        contracts=[
            'Weakness Policy belongs to supereffectivetext: 162 mapped moves reach it before post-faint/hit effects, seven ordinary multi-hit moves after hit effects, and 86 source moves have no such command. Future Sight setup exits before the checkpoint but its delayed impact reaches it. A living holder can activate after zero-damage False Swipe; ordinary damage admission is not a source condition. Positive increments are pending before later hit-ability Mirror Herb checks; an early capped attempt cannot be retried after a secondary stat change. Late hit effects terminate the move when the user faints, but early Explosion still reaches Policy with a fainted user.',
            'Leech Seed, Attract, Foresight, Supersonic, Confuse Ray, Sweet Kiss and Trick print ordinary late failure text directly, without the Blunder Policy helper. Their earlier checkpriority failure can still route an existing miss through that helper.',
            'Sing runs checkpowder before checkpriority: its Soundproof nullification overwrites an ordinary accuracy miss. Growl, Screech and Supersonic reach checkpriority directly, which preserves an earlier miss. Toxic and Thunder Wave establish type immunity before reflection without overwriting an existing miss; reflected ResetTurn retains that failure.',
            'Trick and Pain Split check hit before priority without reflection. Pain Split then ignores the ordinary hit result but retains its separate hidden-target gate. All default damaging scripts except Counter/Mirror Coat check hit before priority; Sucker Punch first requires an eligible pending damaging action.',
            'Source direct damage resolves ordinary status/confusion and HP/stat healing before Enigma and critical text. A Berserk holder instead runs Enigma, threshold reevaluation and single-hit Berserk before ordinary healing; regular multihit and Parental Bond retain their deferred Berserk checkpoint. Pending item loss blocks HP/stat healing but not Enigma, whose source handler can restore zero HP before faint processing. Optional native items and formats retain their own activation paths.',
            'Active Future Sight users retain effective Parental Bond and repeat from checkhit within the impact script, including the second critical-text checkpoint. External original users have no ability. Intermediate ordinary healing can cancel the pending Berserk threshold before the second hit resolves; native Future Sight reentry remains unchanged.',
            'Nine primary target-stat scripts run accuracy before priority/nullification, reflection and FarChangeStat caps or immunity. A real miss can consume Blunder Policy despite a capped or protected stat; priority failure preserves that miss. Magic Bounce reflects an ordinary accuracy miss while retaining that failure through ResetTurn and skipping a second hit check; Protect, hiding or a blocking doll prevent reflection. Swagger alone has the early stat-move doll check. Source Guard Spec blocks lowering even with Infiltrator, defensive abilities precede Clear Amulet, and Illuminate protects Accuracy independently of native generation presets.',
            'Source Anger Point runs during critical text before burn-command thaw and post-hit effects, including a living holder behind a Substitute. The seven burntarget scripts thaw independently of secondary chance, Shield Dust, Cloak, current type and target life, except that SubHit and effective Sheer Force block this branch. Tri Attack calls BurnTarget directly and never inherits the native legacy thaw.',
            'Source posthiteffects runs hit abilities, then living real-hit current-Fire thaw, Air Balloon and Rage before reactive hit items. Broken Substitute permits balloon and Rage but its recorded SubHit still blocks thaw. Native phase order remains available for optional actions.',
            'The optional source traded-as-OT setting bypasses the foreign-player obedience branch before the 16-bit trainer ID and badge checks. Its default is off; native optional formats and either active gimmick retain native obedience.',
            'Optional source affection reads true-party friendship on either side, including external Future Sight; thresholds are 180/220/255 and Link/Tower disable it. Level 3 doubles critical numerator and independently misses one in ten after perfect-accuracy bypasses; level 2 or 3 cures one in five before Future Sight. Endure precedes affection survival at (level+1)/20, then Band/Sash/Sturdy. Source option unset preserves the default; optional native formats/gimmicks retain native affection.',
            'Direct Trick Room enables its counter before snapshotting Speed order for Room Service. A holder consumes only after a successful Speed drop; toggling off and expiry do not invoke the item pass.',
            'Brick Break ignores active screens for damage, then clears both screen counters after applydamage and critical text, including a lethal hit; miss, Protect and immunity end before the removal command. Native optional screen breakers keep their original timing.',
            'High Jump Kick failure damage uses half maximum HP and the same skip-items write as confusion and disobedience, without a Magic Guard exception. A later real HP update or genuine source healing sweep releases deferred HP/stat berries.',
            'Mapped source status medicines reject fainted targets. All-status medicines cure primary status and active confusion but preserve attraction; optional native medicines retain their behavior. Persim calls UseItemText, which falls through to disposal. Manual PP restoration preserves PP Up bits and permanent-party PP while excluding transformed or mismatched active moves.',
            'Attract rejects incompatible party genders, Fly/Dig and existing infatuation before Oblivious. CheckOppositeGender uses party species/gender even after Transform; Rivalry and Cute Charm share that source identity. Foresight rejects a repeated identification independently of native presets.',
            'Armor Suit is essential only when either party identity is specifically Armored Mewtwo; ordinary Mewtwo does not match the explicit form byte, and Transform does not change eligibility.',
            'Confusion and disobedience self-damage use SubtractHPFromUser_SkipItems, and a cancelled CheckTurn never reaches end-move healing. HP/stat berries wait for actual later damage, Unnerve departure or the final healing sweep; an intervening harmless action cannot trigger them.',
            'Selectable Switch/Predict mechanics ignore escape traps when offering a living reserve and reoffer after party cancellation. Accepted departure abilities precede departed-Speed send-in order, and both entrants hazards precede entry abilities. Source option initialization and Predict disclosure retain their original setup/presentation owner.',
            'Source PSS_OPT defaults on; the optional classic category branch keeps status moves as status and uses the original move-table type before ability conversion. The runtime option also changes the category recorded for Counter/Mirror Coat, while optional moves such as Weather Ball and Aqua Step and active gimmicks retain native categories.',
            'Minimize still sets its minimized bit when the forced Evasion increase reaches the cap; Focus Energy sets the full two-stage source bit independently of native presets and rejects a second application.',
            'Bug Bite executes through an intact Substitute, while Thief and Knock Off check the current Substitute after damage and permit the breaking hit or effective Infiltrator. Pending Knock Off berry loss uses that same Infiltrator-aware guard before damage; misses and Protect retain items.',
            'Source Justified, Rattled and Weak Armor can activate on the exact Substitute-breaking hit. Anger Point also activates on a living holder behind an intact Substitute, followed by its immediate Mirror Herb checkpoint; Gas still suppresses these abilities.',
            'Berserk is suppressed by Sheer Force even under optional native Champions presets. Enigma healing precedes its threshold decision, while ordinary healing follows it; pending multi-hit resolution must retain that ordering.',
            'Cud Chew clears its pending berry before retrying and announces only a successful berry effect; full HP, no matching status and capped stat or Focus Energy retries expire silently. Natural Cure uses the effective departing ability and correct party slot after Gas or Skill Swap.',
            'Source Tough Claws calls the common contact check, so Protective Pads suppresses its damage boost as well as reactive contact effects. Pinch, Rivalry, Sand Force, Steely Spirit, Galvanize and Adaptability retain source trigger/type/suppression and ordered arithmetic with native controls.',
            'Primary source status commands check Safeguard before ordinary status/type/ability rejection. CanStatusTarget retains matching-status/type checks before Substitute and ability checks after it; Infiltrator and Corrosion keep their distinct bypasses. Electric paralysis immunity is independent of native generation presets.',
            'Leppa reads active battle slots, including temporary transformed moves. Held recovery selects the first zero-PP move; stolen and Cud Chew recovery select the first raw packed PP byte unequal to its maximum. Source listed moves add recovery to the unmasked byte with eight-bit wrap, clamp to the old boosted maximum, then clear only that slot PP Ups; transformed recovery cannot overwrite original party PP or bonuses. Optional native moves retain independent PP storage.',
            'Pay Day uses either actor level times five and wraps the three-byte accumulator. An intact opposing Substitute blocks coins unless Infiltrator bypasses it; the breaking hit permits coins. Native side restrictions and saturation remain available.',
            'Sleeping source Rollout returns before both lock advancement and all Rollout/Defense Curl damage multiplication. Awake Rollout and native generation behavior retain their respective paths.',
            'Source full-party capture sends the current caught mon directly to PC and leaves all six original party backups intact. Actual terminal trainer/wild losses restore matching nonconsumed backups before later whiteout healing; consumed matching berries remain absent. Native optional caught-mon swap remains available.',
            'Source Metronome held-item streaks count attempted moves after CheckTurn and before obedience/PP, without requiring the item. Misses and Protect retain repeats, cancelled actions do not replace history, different executed moves reset it, and the counter caps at five. Called moves count the caller once; Struggle resets the count without replacing the selected identity.',
            'Source pre-action Metronome count is independent of native post-action history, preserving native boosts when a gimmick activates midbattle. Enemy source Baton Pass inherits selected-move identity/count because it skips NewEnemyMonStatus; player sendout clears its selected identity.',
            'Magic Bounce ResetTurn leaves the reflector in source called state through its later own action that turn. It skips that action\'s obedience/PP and Metronome increment and preserves previous last-move history; explicit caller/copy commands still clear their histories. Fresh selection or replacement ends the inherited called state.',
            'Protect/Endure share a chain reset by selecting another move or a nonmove action, even when that action is cancelled. A cancelled Protect/Endure retains the chain. Native optional protection moves, doubles and active gimmicks retain native chains.',
            'Metronome/Sleep Talk scripts spend caller PP before called reentry and have no hastarget Pressure charge. Source called charging retains the reentry flag through continuation, clears last/counter-move history, and clears that flag at the next free selection or replacement. Direct Fly/Dig/Solar Beam still pay Pressure once per attack.',
            'A failed first Rage does not start Rage, while a failed repeat leaves an existing Rage active. Selecting another move or an item clears Rage before the opposing action.',
            'Roost changes the active source types before healing, with pure Flying becoming Normal; full-HP rejection does not change types. Transform copies those current temporary types and keeps its copy after the original Roost expires.',
            'Shed Skin cures on 77 of 256 byte outcomes; Ice Body healing is allowed while flying or underground and checks holder Utility Umbrella. Moody selects only the five ordinary stats independently of native generation presets.',
            'Source status moves bypass Volt Absorb, Motor Drive, Water Absorb, Dry Skin and Sap Sipper; Lightning Rod and Flash Fire retain their source exceptions. Ordinary type immunity precedes nullification abilities. Bulletproof is not in the source Mold Breaker suppression list; Neutralizing Gas still suppresses it. Weak Armor raises Speed by two stages independently of native presets.',
            'Source lethal damage checks Endure, enabled affection, Focus Band/Sash, then Sturdy. Focus Band succeeds below raw byte 30; failure cannot fall back to Sturdy. Focus Sash consumes before Sturdy and requires full HP. False Swipe clamps damage before survival checks; native optional moves and formats retain their own ordering.',
            'Wild Red Card selects a living reserve on the attacking player side and retains the item when none exists. Pending outgoing actions are discarded. Roar alone uses the exact forced-target flag combination that applies Mold Breaker to entry hazards and suppresses Toxic Spikes Synchronize; item-forced entry uses the ordinary hazard path.',
            'FaintUserPokemon clears status, both binding counters and attraction, but retains Mean Look until replacement. Pursuit KO prevents departure abilities such as Regenerator; living departures still activate them.',
            'Rest rejects full HP, existing sleep, effective Insomnia/Vital Spirit and user Leaf Guard in sun after the opponent Utility Umbrella. This source Rest check ignores the user Umbrella and native generation presets. Successful Rest overwrites status with a three-tick sleep, heals fully, then checks status berries and ability cures.',
            'Counter/Mirror Coat require matching damage this turn and nonimmune target type; they double the last strike, not the sum, with unsigned saturation. Parental Bond repeats the doubled amount without doubling it again. Charging checks source sun/user Umbrella before Power Herb, consumes only one PP for continuation, and cancellation clears semi-invulnerability.',
            'In-battle evolution skips reloading the active Item field. The consumed temporary party item does not clear the source backup; active held items and final restoration retain it. Evolution after PostBattleTasks still consumes normally.',
            'Forgetting an occupied move slot clears matching active-player Disable and shared Encore/Choice locks before replacement, even for a benched learner. Other slots and native profiles retain their state.',
            'Source rampage lasts two or three actions independently of native presets, never starts from sleeping Sleep Talk, ends on a nonfinal miss without confusion, and confuses on final-turn cancellation unless already confused or effective Own Tempo applies. Fainted users and Future Sight impact do not run fatigue.',
            'Explosion clears user status/Leech Seed and opposing Destiny Bond, reserves the first faint, and resolves Unnerve Leppa/healing sweeps before attack damage. Damp prevents all selfdestruct state changes unless bypassed.',
            'Source action cancellation checks recharge, flinch, sleep/freeze (including self-thaw), confusion, attraction, Disable and paralysis before obedience and PP. CantMove clears Destiny Bond for a blocked repeat as well as other choices.',
            'Source obedience compares the original 16-bit trainer ID, ignores OT name and met level, uses Johto caps 10/20/30/50/70/101, and bypasses enemy, link, Tower and forced multi-turn actions. Default traded-as-OT is off. Accepted byte-draw ranges preserve source outcome probabilities; exact shared RNG call count remains Suite 3.',
            'A disobedient alternate move needs another slot with PP and no active Disable; it ignores Choice/Encore restrictions. Napping overwrites status with a uniform 1..7 counter, bypassing ordinary sleep prevention. All disobedient outcomes clear source move and Encore/Choice history after resolution.',
            'Forewarn compares source listed powers, uses 120 for Counter/Mirror Coat, skips zero-power moves, and has no activation when no attacking move exists. Equal powers are equiprobable; optional move sets retain native rankings. Anticipation ignores status moves including Thunder Wave.',
            'The six source X items always request two stages; successful use clears pending Mirror Herb copies. This is independent of native X-item generation presets.',
            'PostBattleTasks restores all six source held-item backups. Only consumption of the matching player berry clears its backup; removal does not. A different consumed berry cannot erase an original nonberry, and popped Air Balloon has no used-item history but still has its battle-start backup.',
            'Wild Thief/Pickpocket and wild Pickup fill an empty player backup; Harvest fills it in either battle mode. Wild Trick overwrites it even when nonempty. Trainer transfer leaves backups unchanged. Capture initializes the new party slot from the current held item.',
            'Choice history records every selected move before action cancellation, independently of the current held item or ability. Struggle and forced continuation do not select anew. Item changes preserve this history; switching clears it.',
            'Encore writes shared Choice history before Mental Herb; cure and expiry preserve that history. Transform and Sketch remap by surviving move identity. Native optional formats keep their separate history.',
            'Source Pickup recovers only the active opponent party slot history, including earlier turns and switch returns, clears that history once, refuses an occupied recipient and cannot be used by wild opponents. Harvest and Pickup preserve earned Unburden flags after ability changes. Recovery precedes the final healing pass; optional Recycle, doubles and active gimmicks retain native behavior. Post-battle backups are checked separately; exact RNG consumption remains open.',
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
            'Sketch clears user history before any failure, uses the opponent counter-move record, and rejects link mode, non-Infiltrator Substitute, transformed users, absent/Struggle history and any already-known move. Its CheckSubstituteOpp call inherits the effective Infiltrator bypass, including suppression checks.',
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
            'Source CantMove clears Destiny Bond even for a cancelled repeat choice. An executed repeat clears and fails; ordinary non-Destiny-Bond actions also clear it.'],
        live_bindings=['src/battle_move_resolution.c', 'src/battle_end_turn.c', 'src/battle_util.c',
            'src/battle_hold_effects.c', 'src/battle_stat_change.c', 'src/battle_main.c', 'src/battle_set_effect.c', 'src/battle_script_commands.c', 'src/battle_switch_in.c', 'src/item_use.c', 'src/polished_damage.c', 'data/battle_scripts_1.s'],
        regression_file='test/battle/polished_action_resolution.c',
        production_regression_files=['tools/polished_port/tests/shift_switch_fixture.c',
            'tools/polished_port/scripts/test_shift_switch_emulator.py',
            'tools/polished_port/scripts/test_battle_evolution_emulator.py'],
        additional_regression_files=['test/battle/hold_effect/polished_weakness_policy.c', 'test/battle/move_effect/polished_stat_command_order.c', 'test/battle/ability/polished_magic_bounce_accuracy.c', 'test/battle/status1/polished_incoming_thaw.c', 'test/battle/move_effect/polished_post_hit.c', 'test/battle/polished_affection.c', 'test/battle/polished_category_option.c', 'test/battle/polished_self_hit_items.c', 'test/battle/polished_item_branch_closure.c', 'test/battle/item_effect/polished_medicine_closure.c', 'test/battle/ability/polished_entry_closure.c', 'test/battle/move_effect/polished_command_identity.c', 'test/battle/move_effect/polished_command_closure.c', 'test/battle/ability/cud_chew.c', 'test/battle/polished_damage_closure.c', 'test/battle/ability/polished_cud_chew.c', 'test/battle/ability/polished_hit_stat_abilities.c', 'test/battle/ability/polished_departure_abilities.c', 'test/battle/move_effect/polished_command_edges.c', 'test/battle/hold_effect/polished_leppa.c', 'test/battle/status1/polished_status_gates.c', 'test/battle/polished_cleanup_closure.c', 'test/battle/move_effect/polished_pay_day.c', 'test/battle/polished_action_boundaries.c', 'test/battle/polished_move_history.c', 'test/battle/polished_switch_closure.c', 'test/battle/ability/polished_endturn_abilities.c', 'test/battle/ability/polished_absorption.c', 'test/battle/hold_effect/polished_survival_items.c', 'test/battle/move_effect/polished_protect_chain.c', 'test/battle/polished_obedience.c', 'test/battle/ability/polished_entry_information.c', 'test/battle/item_effect/polished_x_items.c', 'test/battle/polished_persistence.c', 'test/battle/polished_choice.c', 'test/battle/polished_deferred_switch.c', 'test/battle/polished_entry_items.c', 'test/battle/polished_stat_items.c', 'test/battle/polished_mirror_ledger.c', 'test/battle/polished_berserk_gene.c', 'test/battle/move_effect/polished_sketch.c', 'test/battle/move_effect/polished_suite2_copy.c', 'test/battle/move_effect/polished_suite2_conversion.c', 'test/battle/move_effect/polished_suite2_items.c', 'test/battle/polished_suite2_boss.c', 'test/battle/polished_suite2_switching.c'],
        implementation_review='tools/polished_port/SUITE2_CLOSURE_REVIEW.md',
        validation_record='tools/polished_port/SUITE2_VALIDATION.json',
        implementation_status='Suite 2 reconciled and locally validated with the explicit reachable Light Metal nontermination deviation',
        remaining=['Whole-battle certification retains the original Suite 3 context, AI, exact RNG stream and source presentation obligations.',
            'The documented source nontermination deviation remains explicit in the final parity assessment; newly discovered Suite 2 mechanical defects retain Suite 2 ownership.'])
    powers = {}
    for line in source.source.default_lines(move_files['data/moves/moves.asm']):
        if line.startswith('move '):
            row = [v.strip() for v in line[5:].split(',')]
            powers[row[0]] = 120 if row[1] == 'EFFECT_COUNTER' else int(row[2])
    entry = ['/* Generated from pinned source move rows and ForewarnAbility. */',
        'static const u8 sPolishedForewarnPower[MOVES_COUNT] = {']
    entry += [f'    [{m["target_move"]}] = {powers[m["source_move"]]},' for m in moves]
    entry += ['};', 'static const bool8 sPolishedEntryMoves[MOVES_COUNT] = {']
    entry += [f'    [{m["target_move"]}] = TRUE,' for m in moves]
    entry += ['};', '/* 0: absent, 1: before post-faint effects, 2: after hit effects. */',
        'static const u8 sPolishedWeaknessPolicyPhases[MOVES_COUNT] = {']
    entry += [f'    [{move}] = {phase},' for move, phase in policy_phases.items() if phase]
    entry += ['};', '']
    return {MANIFEST: json.dumps(report, indent=2)+'\n',
            ROOT/'src/data/polished_entry_abilities.h': '\n'.join(entry)}


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
