#!/usr/bin/env python3
"""Reject changes that invalidate authenticated action ordering contracts."""
import copy
import unittest
import build_action_resolution as action


class ActionResolutionTest(unittest.TestCase):
    def test_policy_checkpoint_phase_is_bound_for_every_source_effect(self):
        files = action.source.authenticated_sources()
        _, moves, effects = action.attacks.inventory()
        phases = action.validate_weakness_policy_timing_source(files, moves, effects)
        self.assertEqual([list(phases.values()).count(phase) for phase in (0, 1, 2)], [86, 162, 7])
        for effect in sorted({move['source_effect'] for move in moves}):
            with self.subTest(effect=effect):
                changed = copy.deepcopy(effects)
                commands = changed[effect]['commands']
                names = [command.split()[0] for command in commands]
                if 'supereffectivetext' in names:
                    first = names.index('supereffectivetext')
                    second = names.index('posthiteffects' if first < names.index('posthiteffects') else 'postfainteffects')
                    commands[first], commands[second] = commands[second], commands[first]
                else:
                    commands.insert(names.index('endmove'), 'supereffectivetext')
                with self.assertRaises(AssertionError):
                    action.validate_weakness_policy_timing_source(files, moves, changed)

    def test_policy_zero_damage_future_and_mirror_checkpoint_branches_are_authenticated(self):
        cases = [
            ('engine/battle/effect_commands.asm', 'BattleCommand_supereffectivetext:', 'ConsumeStolenOpponentItem::', 'call GetOpponentItemAfterUnnerve', 'ld a, [wCurDamage]\n\tand a\n\tret z\n\tcall GetOpponentItemAfterUnnerve'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_supereffectivetext:', 'ConsumeStolenOpponentItem::', 'call HasOpponentFainted', 'call HasUserFainted'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_supereffectivetext:', 'ConsumeStolenOpponentItem::', 'call nz, ConsumeUserItem', 'call z, ConsumeUserItem'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_posthiteffects:', 'CheckStatHerbsAfterIntimidate:', 'call HasUserFainted\n\tjr nz, .check_parental_bond\n\tcall EndMoveEffect', 'call HasUserFainted\n\tjr nz, .check_parental_bond\n\tnop'),
            ('engine/battle/move_effects/false_swipe.asm', 'BattleCommand_falseswipe:', None, 'ld a, [hl]\n\tdec a\n\tld [de], a', 'ld a, [hl]\n\tinc a\n\tld [de], a'),
            ('engine/battle/move_effects/future_sight.asm', 'BattleCommand_checkfuturesight:', 'BattleCommand_futuresight:', 'call GetFutureSightUser', 'call HasUserFainted'),
            ('engine/battle/move_effects/future_sight.asm', 'BattleCommand_checkfuturesight:', 'BattleCommand_futuresight:', 'ld b, futuresight_command', 'ld b, checkfuturesight_command'),
            ('engine/battle/move_effects/future_sight.asm', 'BattleCommand_futuresight:', 'GetFutureSightUser::', 'jmp EndMoveEffect', 'ret'),
            ('engine/battle/abilities.asm', 'StatUpAbility:', 'WeakArmorAbility:', 'farjp CheckMirrorHerb', 'ret'),
            ('engine/battle/abilities.asm', 'WeakArmorAbility:', 'FlashFireAbility:', 'farjp CheckMirrorHerb', 'ret'),
            ('engine/battle/stats.asm', 'DoChangeStat:', '\n.lower_loop\n', 'inc [hl]\n\tinc b', 'inc [hl]\n\tnop'),
        ]
        original = action.source.authenticated_sources()
        _, moves, effects = action.attacks.inventory()
        for path, label, end, before, after in cases:
            with self.subTest(path=path, label=label, mutation=before):
                files = copy.deepcopy(original)
                prefix, suffix = files[path]['text'].split(label, 1)
                body, tail = suffix.split(end, 1) if end else (suffix, '')
                self.assertIn(before, body)
                files[path]['text'] = prefix + label + body.replace(before, after, 1) + (end or '') + tail
                with self.assertRaises((AssertionError, ValueError, KeyError)):
                    action.validate_weakness_policy_timing_source(files, moves, effects)

    def test_future_sight_repeat_and_external_ability_are_authenticated(self):
        original = action.source.authenticated_sources()
        _, _, effects = action.attacks.inventory()
        cases = [
            ('engine/battle/effect_commands.asm', '\n.check_parental_bond\n', '\n.flinch_up:', before, after)
            for before, after in [
                ('call HasOpponentFainted', 'nop'),
                ('call GetTrueUserAbility', 'call GetOpponentAbility'),
                ('cp PARENTAL_BOND', 'cp PRESSURE'),
                ('bit SUBSTATUS_IN_LOOP, [hl]', 'bit SUBSTATUS_IN_ABILITY, [hl]'),
                ('bit SUBSTATUS_IN_ABILITY, [hl]', 'bit SUBSTATUS_IN_LOOP, [hl]'),
                ('res SUBSTATUS_IN_ABILITY, [hl]', 'nop'),
                ('jr nz, .resolve_berserk', 'jr z, .resolve_berserk'),
                ('set SUBSTATUS_IN_ABILITY, [hl]', 'nop'),
                ('ld b, checkhit_command', 'ld b, futuresight_command'),
                ('jmp SkipToBattleCommandBackwards', 'ret'),
                ('farjp ResolveOpponentBerserk', 'ret'),
            ]
        ] + [
            ('home/battle.asm', 'GetTrueUserAbility:', 'GetOpponentAbility::', before, after)
            for before, after in [
                ('farcall GetFutureSightUser', 'nop'),
                ('jr z, .not_external', 'jr nz, .not_external'),
                ('xor a', 'ld a, PARENTAL_BOND'),
                ('call StackCallOpponentTurn', 'ret'),
            ]
        ]
        action.validate_future_sight_repeat_source(original, effects)
        for path, label, end, before, after in cases:
            with self.subTest(path=path, mutation=before):
                files = copy.deepcopy(original)
                prefix, suffix = files[path]['text'].split(label, 1)
                body, tail = suffix.split(end, 1)
                self.assertIn(before, body)
                files[path]['text'] = prefix + label + body.replace(before, after, 1) + end + tail
                with self.assertRaises((AssertionError, ValueError)):
                    action.validate_future_sight_repeat_source(files, effects)

    def test_future_sight_repeat_keeps_the_impact_script_checkpoints(self):
        files = action.source.authenticated_sources()
        _, _, effects = action.attacks.inventory()
        for command in ('checkhit', 'applydamage', 'criticaltext', 'postfainteffects', 'posthiteffects'):
            with self.subTest(removed=command):
                changed = copy.deepcopy(effects)
                changed['EFFECT_FUTURE_SIGHT']['commands'].remove(command)
                with self.assertRaises(AssertionError):
                    action.validate_future_sight_repeat_source(files, changed)
        changed = copy.deepcopy(effects)
        commands = changed['EFFECT_FUTURE_SIGHT']['commands']
        first, second = commands.index('criticaltext'), commands.index('posthiteffects')
        commands[first], commands[second] = commands[second], commands[first]
        with self.assertRaises(AssertionError):
            action.validate_future_sight_repeat_source(files, changed)

    def _assert_miss_routing_mutations(self, cases):
        original = action.source.authenticated_sources()
        _, moves, effects = action.attacks.inventory()
        for path, label, end, before, after in cases:
            with self.subTest(path=path, label=label, mutation=before):
                files = copy.deepcopy(original)
                prefix, suffix = files[path]['text'].split(label, 1)
                body, tail = suffix.split(end, 1) if end else (suffix, '')
                self.assertIn(before, body)
                files[path]['text'] = prefix + label + body.replace(before, after, 1) + (end or '') + tail
                with self.assertRaises((AssertionError, ValueError, KeyError)):
                    action.validate_miss_routing_source(files, moves, effects)

    def test_manual_miss_routes_and_pain_split_ignore_are_authenticated(self):
        self._assert_miss_routing_mutations([
            ('engine/battle/move_effects/leech_seed.asm', 'BattleCommand_leechseed:', None, 'jr nz, .evaded', 'jr z, .evaded'),
            ('engine/battle/move_effects/leech_seed.asm', '\n.evaded\n', None, 'jmp StdBattleTextbox', 'jmp GetFailureResultText'),
            ('engine/battle/move_effects/attract.asm', 'BattleCommand_attract:', 'CheckOpponentMentalHerb:', 'ld a, [wAttackMissed]', 'ld a, [wEffectFailed]'),
            ('engine/battle/move_effects/attract.asm', '\n.failed\n', '\n.no_ability_protection\n', 'jmp FailAttract', 'jmp GetFailureResultText'),
            ('engine/battle/move_effects/foresight.asm', 'BattleCommand_foresight:', None, 'jmp FailForesight', 'jmp GetFailureResultText'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_confuse:', 'BattleCommand_rechargenextturn:', 'ld a, [wAttackMissed]\n\tand a\n\tjr nz, Confuse_CheckSwagger_ConfuseHit', 'ld a, [wAttackMissed]\n\tand a\n\tjr z, Confuse_CheckSwagger_ConfuseHit'),
            ('engine/battle/effect_commands.asm', 'Confuse_CheckSwagger_ConfuseHit:', 'BattleCommand_rechargenextturn:', 'jmp PrintDidntAffect2', 'jmp GetFailureResultText'),
            ('engine/battle/move_effects/trick.asm', 'BattleCommand_trick:', '.OpponentCanBeTricked:', 'jr nz, .failed', 'jr z, .failed'),
            ('engine/battle/move_effects/trick.asm', '\n.failed\n', '.OpponentCanBeTricked:', 'jmp PrintButItFailed', 'jmp GetFailureResultText'),
            ('engine/battle/effect_commands.asm', 'PrintButItFailed:', 'CheckSubstituteOpp:', 'jmp StdBattleTextbox', 'call GetFailureResultText\n\tjmp StdBattleTextbox'),
            ('engine/battle/effect_commands.asm', 'FailText_CheckOpponentProtect:', 'BattleCommand_criticaltext:', 'call _ForceRaiseStat', 'call OtherStatChange'),
            ('engine/battle/effect_commands.asm', 'FailText_CheckOpponentProtect:', 'BattleCommand_criticaltext:', 'call ConsumeUserItem', 'nop'),
            ('engine/battle/move_effects/pain_split.asm', 'BattleCommand_painsplit:', None, 'call CheckHiddenOpponent', 'ld a, [wAttackMissed]\n\tand a\n\tret nz\n\tcall CheckHiddenOpponent'),
            ('engine/battle/move_effects/pain_split.asm', 'BattleCommand_painsplit:', None, 'jr nz, .failed', 'jr z, .failed'),
            ('engine/battle/move_effects/sucker_punch.asm', 'BattleCommand_suckerpunch:', None, 'jr nz, .failed', 'jr z, .failed'),
            ('engine/battle/move_effects/sucker_punch.asm', 'BattleCommand_suckerpunch:', None, 'cp STATUS\n\tret nz', 'cp STATUS\n\tret z'),
            ('engine/battle/move_effects/sucker_punch.asm', 'BattleCommand_suckerpunch:', None, 'jmp EndMoveEffect', 'ret'),
        ])

    def test_nullification_and_immunity_miss_routing_are_authenticated(self):
        self._assert_miss_routing_mutations([
            ('engine/battle/effect_commands.asm', 'BattleCommand_checkpowder:', 'BattleCommand_resettypematchup:', 'cp SING', 'cp GROWL'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_checkpowder:', 'BattleCommand_resettypematchup:', 'farcall CheckNullificationAbilities', 'farcall OtherNullification'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_checkpowder:', 'BattleCommand_resettypematchup:', 'cp THUNDER_WAVE', 'cp GLARE'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_checkpowder:', 'BattleCommand_resettypematchup:', 'cp CORROSION', 'cp NO_ABILITY'),
            ('engine/battle/abilities.asm', 'CheckNullificationAbilities:', 'INCLUDE "data/abilities/type_nullification_abilities.asm"', 'ld hl, SoundMoves', 'ld hl, BulletMoves'),
            ('engine/battle/abilities.asm', 'CheckNullificationAbilities:', 'INCLUDE "data/abilities/type_nullification_abilities.asm"', '\n.soundproof\n', '\n.soundproof\n\tld a, [wAttackMissed]\n\tand a\n\tret nz\n'),
            ('engine/battle/abilities.asm', 'CheckNullificationAbilities:', 'INCLUDE "data/abilities/type_nullification_abilities.asm"', 'dec a ; cp ATKFAIL_MISSED', 'cp ATKFAIL_ACCMISS'),
            ('engine/battle/abilities.asm', 'CheckNullificationAbilities:', 'INCLUDE "data/abilities/type_nullification_abilities.asm"', 'dec a ; cp ATKFAIL_PROTECT', 'cp ATKFAIL_ACCMISS'),
            ('engine/battle/abilities.asm', 'CheckNullificationAbilities:', 'INCLUDE "data/abilities/type_nullification_abilities.asm"', 'ld a, ATKFAIL_ABILITY', 'ld a, ATKFAIL_ACCMISS'),
            ('engine/battle/abilities.asm', 'CheckNullificationAbilities:', 'INCLUDE "data/abilities/type_nullification_abilities.asm"', 'ld [wAttackMissed], a', 'ld [wEffectFailed], a'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_resettypematchup:', 'INCLUDE "data/moves/powder_moves.asm"', 'call BattleCheckTypeMatchup', 'nop'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_resettypematchup:', 'INCLUDE "data/moves/powder_moves.asm"', 'jr nz, .reset', 'jr z, .reset'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_resettypematchup:', 'INCLUDE "data/moves/powder_moves.asm"', 'ld a, [wAttackMissed]\n\tand a\n\tret nz', 'ld a, [wAttackMissed]\n\tand a\n\tret z'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_resettypematchup:', 'INCLUDE "data/moves/powder_moves.asm"', 'ld a, ATKFAIL_IMMUNE', 'ld a, ATKFAIL_ACCMISS'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_resettypematchup:', 'INCLUDE "data/moves/powder_moves.asm"', '\n.reset\n\tld [wTypeModifier], a', '\n.reset\n\tld [wTypeModifier], a\n\tld [wAttackMissed], a'),
            ('engine/battle/effect_commands.asm', '\n.RingTarget:\n', 'BattleCommand_checkpowder:', 'cp HELD_RING_TARGET', 'cp HELD_BRIGHTPOWDER'),
            ('engine/battle/effect_commands.asm', '\n.RingTarget:\n', 'BattleCommand_checkpowder:', 'ld [wTypeMatchup], a', 'ld [wTypeModifier], a'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_checkpriority:', 'BattleCommand_effectchance:', 'or [hl]', 'xor a'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_checkpriority:', 'BattleCommand_effectchance:', 'jmp nz, BattleCommand_failuretext', 'jmp z, BattleCommand_failuretext'),
        ])

    def test_named_miss_routing_scripts_cannot_change_their_failure_path(self):
        files = action.source.authenticated_sources()
        _, moves, effects = action.attacks.inventory()
        by_move = {move['target_move']: move for move in moves}
        for target in ('MOVE_LEECH_SEED', 'MOVE_ATTRACT', 'MOVE_FORESIGHT',
                'MOVE_SUPERSONIC', 'MOVE_CONFUSE_RAY', 'MOVE_SWEET_KISS', 'MOVE_TRICK', 'MOVE_PAIN_SPLIT'):
            with self.subTest(target=target, mutation='late failuretext'):
                changed = copy.deepcopy(effects)
                commands = changed[by_move[target]['source_effect']]['commands']
                commands.insert(-1, 'failuretext')
                with self.assertRaises(AssertionError):
                    action.validate_miss_routing_source(files, moves, changed)
        for target in ('MOVE_SING', 'MOVE_TOXIC', 'MOVE_THUNDER_WAVE'):
            with self.subTest(target=target, mutation='removed checkpowder'):
                changed = copy.deepcopy(effects)
                changed[by_move[target]['source_effect']]['commands'].remove('checkpowder')
                with self.assertRaises(AssertionError):
                    action.validate_miss_routing_source(files, moves, changed)
        for target in ('MOVE_GROWL', 'MOVE_SCREECH', 'MOVE_SUPERSONIC'):
            with self.subTest(target=target, mutation='added checkpowder'):
                changed = copy.deepcopy(effects)
                commands = changed[by_move[target]['source_effect']]['commands']
                commands.insert(commands.index('checkpriority'), 'checkpowder')
                with self.assertRaises(AssertionError):
                    action.validate_miss_routing_source(files, moves, changed)
        for target in ('MOVE_TRICK', 'MOVE_PAIN_SPLIT'):
            with self.subTest(target=target, mutation='priority before hit'):
                changed = copy.deepcopy(effects)
                commands = changed[by_move[target]['source_effect']]['commands']
                first, second = commands.index('checkhit'), commands.index('checkpriority')
                commands[first], commands[second] = commands[second], commands[first]
                with self.assertRaises(AssertionError):
                    action.validate_miss_routing_source(files, moves, changed)

    def test_damaging_priority_scripts_keep_hit_order_and_counter_sucker_exceptions(self):
        files = action.source.authenticated_sources()
        _, moves, effects = action.attacks.inventory()
        damaging_effects = {move['source_effect'] for move in moves if move['source_category'] != 'STATUS'}
        for effect in sorted(damaging_effects - {'EFFECT_COUNTER'}):
            with self.subTest(effect=effect, mutation='priority before hit'):
                changed = copy.deepcopy(effects)
                commands = changed[effect]['commands']
                first, second = commands.index('checkhit'), commands.index('checkpriority')
                commands[first], commands[second] = commands[second], commands[first]
                with self.assertRaises(AssertionError):
                    action.validate_miss_routing_source(files, moves, changed)
        changed = copy.deepcopy(effects)
        commands = changed['EFFECT_COUNTER']['commands']
        commands.insert(commands.index('checkhit') + 1, 'checkpriority')
        with self.assertRaises(AssertionError):
            action.validate_miss_routing_source(files, moves, changed)
        changed = copy.deepcopy(effects)
        commands = changed['EFFECT_SUCKER_PUNCH']['commands']
        first, second = commands.index('suckerpunch'), commands.index('checkhit')
        commands[first], commands[second] = commands[second], commands[first]
        with self.assertRaises(AssertionError):
            action.validate_miss_routing_source(files, moves, changed)
        changed_moves = [move for move in moves if move['target_move'] != 'MOVE_MIRROR_COAT']
        with self.assertRaises(AssertionError):
            action.validate_miss_routing_source(files, changed_moves, effects)

    def test_damage_healing_checkpoint_and_enigma_exceptions_are_authenticated(self):
        cases = [
            ('DealDamageToOpponent:', 'SubtractHPFromUser_OverrideFaintOrder:', 'call SubtractHPFromUser\n\tcall CheckEnigmaBerry', 'call CheckEnigmaBerry\n\tcall SubtractHPFromUser'),
            ('DealDamageToOpponent:', 'SubtractHPFromUser_OverrideFaintOrder:', 'call SubtractHPFromUser_SkipItems', 'call SubtractHPFromUser'),
            ('DealDamageToOpponent:', 'SubtractHPFromUser_OverrideFaintOrder:', 'farcall ResolveOpponentBerserk_CheckMultihit', 'farcall OtherBerserk'),
            ('\nSubtractHP:\n', '\n_SubtractHP:\n', 'call HandleHPHealingItem\n\tcall HandleStatBoostBerry', 'call HandleStatBoostBerry\n\tcall HandleHPHealingItem'),
            ('CheckEnigmaBerry:', '\nSubtractHP:\n', 'cp EFFECTIVE + 1', 'cp EFFECTIVE'),
            ('CheckEnigmaBerry:', '\nSubtractHP:\n', 'predef GetUserItemAfterUnnerve', 'call GetUserItem'),
            ('CheckEnigmaBerry:', '\nSubtractHP:\n', 'call CheckFullHP', 'call HasUserFainted\n\tcall CheckFullHP'),
            ('CheckEnigmaBerry:', '\nSubtractHP:\n', 'call CheckFullHP', 'bit SUBSTATUS_PENDING_ITEMLOSS, a\n\tcall CheckFullHP'),
            ('RestoreHP:', 'UpdateHPBarBattleHuds:', 'add c', 'call HasUserFainted\n\tadd c'),
        ]
        for label, end, before, after in cases:
            with self.subTest(label=label, mutation=before):
                files = copy.deepcopy(action.source.authenticated_sources())
                path = 'engine/battle/core.asm'
                prefix, suffix = files[path]['text'].split(label, 1)
                body, tail = suffix.split(end, 1)
                self.assertIn(before, body)
                files[path]['text'] = prefix + label + body.replace(before, after, 1) + end + tail
                with self.assertRaises((AssertionError, ValueError)):
                    action.validate_damage_healing_checkpoint_source(files)

    def test_primary_stat_miss_mist_and_illuminate_branches_are_authenticated(self):
        cases = [
            ('engine/battle/stats.asm', 'FarChangeStat:', 'PrintStatChange:', 'bit STAT_CANMISS_F, b', 'bit STAT_SILENT_F, b'),
            ('engine/battle/stats.asm', 'FarChangeStat:', 'PrintStatChange:', 'ld a, [wAttackMissed]', 'ld a, [wEffectFailed]'),
            ('engine/battle/stats.asm', 'FarChangeStat:', 'PrintStatChange:', 'and GUARD_MIST', 'cp INFILTRATOR\n\tand GUARD_MIST'),
            ('engine/battle/stats.asm', 'FarChangeStat:', 'PrintStatChange:', 'cp ILLUMINATE', 'cp NO_ABILITY'),
            ('engine/battle/stats.asm', 'FarChangeStat:', 'PrintStatChange:', 'ld c, ACCURACY', 'ld c, EVASION'),
            ('engine/items/item_effects.asm', 'GuardSpec:', 'DireHit:', 'ld a, 5 << 4', 'ld a, 4 << 4'),
            ('engine/battle/ai/items.asm', 'EnemyUsedGuardSpec:', 'EnemyUsedDireHit:', 'ld hl, wEnemyGuards', 'ld hl, wPlayerGuards'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_checkhit:', 'BattleCommand_checkpriority:', 'cp SWAGGER', 'cp GROWL'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_checkpriority:', 'BattleCommand_effectchance:', 'or [hl]', 'nop'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_checkpriority:', 'BattleCommand_effectchance:', 'jmp nz, BattleCommand_failuretext', 'jmp z, BattleCommand_failuretext'),
            ('engine/battle/effect_commands.asm', 'ResetTurn:', 'MoveDisabled:', 'ld [wAlreadyDisobeyed], a', 'ld [wAlreadyDisobeyed], a\n\tld [wAttackMissed], a'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_checkhit:', 'BattleCommand_checkpriority:', 'bit SUBSTATUS_IN_ABILITY, a', 'bit SUBSTATUS_RAGE, a'),
        ]
        _, moves, effects = action.attacks.inventory()
        for path, label, end, before, after in cases:
            with self.subTest(path=path, mutation=before):
                files = copy.deepcopy(action.source.authenticated_sources())
                prefix, suffix = files[path]['text'].split(label, 1)
                body, tail = suffix.split(end, 1)
                self.assertIn(before, body)
                files[path]['text'] = prefix + label + body.replace(before, after, 1) + end + tail
                with self.assertRaises((AssertionError, ValueError)):
                    action.validate_primary_stat_source(files, moves, effects)
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/move_effects/magic_bounce.asm'
        files[path]['text'] = files[path]['text'].replace('cp ATKFAIL_PROTECT', 'cp ATKFAIL_MISS')
        with self.assertRaises(AssertionError):
            action.validate_primary_stat_source(files, moves, effects)

    def test_primary_stat_script_inventory_and_accuracy_order_cannot_drift(self):
        files = action.source.authenticated_sources()
        _, moves, effects = action.attacks.inventory()
        changed_moves = [move for move in moves if move['target_move'] != 'MOVE_SWAGGER']
        with self.assertRaises(AssertionError):
            action.validate_primary_stat_source(files, changed_moves, effects)
        changed_effects = copy.deepcopy(effects)
        effect = next(move['source_effect'] for move in moves if move['target_move'] == 'MOVE_GROWL')
        commands = changed_effects[effect]['commands']
        hit, priority = commands.index('checkhit'), commands.index('checkpriority')
        commands[hit], commands[priority] = commands[priority], commands[hit]
        with self.assertRaises(AssertionError):
            action.validate_primary_stat_source(files, moves, changed_effects)

    def test_incoming_thaw_posthit_order_and_gates_are_authenticated(self):
        cases = [
            ('BattleCommand_burntarget:', 'BurnTarget:', 'call CheckSubHit', 'call OtherSubHit'),
            ('BattleCommand_burntarget:', 'BurnTarget:', 'call CheckSheerForceNegation', 'call OtherNegation'),
            ('BattleCommand_burntarget:', 'BurnTarget:', 'jr DefrostOpponent', 'ld a, [wEffectFailed]\n\tjr DefrostOpponent'),
            ('BattleCommand_burntarget:', 'BurnTarget:', 'jr DefrostOpponent', 'call HasOpponentFainted\n\tjr DefrostOpponent'),
            ('BattleCommand_burntarget:', 'BurnTarget:', 'jr DefrostOpponent', 'ld a, BATTLE_VARS_MOVE_TYPE\n\tjr DefrostOpponent'),
            ('DefrostOpponent:', 'CheckAlreadyExecuted:', 'res FRZ, [hl]', 'call HasUserFainted\n\tres FRZ, [hl]'),
            ('DefrostOpponent:', 'CheckAlreadyExecuted:', 'call UpdateUserInParty', 'call OtherPartyUpdate'),
            ('CheckSheerForceNegation:', 'HasBattleCommand:', 'call GetTrueUserAbility', 'call GetUserAbility'),
            ('BattleCommand_posthiteffects:', '\n.rocky_helmet_done\n', 'farcall RunHitAbilities', 'farcall OtherHitAbilities'),
            ('BattleCommand_posthiteffects:', '\n.rocky_helmet_done\n', 'jr z, .defrost_done', 'jr nz, .defrost_done'),
            ('BattleCommand_posthiteffects:', '\n.rocky_helmet_done\n', 'call CheckSubHit', 'call CheckSubstituteOpp'),
            ('BattleCommand_posthiteffects:', '\n.rocky_helmet_done\n', 'call CheckAirBalloon', 'call OtherBalloonCheck'),
            ('BattleCommand_posthiteffects:', '\n.rocky_helmet_done\n', 'STAT_SKIPTEXT', 'STAT_TARGET'),
            ('BattleCommand_posthiteffects:', '\n.rocky_helmet_done\n', 'RageBuildingText', 'OtherRageText'),
            ('SelfInflictDamageToSubstitute:', 'UpdateMoveData:', 'res SUBSTATUS_SUBSTITUTE, [hl]', 'nop'),
            ('BattleCommand_criticaltext:', 'BattleCommand_startloop:', 'call GetOpponentAbilityAfterMoldBreaker', 'call GetOpponentAbility'),
            ('BattleCommand_criticaltext:', 'BattleCommand_startloop:', 'farcall AngerPointAbility', 'farcall OtherAngerPoint'),
        ]
        _, moves, effects = action.attacks.inventory()
        for label, end, before, after in cases:
            with self.subTest(label=label, mutation=before):
                files = copy.deepcopy(action.source.authenticated_sources())
                path = 'engine/battle/effect_commands.asm'
                prefix, suffix = files[path]['text'].split(label, 1)
                body, tail = suffix.split(end, 1)
                self.assertIn(before, body)
                files[path]['text'] = prefix + label + body.replace(before, after, 1) + end + tail
                with self.assertRaises((AssertionError, ValueError)):
                    action.validate_incoming_thaw_posthit_source(files, moves, effects)
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/move_effects/tri_attack.asm'
        files[path]['text'] = files[path]['text'].replace('dw BurnTarget ; burn, no defrosting', 'dw BattleCommand_burntarget')
        with self.assertRaises(AssertionError):
            action.validate_incoming_thaw_posthit_source(files, moves, effects)

    def test_burn_command_inventory_and_critical_phase_cannot_drift(self):
        files = action.source.authenticated_sources()
        _, moves, effects = action.attacks.inventory()
        for mode in ('remove', 'add', 'reorder'):
            with self.subTest(mode=mode):
                changed_moves, changed_effects = copy.deepcopy(moves), copy.deepcopy(effects)
                if mode == 'remove':
                    changed_moves = [move for move in changed_moves if move['target_move'] != 'MOVE_SCALD']
                elif mode == 'add':
                    changed_effects['EFFECT_TRI_ATTACK']['commands'].append('burntarget')
                else:
                    effect = next(move['source_effect'] for move in changed_moves if move['target_move'] == 'MOVE_SCALD')
                    commands = changed_effects[effect]['commands']
                    critical, burn = commands.index('criticaltext'), commands.index('burntarget')
                    commands[critical], commands[burn] = commands[burn], commands[critical]
                with self.assertRaises(AssertionError):
                    action.validate_incoming_thaw_posthit_source(files, changed_moves, changed_effects)

    def test_item_closure_contracts_reject_branch_loss(self):
        cases = [
            ('dry_skin_sun_fallthrough', 'engine/battle/abilities.asm', 'DrySkinWeatherAbility:', 'IceBodyAbility:', 'call RainRecoveryAbility', 'call OtherRecoveryAbility'),
            ('sun_holder_umbrella', 'engine/battle/abilities.asm', 'DrySkinWeatherAbility:', 'IceBodyAbility:', 'GetWeatherAfterUserUmbrella', 'GetWeatherAfterOpponentUmbrella'),
            ('sun_weather_gate', 'engine/battle/abilities.asm', 'DrySkinWeatherAbility:', 'IceBodyAbility:', 'cp WEATHER_SUN', 'cp WEATHER_RAIN'),
            ('sun_damage_fraction', 'engine/battle/abilities.asm', 'DrySkinWeatherAbility:', 'IceBodyAbility:', 'call GetEighthMaxHP', 'call GetQuarterMaxHP'),
            ('sun_skip_items_route', 'engine/battle/core.asm', 'SubtractHPFromUser:\n', 'DoSubtractHPFromUser:', 'ld de, SubtractHP\n', 'ld de, _SubtractHP\n'),
            ('immediate_status_before_hp_items', 'engine/battle/core.asm', '\nSubtractHP:\n', '\n_SubtractHP:\n', 'call UseHeldStatusHealingItem\n\tcall UseConfusionHealingItem', 'call UseConfusionHealingItem\n\tcall UseHeldStatusHealingItem'),
            ('immediate_pending_itemloss_gate', 'engine/battle/core.asm', '\nSubtractHP:\n', '\n_SubtractHP:\n', 'bit SUBSTATUS_PENDING_ITEMLOSS, a', 'bit SUBSTATUS_FOCUS_ENERGY, a'),
            ('immediate_hp_item_checkpoint', 'engine/battle/core.asm', '\nSubtractHP:\n', '\n_SubtractHP:\n', 'call HandleHPHealingItem', 'call OtherHPHealingItem'),
            ('gluttony_half_only_on_match', 'engine/battle/core.asm', 'QuarterPinchOrGluttony::', 'HandleStatBoostBerry:', 'call z, GetHalfMaxHP', 'call nz, GetHalfMaxHP'),
            ('pinch_inclusive_threshold', 'engine/battle/core.asm', 'QuarterPinchOrGluttony::', 'HandleStatBoostBerry:', 'ret nc', 'ret c'),
            ('hp_minimum_one', 'home/battle.asm', 'GetQuarterMaxHP::', 'GetMaxHP::', '\tinc c\n', '\tdec c\n'),
            ('pinch_unnerve', 'engine/battle/core.asm', 'HandleStatBoostBerry:', 'ReconsumeStatBoostBerry:', 'predef GetUserItemAfterUnnerve', 'call GetUserItem'),
            ('starf_raw_maximum', 'engine/battle/abilities.asm', 'GetCappedStats:', 'SelectRandomLowerStat:', 'cp 13', 'cp 1'),
            ('starf_five_stat_mask', 'engine/battle/abilities.asm', 'GetCappedStats:', 'SelectRandomLowerStat:', 'cp 5', 'cp 7'),
            ('starf_rng_range', 'engine/battle/abilities.asm', 'SelectRandomStat:', 'MoodyAbility:', 'ld a, 5', 'ld a, 7'),
            ('starf_candidate_rejection', 'engine/battle/abilities.asm', 'SelectRandomStat:', 'MoodyAbility:', 'jr z, .loop1', 'jr nz, .loop1'),
            ('starf_selected_actual_stat_operation', 'engine/battle/core.asm', '_HeldStatBoostBerry:', 'ReconsumeHPHealingItem:', 'farcall _ForceRaiseStat', 'farcall _RaiseStat'),
            ('starf_failed_selected_stat', 'engine/battle/core.asm', '_HeldStatBoostBerry:', 'ReconsumeHPHealingItem:', 'ld a, [wFailedMessage]\n\tand a\n\tret nz', 'ld a, [wFailedMessage]\n\tand a\n\tret z'),
            ('policy_parental_second_hit', 'engine/battle/effect_commands.asm', 'BattleCommand_supereffectivetext:', 'ConsumeStolenOpponentItem::', 'bit SUBSTATUS_IN_ABILITY, a', 'bit SUBSTATUS_IN_LOOP, a'),
            ('policy_last_multihit', 'engine/battle/effect_commands.asm', 'BattleCommand_supereffectivetext:', 'ConsumeStolenOpponentItem::', 'dec a\n\tret nz', 'inc a\n\tret nz'),
            ('policy_effectiveness', 'engine/battle/effect_commands.asm', 'BattleCommand_supereffectivetext:', 'ConsumeStolenOpponentItem::', 'cp EFFECTIVE\n', 'cp SUPER_EFFECTIVE\n'),
            ('policy_actual_boost_consumption', 'engine/battle/effect_commands.asm', 'BattleCommand_supereffectivetext:', 'ConsumeStolenOpponentItem::', 'call nz, ConsumeUserItem', 'call ConsumeUserItem'),
            ('policy_no_doll_gate', 'engine/battle/effect_commands.asm', 'BattleCommand_supereffectivetext:', 'ConsumeStolenOpponentItem::', 'call GetOpponentItemAfterUnnerve', 'call CheckSubstituteOpp\n\tret nz\n\tcall GetOpponentItemAfterUnnerve'),
            ('defend_sheer_force', 'engine/battle/effect_commands.asm', 'CheckEndMoveEffects:', 'CheckStatHerbs:', 'call CheckSheerForceNegation', 'call OtherNegation'),
            ('defend_recorded_damage', 'engine/battle/effect_commands.asm', 'CheckEndMoveEffects:', 'CheckStatHerbs:', 'call nz, EndMoveDamageChecks', 'call z, EndMoveDamageChecks'),
            ('defend_holder_alive', 'engine/battle/effect_commands.asm', '.EndMoveOpponentItems:', 'RaiseStatWithItem:', 'call HasOpponentFainted\n\tret z', 'call HasOpponentFainted\n\tret nz'),
            ('defend_category_mapping', 'engine/battle/effect_commands.asm', '.EndMoveOpponentItems:', 'RaiseStatWithItem:', 'cp PHYSICAL', 'cp SPECIAL'),
            ('defend_no_doll_gate', 'engine/battle/effect_commands.asm', '.EndMoveOpponentItems:', 'RaiseStatWithItem:', 'call GetOpponentItemAfterUnnerve', 'call CheckSubHit\n\tret nz\n\tcall GetOpponentItemAfterUnnerve'),
            ('stat_item_requires_actual_change', 'engine/battle/effect_commands.asm', 'RaiseStatWithItem:', 'DittoMetalPowder:', 'ld a, [wFailedMessage]\n\tand a\n\tret nz', 'ld a, [wFailedMessage]\n\tand a\n\tret z'),
            ('retaliation_surviving_doll', 'engine/battle/effect_commands.asm', 'BattleCommand_posthiteffects:', 'CheckStatHerbsAfterIntimidate:', 'call nz, CheckSubstituteOpp', 'call z, CheckSubstituteOpp'),
            ('retaliation_allows_holder_ko', 'engine/battle/effect_commands.asm', '\n.rage_done\n', '\n.rocky_helmet_done\n', 'call GetOpponentItemAfterUnnerve', 'call HasOpponentFainted\n\tret z\n\tcall GetOpponentItemAfterUnnerve'),
            ('retaliation_eighth_hp', 'engine/battle/effect_commands.asm', '\n.rage_done\n', '\n.rocky_helmet_done\n', 'call GetEighthMaxHP', 'call GetQuarterMaxHP'),
            ('retaliation_magic_guard', 'engine/battle/effect_commands.asm', '\n.rage_done\n', '\n.rocky_helmet_done\n', 'cp MAGIC_GUARD', 'cp MAGIC_BOUNCE'),
            ('retaliation_sheer_force_independent', 'engine/battle/effect_commands.asm', 'BattleCommand_posthiteffects:', 'CheckStatHerbsAfterIntimidate:', 'call HandleRampage', 'call CheckSheerForceNegation\n\tret z\n\tcall HandleRampage'),
            ('spray_source_miss_latch', 'engine/battle/effect_commands.asm', 'CheckThroatSpray:', 'CheckWhiteHerbEjectPack:', 'ld a, [wAttackMissed]', 'ld a, [wEffectFailed]'),
            ('spray_source_sound_membership', 'engine/battle/effect_commands.asm', 'CheckThroatSpray:', 'CheckWhiteHerbEjectPack:', 'ld hl, SoundMoves', 'ld hl, OtherMoves'),
            ('perish_self_before_target', 'engine/battle/move_effects/perish_song.asm', 'BattleCommand_perishsong:', '\n.failed\n', 'ld [hl], 4', 'ld [hl], 3'),
            ('perish_target_priority', 'engine/battle/move_effects/perish_song.asm', 'BattleCommand_perishsong:', '\n.failed\n', 'call BattleCommand_checkpriority', 'call OtherPriorityCheck'),
            ('perish_existing_timers_do_not_set_miss', 'engine/battle/move_effects/perish_song.asm', '\n.failed\n', None, 'call AnimateFailedMove', 'ld [wAttackMissed], a\n\tcall AnimateFailedMove'),
            ('failure_text_does_not_set_miss', 'engine/battle/effect_commands.asm', '\nPrintButItFailed:', '\nFailAttract:', 'ld hl, ButItFailedText', 'ld [wAttackMissed], a\n\tld hl, ButItFailedText'),
        ]
        for name, path, label, end, before, after in cases:
            with self.subTest(name=name):
                files = copy.deepcopy(action.source.authenticated_sources())
                prefix, suffix = files[path]['text'].split(label, 1)
                body, tail = suffix.split(end, 1) if end is not None else (suffix, '')
                self.assertIn(before, body)
                files[path]['text'] = prefix + label + body.replace(before, after, 1) + (end or '') + tail
                with self.assertRaises((AssertionError, IndexError)):
                    action.validate_item_closure_source(files)

    def test_traded_as_ot_rejects_lost_obedience_bypass(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/effect_commands.asm'
        label = 'BattleCommand_checkobedience:'
        prefix, suffix = files[path]['text'].split(label, 1)
        token = 'bit TRADED_AS_OT_OPT, a\n\tret nz'
        self.assertIn(token, suffix)
        files[path]['text'] = prefix + label + suffix.replace(token, 'bit TRADED_AS_OT_OPT, a\n\tnop', 1)
        with self.assertRaises(AssertionError):
            action.validate_obedience_entry_source(files)

    def test_affection_contracts_reject_branch_loss(self):
        cases = [
            ('engine/battle/effect_commands.asm', 'CheckAffection:', 'bit AFFECTION_OPT, a', 'bit 0, a'),
            ('engine/battle/effect_commands.asm', 'CheckAffection:', 'ld a, [wLinkMode]', 'ld a, [wBattleMode]'),
            ('engine/battle/effect_commands.asm', 'CheckAffection:', 'ld a, [wInBattleTowerBattle]', 'ld a, [wBattleMode]'),
            ('engine/battle/effect_commands.asm', 'CheckAffection:', 'call TrueUserPartyAttr', 'call UserPartyAttr'),
            ('engine/battle/effect_commands.asm', 'CheckAffection:', '\tdb 220', '\tdb 219'),
            ('engine/battle/effect_commands.asm', 'CheckAffection:', '\tdb 180', '\tdb 181'),
            ('home/battle.asm', 'TrueUserPartyAttr::', 'ld hl, wOTPartyMons', 'ld hl, wPartyMons'),
            ('home/battle.asm', 'TrueUserPartyAttr::', 'farcall GetFutureSightUser', 'nop'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_critical:', 'sla b', 'inc c'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_checkhit:', 'ld a, 10', 'ld a, 20'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_checkhit:', 'cp 3\n\tjr c, .no_affection_evasion', 'cp 2\n\tjr c, .no_affection_evasion'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_applydamage:', 'inc a\n\tld b, a', 'ld b, a'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_applydamage:', 'ld a, 20', 'ld a, 10'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_applydamage:', 'jr z, .cont', 'jr z, .no_endure'),
            ('engine/battle/endturn.asm', 'HandleBetweenTurnEffects:', 'call HandleAffectionSelfCure\n\tcall HandleFutureSight', 'call HandleFutureSight\n\tcall HandleAffectionSelfCure'),
            ('engine/battle/endturn.asm', 'HandleAffectionSelfCure:', 'cp 2\n\tret c', 'cp 3\n\tret c'),
            ('engine/battle/endturn.asm', 'HandleAffectionSelfCure:', 'ld a, 5', 'ld a, 4'),
            ('engine/battle/endturn.asm', 'HandleAffectionSelfCure:', 'call SwitchTurn', 'nop'),
            ('engine/battle/endturn.asm', 'HandleAffectionSelfCure:', 'jmp UpdateEnemyMonInParty', 'ret'),
        ]
        for path, label, token, replacement in cases:
            with self.subTest(path=path, token=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                prefix, suffix = files[path]['text'].split(label, 1)
                self.assertIn(token, suffix)
                files[path]['text'] = prefix + label + suffix.replace(token, replacement, 1)
                with self.assertRaises((AssertionError, IndexError)):
                    action.validate_affection_source(files)

    def test_room_screen_contracts_reject_branch_loss(self):
        _, _, effects = action.attacks.inventory()
        for path, label, token in [
            ('engine/battle/move_effects/trick_room.asm', '', 'ld [hl], 5'),
            ('engine/battle/move_effects/trick_room.asm', 'HandleRoomService:', 'call SetFastestTurn'),
            ('engine/battle/move_effects/trick_room.asm', 'RoomServiceItem:', 'ret nz'),
            ('engine/battle/move_effects/brick_break.asm', '', 'ld [hl], 0'),
            ('engine/battle/effect_commands.asm', 'GetOpponentActiveScreens:', 'xor EFFECT_BRICK_BREAK')]:
            with self.subTest(path=path, token=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                prefix, suffix = files[path]['text'].split(label, 1) if label else ('', files[path]['text'])
                self.assertIn(token, suffix)
                files[path]['text'] = prefix + label + suffix.replace(token, 'nop', 1)
                with self.assertRaises((AssertionError, IndexError)):
                    action.validate_room_screen_source(files, effects)

    def test_brick_break_rejects_pre_damage_screen_removal(self):
        files = action.source.authenticated_sources()
        _, _, effects = action.attacks.inventory()
        commands = effects['EFFECT_BRICK_BREAK']['commands']
        commands.remove('brickbreak')
        commands.insert(commands.index('applydamage'), 'brickbreak')
        with self.assertRaises(AssertionError):
            action.validate_room_screen_source(files, effects)

    def test_medicine_crash_contracts_reject_branch_loss(self):
        cases = [
            ('engine/battle/effect_commands.asm', 'GetFailureResultText:', 'call GetHalfMaxHP'),
            ('engine/battle/effect_commands.asm', 'GetFailureResultText:', 'jmp TakeOpponentDamage'),
            ('engine/items/item_effects.asm', 'UseStatusHealer:', 'call IsMonFainted'),
            ('engine/items/item_effects.asm', 'UseStatusHealer:', 'inc a ; cp $ff'),
            ('engine/items/item_effects.asm', '\nHealStatus:', 'res SUBSTATUS_CONFUSED, [hl]'),
            ('engine/items/item_effects.asm', 'PersimBerry:', 'call UseItemText'),
            ('engine/items/item_effects.asm', 'UseDisposableItem:', 'jmp TossItem'),
            ('engine/items/item_effects.asm', 'BattleRestorePP:', 'bit SUBSTATUS_TRANSFORMED, a'),
            ('engine/items/item_effects.asm', 'BattleRestorePP:', 'cp [hl]'),
            ('engine/items/item_effects.asm', '\nRestorePP:', 'and 3 << 6'),
        ]
        for path, label, token in cases:
            with self.subTest(path=path, token=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                prefix, suffix = files[path]['text'].split(label, 1)
                self.assertIn(token, suffix)
                files[path]['text'] = prefix + label + suffix.replace(token, 'nop', 1)
                with self.assertRaises((AssertionError, IndexError)):
                    action.validate_medicine_crash_source(files)

    def test_medicine_crash_reject_invented_exemptions(self):
        for path, label, insertion in [
            ('engine/battle/effect_commands.asm', 'GetFailureResultText:', '\n\tcp MAGIC_GUARD'),
            ('engine/items/item_effects.asm', '\nHealStatus:', '\n\tres SUBSTATUS_IN_LOVE, [hl]'),
            ('engine/items/item_effects.asm', 'UseItemText:', '\n\tret')]:
            with self.subTest(path=path, label=label):
                files = copy.deepcopy(action.source.authenticated_sources())
                files[path]['text'] = files[path]['text'].replace(label, label + insertion, 1)
                with self.assertRaises(AssertionError):
                    action.validate_medicine_crash_source(files)

    def test_identity_cancellation_contracts_reject_branch_loss(self):
        cases = [
            ('engine/battle/move_effects/attract.asm', '', 'call CheckHiddenOpponent'),
            ('engine/battle/move_effects/attract.asm', 'CheckOppositeGender:', 'call TrueUserPartyAttr'),
            ('engine/battle/move_effects/foresight.asm', '', 'bit SUBSTATUS_IDENTIFIED, [hl]'),
            ('engine/battle/effect_commands.asm', 'UserCanLoseItem:', 'ld a, MON_FORM'),
            ('home/pokemon.asm', 'CompareSpeciesForm:', 'and FORM_MASK'),
            ('engine/battle/effect_commands.asm', 'DoTurn:', 'bit 7, a'),
            ('engine/battle/effect_commands.asm', 'HitConfusion:', 'call TakeOpponentDamage'),
            ('engine/battle/effect_commands.asm', 'TakeOpponentDamage:', 'farcall SubtractHPFromUser_SkipItems'),
            ('engine/battle/core.asm', 'SubtractHPFromUser_SkipItems:', 'ld de, _SubtractHP'),
            ('engine/battle/core.asm', 'OfferSwitch:', 'jr c, OfferSwitch'),
            ('engine/battle/endturn.asm', '', 'farcall SpikesDamageBoth'),
            ('home/battle.asm', 'GetFixedCategory::', 'bit PSS_OPT, a'),
            ('home/battle.asm', 'GetFixedCategory::', 'ld bc, MOVE_TYPE'),
        ]
        for path, label, token in cases:
            with self.subTest(path=path, token=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                prefix, suffix = files[path]['text'].split(label, 1) if label else ('', files[path]['text'])
                self.assertIn(token, suffix)
                files[path]['text'] = prefix + label + suffix.replace(token, 'nop', 1)
                with self.assertRaises((AssertionError, IndexError)):
                    action.validate_identity_cancellation_source(files)

    def test_self_hit_rejects_an_invented_action_end_healing_sweep(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/core.asm'
        files[path]['text'] = files[path]['text'].replace('PerformMove:',
            'PerformMove:\n\tcall HandleHealingItems', 1)
        with self.assertRaises(AssertionError):
            action.validate_identity_cancellation_source(files)

    def test_closure_followup_contracts_reject_branch_loss(self):
        _, _, effects = action.attacks.inventory()
        cases = [
            ('engine/battle/move_effects/minimize.asm', '', 'set SUBSTATUS_MINIMIZED, [hl]'),
            ('engine/battle/move_effects/focus_energy.asm', '', 'set SUBSTATUS_FOCUS_ENERGY'),
            ('engine/battle/effect_commands.asm', '.FocusEnergy:', 'inc c'),
            ('engine/battle/move_effects/knock_off.asm', '', 'CheckSubstituteOpp'),
            ('engine/battle/move_effects/thief.asm', '', 'call CanStealItem'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_criticaltext:', 'call HasOpponentFainted'),
            ('engine/battle/effect_commands.asm', 'SelfInflictDamageToSubstitute:', 'res SUBSTATUS_SUBSTITUTE, [hl]'),
            ('engine/battle/abilities.asm', 'ResolveOpponentBerserk:', 'farcall CheckSheerForceNegation'),
            ('engine/battle/core.asm', 'DealDamageToOpponent:', 'call CheckEnigmaBerry ; takes priority over Berserk'),
            ('engine/battle/abilities.asm', 'CudChewAbility:', 'xor a\n\tld [hl], a'),
            ('engine/battle/core.asm', 'ReconsumeHPHealingItem:', 'call CheckFullHP'),
            ('engine/battle/core.asm', '_HeldStatusHealingItem:', 'farcall ShowPotentialAbilityActivation'),
            ('engine/battle/misc.asm', '_CheckContactMove::', 'cp HELD_PROTECTIVE_PADS'),
            ('engine/battle/abilities.asm', 'ToughClawsAbility:', 'ret c'),
            ('engine/battle/abilities.asm', 'PinchAbility:', 'call CheckPinch'),
            ('engine/battle/abilities.asm', 'RivalryAbility:', 'ret c'),
            ('engine/battle/abilities.asm', 'GutsAbility:', 'jmp ApplyPhysicalAttackDamageMod'),
            ('engine/battle/abilities.asm', 'AnalyticAbility:', 'xor b'),
        ]
        for path, label, token in cases:
            with self.subTest(path=path, token=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                prefix, suffix = files[path]['text'].split(label, 1) if label else ('', files[path]['text'])
                self.assertIn(token, suffix)
                files[path]['text'] = prefix + label + suffix.replace(token, 'nop', 1)
                with self.assertRaises((AssertionError, IndexError)):
                    action.validate_closure_followup_source(files, effects)

    def test_closure_followup_effect_order_rejects_lost_commands(self):
        files = action.source.authenticated_sources()
        _, _, original = action.attacks.inventory()
        for effect, token in [('EFFECT_MINIMIZE', 'minimize'),
                              ('EFFECT_BUG_BITE', 'bugbite FALSE'),
                              ('EFFECT_BUG_BITE', 'bugbite TRUE'),
                              ('EFFECT_KNOCK_OFF', 'knockoff FALSE'),
                              ('EFFECT_THIEF', 'posthiteffects')]:
            with self.subTest(effect=effect, token=token):
                effects = copy.deepcopy(original)
                self.assertIn(token, effects[effect]['commands'])
                effects[effect]['commands'].remove(token)
                with self.assertRaises(AssertionError):
                    action.validate_closure_followup_source(files, effects)

    def test_sketch_uses_the_transitive_infiltrator_substitute_guard(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/effect_commands.asm'
        prefix, suffix = files[path]['text'].split('CheckSubstituteOpp:', 1)
        files[path]['text'] = prefix + 'CheckSubstituteOpp:' + suffix.replace('cp INFILTRATOR', 'nop', 1)
        with self.assertRaises(AssertionError):
            action.validate_source(files)

    def test_cud_chew_and_bug_bite_reject_invented_early_gates(self):
        _, _, effects = action.attacks.inventory()
        for path, label, insertion in [
            ('engine/battle/abilities.asm', 'CudChewAbility:', '\n\tcall ShowAbilityActivation'),
            ('engine/battle/move_effects/bug_bite.asm', 'BattleCommand_bugbite:', '\n\tcall CheckSubstituteOpp')]:
            with self.subTest(path=path):
                files = copy.deepcopy(action.source.authenticated_sources())
                self.assertIn(label, files[path]['text'])
                files[path]['text'] = files[path]['text'].replace(label, label + insertion, 1)
                with self.assertRaises(AssertionError):
                    action.validate_closure_followup_source(files, effects)

    def test_status_pp_cleanup_contracts_reject_branch_loss(self):
        cases = [
            ('engine/battle/effect_commands.asm', 'CanParalyzeTarget:', 'lb bc, ELECTRIC, ELECTRIC'),
            ('engine/battle/effect_commands.asm', 'CanStatusTarget:', 'call CheckSubstituteOpp'),
            ('engine/battle/move_effects/safeguard.asm', 'SafeCheckSafeguard:', 'cp INFILTRATOR'),
            ('engine/battle/core.asm', 'PreparePPRestore:', 'ld hl, wBattleMonPP'),
            ('engine/battle/core.asm', 'GetZeroPPMove:', 'and $3f'),
            ('engine/battle/core.asm', 'GetNonfullPPMove:', 'ld e, [hl]'),
            ('engine/battle/core.asm', 'GetNonfullPPMove:', 'jr nz, .got_nonfull_pp'),
            ('engine/battle/core.asm', 'LeppaRestorePP:', 'add 10'),
            ('engine/battle/core.asm', 'LeppaRestorePP:', 'bit SUBSTATUS_TRANSFORMED, a'),
            ('engine/items/item_effects.asm', 'PokeBallEffect:', 'jmp z, .SendToPC'),
            ('engine/battle/core.asm', 'PostBattleTasks::', 'call RestoreBattleItems'),
            ('engine/battle/core.asm', 'ExitBattle:', 'call PostBattleTasks'),
            ('engine/events/whiteout.asm', 'Script_Whiteout:', 'special HealParty'),
            ('engine/battle/move_effects/pay_day.asm', '', 'ld hl, wPayDayMoney + 2'),
            ('engine/battle/move_effects/rollout.asm', '', 'and SLP_MASK'),
            ('engine/battle/abilities.asm', 'DrySkinAbility:', 'ld hl, HPIsFullText'),
        ]
        for path, label, token in cases:
            with self.subTest(path=path, token=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                prefix, suffix = files[path]['text'].split(label, 1) if label else ('', files[path]['text'])
                files[path]['text'] = prefix + label + suffix.replace(token, 'nop', 1)
                with self.assertRaises((AssertionError, IndexError)):
                    action.validate_status_pp_cleanup_source(files)

    def test_leppa_rejects_masking_raw_pp_before_recovery(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/core.asm'
        prefix, suffix = files[path]['text'].split('LeppaRestorePP:', 1)
        suffix = suffix.replace('ld a, [hl]\n\tadd 10', 'ld a, [hl]\n\tand $3f\n\tadd 10', 1)
        files[path]['text'] = prefix + 'LeppaRestorePP:' + suffix
        with self.assertRaisesRegex(AssertionError, 'Leppa adds to packed PP'):
            action.validate_status_pp_cleanup_source(files)

    def test_ability_survival_contracts_reject_branch_loss(self):
        cases = [
            ('engine/battle/abilities.asm', 'ShedSkinAbility:', 'cp 1 + (30 percent)'),
            ('macros/data.asm', '', 'DEF percent EQUS "* $ff / 100"'),
            ('engine/battle/abilities.asm', 'WeatherRecoveryAbility:', 'call GetSixteenthMaxHP'),
            ('engine/battle/abilities.asm', 'SelectRandomStat:', 'ld a, 5'),
            ('engine/battle/abilities.asm', 'CheckNullificationAbilities:', 'cp LIGHTNING_ROD'),
            ('engine/battle/abilities.asm', 'CheckNullificationAbilities:', 'cp STATUS'),
            ('engine/battle/effect_commands.asm', 'CheckTypeMatchup:', 'jr z, .end'),
            ('engine/battle/abilities.asm', 'WeakArmorAbility:', 'ld b, $10 | SPEED'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_applydamage:', 'cp HELD_FOCUS_BAND'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_applydamage:', 'call GetOpponentAbilityAfterMoldBreaker'),
            ('data/items/attributes.asm', '', 'HELD_FOCUS_BAND, 30'),
        ]
        for path, label, token in cases:
            with self.subTest(path=path, token=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                prefix, suffix = files[path]['text'].split(label, 1) if label else ('', files[path]['text'])
                files[path]['text'] = prefix + label + suffix.replace(token, 'nop', 1)
                with self.assertRaises((AssertionError, IndexError)):
                    action.validate_ability_survival_source(files)

    def test_bulletproof_mold_breaker_exception_is_explicit(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        files['data/abilities/mold_breaker_suppressed_abilities.asm']['text'] += '\tdb BULLETPROOF\n'
        with self.assertRaises(AssertionError):
            action.validate_ability_survival_source(files)

    def test_move_state_contracts_reject_branch_loss(self):
        cases = [
            ('engine/battle/effect_commands.asm', 'IncreaseMetronomeCount:', 'call CheckUserIsCharging'),
            ('engine/battle/effect_commands.asm', 'IncreaseMetronomeCount:', 'cp 5'),
            ('engine/battle/effect_commands.asm', 'IncreaseMetronomeCount:', 'inc a ; cp STRUGGLE'),
            ('engine/battle/effect_commands.asm', 'BattleConsumePP:', 'call CheckUserIsCharging'),
            ('engine/battle/core.asm', 'MoveSelectionScreen:', 'ld [wPlayerSelectedMove], a'),
            ('engine/battle/core.asm', '.continue_rage\n', 'cp EFFECT_ENDURE'),
            ('engine/battle/core.asm', '.no_rage\n', 'ld [wEnemyProtectCount], a'),
            ('engine/battle/move_effects/protect.asm', 'ProtectChance:', 'call CheckOpponentWentFirst'),
            ('engine/battle/move_effects/roost.asm', '', 'ret z'),
            ('engine/battle/move_effects/roost.asm', '', 'ld a, NORMAL'),
            ('engine/battle/move_effects/transform.asm', '', 'ld bc, wBattleMonStructEnd - wBattleMonStats'),
            ('engine/battle/move_effects/magic_bounce.asm', '', 'call ResetTurn'),
            ('engine/battle/core.asm', 'SendOutPlayerMon:', 'ld [wPlayerSelectedMove], a'),
        ]
        for path, label, token in cases:
            with self.subTest(path=path, token=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                prefix, suffix = files[path]['text'].split(label, 1) if label else ('', files[path]['text'])
                files[path]['text'] = prefix + label + suffix.replace(token, 'nop', 1)
                with self.assertRaises((AssertionError, IndexError)):
                    action.validate_move_state_source(files)

    def test_switch_boundaries_reject_branch_loss(self):
        cases = [
            ('ForceDeferredSwitch:', 'farcall CheckAnyOtherAliveMons'),
            ('ForceDeferredSwitch:', 'call nz, PursuitSwitch'),
            ('ForceDeferredSwitch:', 'cp 1 << SWITCH_DEFERRED | 1 << SWITCH_TARGET | 1 << SWITCH_FORCED'),
            ('ForceDeferredSwitch:', 'call SpikesDamage_CheckMoldBreaker'),
            ('ForceDeferredSwitch:', 'call SpikesDamage\n'),
            ('FaintUserPokemon:', 'ld [wEnemyWrapCount], a'),
            ('NewBattleMonStatus:', 'res SUBSTATUS_CANT_RUN'),
        ]
        for label, token in cases:
            with self.subTest(label=label, token=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                path = 'engine/battle/core.asm'
                prefix, suffix = files[path]['text'].split(label, 1)
                files[path]['text'] = prefix + label + suffix.replace(token, 'nop', 1)
                with self.assertRaises((AssertionError, IndexError)):
                    action.validate_switch_boundaries_source(files)

    def test_faint_trap_and_wild_card_boundaries_are_explicit(self):
        for label, inserted in [('FaintUserPokemon:', '\n\tres SUBSTATUS_CANT_RUN, [hl]'),
                                ('ForceDeferredSwitch:', '\n\tld a, [wBattleMode]')]:
            with self.subTest(label=label):
                files = copy.deepcopy(action.source.authenticated_sources())
                path = 'engine/battle/core.asm'
                files[path]['text'] = files[path]['text'].replace(label, label + inserted, 1)
                with self.assertRaises(AssertionError):
                    action.validate_switch_boundaries_source(files)

    def test_action_boundaries_reject_lost_source_branches(self):
        cases = [
            ('engine/battle/effect_commands.asm', 'BattleCommand_rampage:', 'and SLP_MASK'),
            ('engine/battle/effect_commands.asm', 'HandleRampage:', 'call HasUserFainted'),
            ('engine/battle/effect_commands.asm', 'CantMove:', 'call z, HandleRampage_ConfuseUser'),
            ('engine/battle/move_effects/explosion.asm', '', 'cp DAMP'),
            ('engine/battle/move_effects/explosion.asm', '', 'farcall HandleHealingItems'),
            ('engine/battle/move_effects/explosion.asm', '', 'res SUBSTATUS_DESTINY_BOND'),
            ('engine/pokemon/learn.asm', '', 'ld [wPlayerEncoreCount], a'),
            ('engine/battle/evolve_battle.asm', '', 'inc hl ; skip Item'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_heal:', 'call IsOpponentLeafGuardActive'),
            ('engine/battle/effect_commands.asm', 'DoLeafGuardCheck:', 'call GetWeatherAfterOpponentUmbrella'),
            ('engine/battle/effect_commands.asm', 'BattleCommand_heal:', 'ld [hl], REST_SLEEP_TURNS + 1'),
            ('engine/battle/effect_commands.asm', 'CheckPowerHerb:', 'jr z, .chargeup'),
            ('engine/battle/move_effects/counter.asm', '', 'jr nz, .got_damage'),

        ]
        for path, label, token in cases:
            with self.subTest(path=path, token=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                prefix, suffix = files[path]['text'].split(label, 1) if label else ('', files[path]['text'])
                files[path]['text'] = prefix + label + suffix.replace(token, 'nop', 1)
                with self.assertRaises((AssertionError, IndexError)):
                    action.validate_action_boundaries(files)

    def test_persistence_and_choice_contracts_reject_branch_loss(self):
        cases = [
            ('home/battle.asm', 'SetBackupItem::', 'and a\n\tret nz'),
            ('home/battle.asm', 'SetBackupItem::', 'ld [hl], b'),
            ('home/battle.asm', 'RestoreBattleItems::', 'ld b, 7'),
            ('engine/battle/core.asm', 'PostBattleTasks:', 'call RestoreBattleItems'),
            ('engine/battle/effect_commands.asm', '_ConsumeUserItem::', 'cp BERRIES'),
            ('engine/battle/effect_commands.asm', '_ConsumeUserItem::', 'cp d'),
            ('engine/battle/move_effects/trick.asm', 'BattleCommand_trick:', 'call GetBackupItemAddr'),
            ('engine/battle/abilities.asm', 'HarvestAbility:', 'jmp SetBackupItem'),
            ('engine/battle/abilities.asm', 'PickupAbility:', 'jmp SetBackupItem'),
            ('engine/items/item_effects.asm', '', 'ld hl, wPartyBackupItems'),
            ('engine/battle/core.asm', 'SetChoiceLock:', 'and $f'),
            ('engine/battle/move_effects/attract.asm', 'CheckMentalHerb:', 'and $f0'),
            ('engine/battle/move_effects/encore_disable.asm', '', 'cp STRUGGLE'),
        ]
        for path, label, token in cases:
            with self.subTest(path=path, token=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                prefix, suffix = files[path]['text'].split(label, 1) if label else ('', files[path]['text'])
                files[path]['text'] = prefix + label + suffix.replace(token, 'nop', 1)
                with self.assertRaises((AssertionError, IndexError)):
                    action.validate_persistence_choice_source(files)

    def test_pickup_requires_wild_gate_and_opponent_history(self):
        for token, replacement in (('ld a, [wBattleMode]', 'nop'),
                                   ('call GetOpponentUsedItemAddr', 'call GetUsedItemAddr')):
            files = copy.deepcopy(action.source.authenticated_sources())
            path = 'engine/battle/abilities.asm'
            prefix, suffix = files[path]['text'].split('PickupAbility:', 1)
            files[path]['text'] = prefix + 'PickupAbility:' + suffix.replace(token, replacement, 1)
            with self.assertRaises(AssertionError):
                action.validate_recovery_source(files)

    def test_recovery_must_clear_used_history(self):
        for label in ('HarvestAbility:', 'PickupAbility:'):
            files = copy.deepcopy(action.source.authenticated_sources())
            path = 'engine/battle/abilities.asm'
            prefix, suffix = files[path]['text'].split(label, 1)
            files[path]['text'] = prefix + label + suffix.replace('xor a\n\tld [hl], a', 'nop', 1)
            with self.assertRaises(AssertionError):
                action.validate_recovery_source(files)

    def test_recovery_cannot_reset_unburden_or_consume_immediately(self):
        for label in ('HarvestAbility:', 'PickupAbility:', 'RegainItemByAbility:'):
            for instruction in ('res SUBSTATUS_UNBURDEN, [hl]', 'call ConsumeUserItem'):
                files = copy.deepcopy(action.source.authenticated_sources())
                path = 'engine/battle/abilities.asm'
                files[path]['text'] = files[path]['text'].replace(label, label + '\n\t' + instruction, 1)
                with self.assertRaises(AssertionError):
                    action.validate_recovery_source(files)

    def test_harvest_requires_source_weather_probability_and_berry_gate(self):
        for token in ('call GetWeatherAfterUserUmbrella', 'cp WEATHER_SUN', 'add a', 'cp BERRIES'):
            files = copy.deepcopy(action.source.authenticated_sources())
            path = 'engine/battle/abilities.asm'
            prefix, suffix = files[path]['text'].split('HarvestAbility:', 1)
            files[path]['text'] = prefix + 'HarvestAbility:' + suffix.replace(token, 'nop', 1)
            with self.assertRaises(AssertionError):
                action.validate_recovery_source(files)

    def test_recovery_history_tracks_the_current_party_slot(self):
        for token in ('call StackCallOpponentTurn', 'ld a, [wCurBattleMon]', 'ld a, [wCurOTMon]'):
            files = copy.deepcopy(action.source.authenticated_sources())
            path = 'home/battle.asm'
            prefix, suffix = files[path]['text'].split('GetOpponentUsedItemAddr::', 1)
            files[path]['text'] = prefix + 'GetOpponentUsedItemAddr::' + suffix.replace(token, 'xor a', 1)
            with self.assertRaises(AssertionError):
                action.validate_recovery_source(files)

    def test_recovered_berry_healing_must_follow_both_ability_passes(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/endturn.asm'
        text = files[path]['text'].replace('call HandleHealingItems', 'nop', 1)
        files[path]['text'] = text.replace('call HandleEndturnBlockB', 'call HandleHealingItems\n\tcall HandleEndturnBlockB', 1)
        with self.assertRaises(AssertionError):
            action.validate_recovery_source(files)

    def test_item_transfers_cannot_consume_or_change_unburden(self):
        for name in ('thief', 'knock_off'):
            for instruction in ('set SUBSTATUS_UNBURDEN, [hl]', 'res SUBSTATUS_UNBURDEN, [hl]',
                                'call ConsumeUserItem', 'call ConsumeOpponentItem'):
                with self.subTest(name=name, instruction=instruction):
                    files = copy.deepcopy(action.source.authenticated_sources())
                    files[f'engine/battle/move_effects/{name}.asm']['text'] += '\n\t' + instruction
                    with self.assertRaises(AssertionError):
                        action.validate_item_unburden_source(files)

    def test_pickpocket_must_use_source_thief(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/effect_commands.asm'
        prefix, suffix = files[path]['text'].split('; Pickpocket', 1)
        files[path]['text'] = prefix + '; Pickpocket' + suffix.replace('call BattleCommand_thief', 'nop', 1)
        with self.assertRaises(AssertionError):
            action.validate_item_unburden_source(files)

    def test_bug_bite_consumption_must_address_the_victim(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/effect_commands.asm'
        prefix, suffix = files[path]['text'].split('ConsumeStolenOpponentItem::', 1)
        files[path]['text'] = prefix + 'ConsumeStolenOpponentItem::' + suffix.replace('call StackCallOpponentTurn', 'nop', 1)
        with self.assertRaises(AssertionError):
            action.validate_item_unburden_source(files)

    def test_consumption_requires_effective_unburden_and_sets_the_flag(self):
        for token in ('call GetTrueUserAbility', 'cp UNBURDEN', 'set SUBSTATUS_UNBURDEN, [hl]'):
            with self.subTest(token=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                path = 'engine/battle/effect_commands.asm'
                prefix, suffix = files[path]['text'].split('_ConsumeUserItem::', 1)
                files[path]['text'] = prefix + '_ConsumeUserItem::' + suffix.replace(token, 'nop', 1)
                with self.assertRaises(AssertionError):
                    action.validate_item_unburden_source(files)

    def test_curse_requires_hidden_substitute_and_duplicate_guards(self):
        for token in ('call CheckHiddenOpponent', 'call CheckSubstituteOpp',
                      'BATTLE_VARS_SUBSTATUS1_OPP', 'bit SUBSTATUS_CURSE, [hl]',
                      'call GetHalfMaxHP', 'call UpdateUserInParty'):
            with self.subTest(token=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                path = 'engine/battle/move_effects/curse.asm'
                files[path]['text'] = files[path]['text'].replace(token, 'nop', 1)
                with self.assertRaises((AssertionError, ValueError)):
                    action.validate_curse_trick_source(files)

    def test_curse_substitute_requires_effective_infiltrator(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/effect_commands.asm'
        prefix, suffix = files[path]['text'].split('CheckSubstituteOpp:', 1)
        files[path]['text'] = prefix + 'CheckSubstituteOpp:' + suffix.replace('call GetTrueUserAbility', 'call GetUserAbility', 1)
        with self.assertRaises((AssertionError, ValueError)):
            action.validate_curse_trick_source(files)

    def test_curse_residual_and_hp_floor_are_authenticated(self):
        for path, label, before in (
                ('home/battle.asm', 'FloorBC::', 'inc c'),
                ('engine/battle/endturn.asm', 'HandleCurse:', 'call nz, PreventEndturnDamage'),
                ('engine/battle/endturn.asm', 'HandleCurse:', 'call GetQuarterMaxHP')):
            with self.subTest(token=before):
                files = copy.deepcopy(action.source.authenticated_sources())
                prefix, suffix = files[path]['text'].split(label, 1)
                files[path]['text'] = prefix + label + suffix.replace(before, 'nop', 1)
                with self.assertRaises((AssertionError, ValueError)):
                    action.validate_curse_trick_source(files)

    def test_trick_cannot_set_or_clear_unburden_or_consume_items(self):
        for instruction in ('set SUBSTATUS_UNBURDEN, [hl]', 'res SUBSTATUS_UNBURDEN, [hl]', 'call ConsumeUserItem'):
            with self.subTest(instruction=instruction):
                files = copy.deepcopy(action.source.authenticated_sources())
                files['engine/battle/move_effects/trick.asm']['text'] += '\n\t' + instruction + '\n'
                with self.assertRaises(AssertionError):
                    action.validate_curse_trick_source(files)

    def test_unburden_boost_cannot_require_an_empty_item_slot(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/abilities.asm'
        files[path]['text'] = files[path]['text'].replace('UnburdenAbility:', 'UnburdenAbility:\n\tcall GetUserItem', 1)
        with self.assertRaises(AssertionError):
            action.validate_curse_trick_source(files)

    def test_remaining_source_contracts_reject_removed_guards(self):
        for path, token in (
                ('transform', 'call ResetActorDisable'), ('transform', 'cp ARMOR_SUIT'),
                ('skill_swap', 'ld [wPlayerCudChewBerry], a'),
                ('conversion', 'farcall MoveSelectionScreen'),
                ('thief', 'call ReceiveItem'), ('bug_bite', 'cp HELD_NO_BUG_BITE'),
                ('bug_bite', 'call CheckPendingItemLoss')):
            with self.subTest(path=path, token=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                key = 'engine/battle/move_effects/'+path+'.asm'
                files[key]['text'] = files[key]['text'].replace(token, 'nop', 1)
                with self.assertRaises((AssertionError, ValueError)):
                    action.validate_remaining_source(files)

    def test_gene_and_mirror_must_precede_action_selection(self):
        for token in ('call HandleBerserkGene', 'farcall CheckMirrorHerb'):
            with self.subTest(checkpoint=token):
                files = copy.deepcopy(action.source.authenticated_sources())
                path = 'engine/battle/core.asm'
                prefix, turn = files[path]['text'].split('BattleTurn:', 1)
                turn = turn.replace(token, 'nop', 1)
                turn = turn.replace('call DetermineMoveOrder', 'call DetermineMoveOrder\n\t' + token, 1)
                files[path]['text'] = prefix + 'BattleTurn:' + turn
                with self.assertRaises((AssertionError, ValueError)):
                    action.validate_source(files)

    def test_gene_requires_consumption_success_and_confusion_guards(self):
        for before, after in (
                ('and a\n\tret nz', 'and a\n\tnop'),
                ('cp OWN_TEMPO\n\tret z', 'cp OWN_TEMPO\n\tnop'),
                ('bit SUBSTATUS_CONFUSED, [hl]\n\tret nz', 'bit SUBSTATUS_CONFUSED, [hl]\n\tnop'),
                ('and 1\n\tadd 2', 'and 3\n\tadd 2'),
                ('call SetFastestTurn', 'nop'),
                ('cp HELD_BERSERK_GENE', 'cp HELD_BERSERK_GENE\n\tcp GUARD_SAFEGUARD')):
            with self.subTest(mutation=before):
                files = copy.deepcopy(action.source.authenticated_sources())
                path = 'engine/battle/core.asm'
                prefix, gene = files[path]['text'].split('HandleBerserkGene:', 1)
                files[path]['text'] = prefix + 'HandleBerserkGene:' + gene.replace(before, after, 1)
                with self.assertRaises((AssertionError, ValueError)):
                    action.validate_source(files)

    def test_sketch_rejects_wrong_history_and_removed_failure_guards(self):
        for before, after in (
                ('call ClearLastMove', 'nop'),
                ('ld a, [wLinkMode]', 'xor a'),
                ('call CheckSubstituteOpp', 'xor a'),
                ('bit SUBSTATUS_TRANSFORMED', 'bit SUBSTATUS_CONFUSED'),
                ('BATTLE_VARS_LAST_COUNTER_MOVE_OPP', 'BATTLE_VARS_LAST_MOVE_OPP'),
                ('inc a ; cp STRUGGLE', 'nop'),
                ('cp b\n\tjr z, .pop_hl_and_fail', 'cp b\n\tnop')):
            with self.subTest(mutation=before):
                files = copy.deepcopy(action.source.authenticated_sources())
                path = 'engine/battle/move_effects/sketch.asm'
                files[path]['text'] = files[path]['text'].replace(before, after, 1)
                with self.assertRaises((AssertionError, ValueError)):
                    action.validate_source(files)

    def test_sketch_keeps_base_pp_party_guard_and_lock_remapping(self):
        for before, after in (
                ('ld hl, Moves + MOVE_PP', 'ld hl, Moves + MOVE_POWER'),
                ('cp SKETCH\n\tjr nz, .finished_overwriting_sketch', 'cp SKETCH\n\tnop'),
                ('farcall GetDisableEncoreMoves', 'nop'),
                ('farcall SetDisableEncoreMoves', 'nop')):
            with self.subTest(mutation=before):
                files = copy.deepcopy(action.source.authenticated_sources())
                path = 'engine/battle/move_effects/sketch.asm'
                files[path]['text'] = files[path]['text'].replace(before, after, 1)
                with self.assertRaises((AssertionError, ValueError)):
                    action.validate_source(files)

    def test_called_move_text_cannot_restore_counter_move_history(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/used_move_text.asm'
        files[path]['text'] = files[path]['text'].replace('dec a\n\tjr z, .charging', 'dec a\n\tnop', 1)
        with self.assertRaises((AssertionError, ValueError)):
            action.validate_source(files)

    def test_authenticated_source_and_complete_inventory(self):
        self.assertEqual(set(action.build()), {action.MANIFEST, action.ROOT/'src/data/polished_entry_abilities.h'})

    def test_reversed_faint_ability_order_is_rejected(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/abilities.asm'
        files[path]['text'] = files[path]['text'].replace('call _RunFaintUserAbilities', 'call TEMP').replace('call _RunFaintOpponentAbilities', 'call _RunFaintUserAbilities').replace('call TEMP', 'call _RunFaintOpponentAbilities')
        with self.assertRaises(AssertionError):
            action.validate_source(files)

    def test_last_switch_wins_is_rejected(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/effect_commands.asm'
        files[path]['text'] = files[path]['text'].replace('jr nz, .done\n\tld [hl], a', 'jr z, .done\n\tld [hl], a')
        with self.assertRaises(AssertionError):
            action.validate_source(files)

    def test_pack_reservation_must_not_require_a_replacement(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/effect_commands.asm'
        files[path]['text'] = files[path]['text'].replace('CheckWhiteHerbEjectPack:', 'CheckWhiteHerbEjectPack:\n\tcall CheckAnyOtherAliveMons')
        with self.assertRaises(AssertionError):
            action.validate_source(files)

    def test_reversed_stat_herb_order_is_rejected(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/effect_commands.asm'
        files[path]['text'] = files[path]['text'].replace('call CheckWhiteHerbEjectPack\n\tcall CheckMirrorHerb', 'call CheckMirrorHerb\n\tcall CheckWhiteHerbEjectPack')
        with self.assertRaises(AssertionError):
            action.validate_source(files)

    def test_magic_guard_in_aftermath_is_rejected(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/abilities.asm'
        files[path]['text'] = files[path]['text'].replace('AftermathAbility:', 'AftermathAbility:\n\tcp MAGIC_GUARD')
        with self.assertRaises(AssertionError):
            action.validate_source(files)

    def test_intimidate_mirror_must_precede_white_herb(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/abilities.asm'
        files[path]['text'] = files[path]['text'].replace('farcall CheckMirrorHerb\n\tfarjp CheckStatHerbsAfterIntimidate', 'farcall CheckStatHerbsAfterIntimidate\n\tfarjp CheckMirrorHerb')
        with self.assertRaises((AssertionError, ValueError)):
            action.validate_source(files)

    def test_entry_pack_preserves_the_outer_reservation(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/effect_commands.asm'
        files[path]['text'] = files[path]['text'].replace('.no_deferred_switch\n\tpop hl\n\tpop af\n\tld [hl], a', '.no_deferred_switch\n\tpop hl\n\tpop af\n\txor a\n\tld [hl], 0')
        with self.assertRaises((AssertionError, ValueError)):
            action.validate_source(files)

    def test_gas_deactivation_must_prevent_nested_pack(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/effect_commands.asm'
        files[path]['text'] = files[path]['text'].replace('inc a\n\tjr z, .no_deferred_switch', 'inc a\n\tjr nz, .no_deferred_switch')
        with self.assertRaises(AssertionError):
            action.validate_source(files)


    def test_each_ability_requires_its_immediate_mirror_checkpoint(self):
        for label in ('DownloadAbility:', 'TanglingHairAbility:', 'StatUpAbility:',
                      'WeakArmorAbility:', 'MoodyAbility:', 'AngerPointAbility:'):
            with self.subTest(ability=label):
                files = copy.deepcopy(action.source.authenticated_sources())
                path = 'engine/battle/abilities.asm'
                prefix, suffix = files[path]['text'].split(label, 1)
                files[path]['text'] = prefix + label + suffix.replace('farjp CheckMirrorHerb', 'ret', 1)
                with self.assertRaises(AssertionError):
                    action.validate_source(files)

    def test_fainted_mirror_holder_is_rejected(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/effect_commands.asm'
        prefix, suffix = files[path]['text'].split('CheckMirrorHerb:', 1)
        files[path]['text'] = prefix + 'CheckMirrorHerb:' + suffix.replace('call nz, HasOpponentFainted', 'nop', 1)
        with self.assertRaises((AssertionError, ValueError)):
            action.validate_source(files)

    def test_pending_copies_do_not_survive_switching(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/core.asm'
        prefix, suffix = files[path]['text'].split('ResetMirrorHerb:', 1)
        files[path]['text'] = prefix + 'ResetMirrorHerb:' + suffix.replace('rst ByteFill', 'ret', 1)
        with self.assertRaises((AssertionError, ValueError)):
            action.validate_source(files)

    def test_generic_endturn_stat_herbs_are_rejected(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        files['engine/battle/endturn.asm']['text'] += '\n\tfarcall CheckStatHerbs\n'
        with self.assertRaises(AssertionError):
            action.validate_source(files)


    def test_pending_boosts_saturate_instead_of_wrapping(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/stats.asm'
        files[path]['text'] = files[path]['text'].replace('or $f0', 'and $0f')
        with self.assertRaises((AssertionError, ValueError)):
            action.validate_source(files)

    def test_pending_boosts_are_recorded_without_requiring_a_herb(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/stats.asm'
        files[path]['text'] = files[path]['text'].replace('.queue_mirror_herb\n', '.queue_mirror_herb\n\tcall GetOpponentItem\n', 1)
        with self.assertRaises(AssertionError):
            action.validate_source(files)

    def test_failed_mirror_copy_does_not_consume_the_item(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/effect_commands.asm'
        prefix, suffix = files[path]['text'].split('CheckMirrorHerb:', 1)
        files[path]['text'] = prefix + 'CheckMirrorHerb:' + suffix.replace('call nz, ConsumeUserItem', 'call ConsumeUserItem', 1)
        with self.assertRaises((AssertionError, ValueError)):
            action.validate_source(files)

    def test_healing_sweep_copies_stat_berries_before_confusion_healing(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/endturn.asm'
        files[path]['text'] = files[path]['text'].replace('farcall CheckMirrorHerb ; for stat boost berry', 'nop')
        with self.assertRaises((AssertionError, ValueError)):
            action.validate_source(files)


if __name__ == '__main__':
    unittest.main()
