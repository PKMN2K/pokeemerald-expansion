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

    def test_magic_guard_in_aftermath_is_rejected(self):
        files = copy.deepcopy(action.source.authenticated_sources())
        path = 'engine/battle/abilities.asm'
        files[path]['text'] = files[path]['text'].replace('AftermathAbility:', 'AftermathAbility:\n\tcp MAGIC_GUARD')
        with self.assertRaises(AssertionError):
            action.validate_source(files)


if __name__ == '__main__':
    unittest.main()
