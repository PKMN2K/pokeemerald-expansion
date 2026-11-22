#!/usr/bin/env python3
"""Reject changes that invalidate authenticated action ordering contracts."""
import copy
import unittest
import build_action_resolution as action


class ActionResolutionTest(unittest.TestCase):
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
                ('cp b\n\tjr z, .pop_hl_and_fail', 'cp b\n\tnop'),
                ('call ClearLastMove', 'call ClearLastMove\n\tcp INFILTRATOR')):
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
        self.assertEqual(set(action.build()), {action.MANIFEST})

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
