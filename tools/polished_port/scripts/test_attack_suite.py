#!/usr/bin/env python3
"""Prevent incomplete or skipped attack runs from being reported as passing."""
import unittest

import build_attack_suite as vectors
import run_attack_suite as runner


class AttackSuiteTest(unittest.TestCase):
    def test_every_source_identity_and_effect(self):
        _, moves, scripts = vectors.inventory()
        self.assertEqual([m['source_id'] for m in moves], list(range(1, 256)))
        self.assertEqual(len({m['target_move'] for m in moves}), 255)
        self.assertEqual(len({m['source_effect'] for m in moves}), 133)
        self.assertEqual(moves[248]['target_move'], 'MOVE_BRICK_BREAK')
        self.assertEqual(moves[248]['source_constant'], 'ROCK_SMASH')
        for move in moves:
            self.assertEqual(scripts[move['source_effect']]['commands'][-1], 'endmove')
            self.assertFalse(move['complete_behavior_parity'])

    def test_missing_failed_skipped_and_expected_failures_cannot_pass(self):
        for log in ['', 'Tests PASSED: 0\nTests TOTAL: 0',
                    'Tests PASSED: 1\nTests TOTAL: 2',
                    'Tests FAILED: 1\nTests PASSED: 2\nTests TOTAL: 2',
                    'Tests EXPECT_FAILING: 1\nTests PASSED: 1\nTests TOTAL: 2']:
            with self.subTest(log=log), self.assertRaises(AssertionError):
                runner.summary(log, 2)

    def test_valid_executed_summary(self):
        self.assertEqual(runner.summary('\x1b[32mTests PASSED: 1,234\x1b[0m\nTests TOTAL: 1,234', 1234)['PASSED'], 1234)

    def test_catalog_includes_unit_tests_as_well_as_battle_tests(self):
        catalog = runner.test_catalog()
        self.assertTrue(any(t['kind'] == 'TEST' and t['name'].startswith('Polished move PP') for t in catalog))
        self.assertTrue(any(t['kind'] == 'SINGLE_BATTLE_TEST' and t['name'].startswith('Polished attack suite') for t in catalog))
        self.assertTrue(any(t['name'].startswith('AI thinking time') for t in catalog))

    def test_source_stat_vectors_are_not_inferred_from_target_tables(self):
        _, moves, _ = vectors.inventory()
        smash = next(m for m in moves if m['source_move'] == 'SHELL_SMASH')
        self.assertEqual({c['stat']: c['stages'] for c in smash['stat_changes']},
                         {'STAT_ATK': 2, 'STAT_SPATK': 2, 'STAT_SPEED': 2, 'STAT_DEF': -1, 'STAT_SPDEF': -1})


if __name__ == '__main__':
    unittest.main()
