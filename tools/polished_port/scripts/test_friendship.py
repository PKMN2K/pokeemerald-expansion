#!/usr/bin/env python3
"""Independent boundary checks for imported source happiness and growth."""
import json
import unittest
import build_friendship as friendship


class FriendshipTest(unittest.TestCase):
    def test_nonnegative_zero_and_saturation(self):
        self.assertEqual(friendship.reference_change(160, 0, 1, 1), 162)
        self.assertEqual(friendship.reference_change(254, 0, 1, 1), 255)
        self.assertEqual(friendship.reference_change(250, 6, 1, 1), 255)
        self.assertEqual(friendship.reference_change(99, 3, 1, 1), 104)

    def test_negative_branch_does_not_receive_extras(self):
        self.assertEqual(friendship.reference_change(160, -10, 1, 1), 150)
        self.assertEqual(friendship.reference_change(4, -5, 1, 1), 0)
        self.assertEqual(friendship.reference_change(0, -1, 1, 1), 0)

    def test_authenticated_tables_and_bosses(self):
        report = json.loads(friendship.build()[friendship.MANIFEST])
        self.assertEqual(report['thresholds'], [100, 160])
        self.assertEqual(report['events']['HAPPINESS_GAINLEVEL'], [3, 2, 0])
        self.assertEqual(report['events']['HAPPINESS_GAINLEVELATHOME'], [6, 4, 0])
        self.assertEqual(report['events']['HAPPINESS_GYMBATTLE'], [5, 3, 0])
        bosses = report['registered_boss_bindings']
        self.assertEqual([k for k,v in bosses.items() if v], ['TRAINER_POLISHED_FALKNER', 'TRAINER_POLISHED_GIOVANNI1'])
        self.assertFalse(bosses['TRAINER_POLISHED_SPROUT_LI'])

    def test_scoped_reproducible_reference(self):
        result = friendship.build()
        report = json.loads(result[friendship.MANIFEST])
        self.assertEqual(report['source_commit'], friendship.source.source.PIN)
        self.assertEqual(report['reference_cases'], 23552)
        self.assertEqual(report['growth_vectors'], 400)
        self.assertFalse(report['complete_behavior_parity'])
        self.assertTrue(report['remaining'])


if __name__ == '__main__':
    unittest.main()
