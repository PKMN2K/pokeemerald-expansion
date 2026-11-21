#!/usr/bin/env python3
"""Reject changes that invalidate authenticated action ordering contracts."""
import copy
import unittest
import build_action_resolution as action


class ActionResolutionTest(unittest.TestCase):
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


if __name__ == '__main__':
    unittest.main()
