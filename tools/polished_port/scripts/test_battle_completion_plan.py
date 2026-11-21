#!/usr/bin/env python3
"""Reject omitted ownership and reordered/omitted live lifecycle phases."""
import copy
import unittest
import build_battle_completion_plan as plan


class CompletionPlanTest(unittest.TestCase):
    def test_three_suites_cover_all_obligations(self):
        plan.validate_ownership(plan.SUITES)
        self.assertEqual(set(plan.build()), {plan.MANIFEST, plan.LIFECYCLE})

    def test_missing_duplicate_or_fourth_suite_is_rejected(self):
        for mode in range(3):
            suites = copy.deepcopy(plan.SUITES)
            if mode == 0:
                suites[0]['domains'].pop()
            elif mode == 1:
                suites[1]['domains'].append('field')
            else:
                suites.append(dict(id=4, domains=[], contracts=['Unassigned leftovers']))
            with self.assertRaises(AssertionError):
                plan.validate_ownership(suites)

    def test_removed_checkpoint_or_early_healing_is_rejected(self):
        src = plan.source.authenticated_sources()['engine/battle/endturn.asm']['text']
        target = (plan.ROOT / 'src/battle_end_turn.c').read_text()
        constants = (plan.ROOT / 'include/constants/battle_end_turn.h').read_text()
        start = target.index('sPolishedEndTurnOrder[] =')
        prefix, schedule = target[:start], target[start:]
        for changed in [schedule.replace('ENDTURN_POISON, ENDTURN_POLISHED_CHECKPOINT,', 'ENDTURN_POISON,', 1),
                        schedule.replace('ENDTURN_POLISHED_LEPPA, ENDTURN_POLISHED_HEALING_ITEMS,',
                                         'ENDTURN_POLISHED_HEALING_ITEMS, ENDTURN_POLISHED_LEPPA,', 1),
                        schedule.replace('ENDTURN_AQUA_RING, ', '', 1)]:
            with self.assertRaises(AssertionError):
                plan.validate_schedule(src, prefix+changed, constants)


if __name__ == '__main__':
    unittest.main()
