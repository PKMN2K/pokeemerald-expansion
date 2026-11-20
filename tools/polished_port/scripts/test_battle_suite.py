#!/usr/bin/env python3
"""Negative controls for omitted source code and false parity certification."""
import base64
import copy
import gzip
import json
import unittest

import build_battle_suite as suite
import run_attack_suite as execution


class BattleSuiteTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.archive = json.loads(gzip.decompress(base64.b64decode(suite.ARCHIVE.read_bytes())))
        cls.outputs = suite.build()
        cls.report = json.loads(cls.outputs[suite.MANIFEST])

    def test_all_battle_domains_and_dependencies_are_registered(self):
        self.assertEqual(len(self.report['domains']), 15)
        paths = {f['path'] for f in self.report['source_files']}
        for path in ['engine/battle/core.asm', 'engine/battle/endturn.asm', 'engine/battle/ai/scoring.asm',
                     'engine/items/poke_balls.asm', 'engine/pokemon/experience.asm', 'home/random.asm',
                     'engine/events/battle_tower/battle_tower.asm', 'data/trainers/parties.asm']:
            self.assertIn(path, paths)
        self.assertEqual(self.report['source_moves'], 255)
        self.assertEqual(self.report['source_effects'], 133)
        self.assertEqual(len(self.report['source_commands']), 129)

    def test_deleted_source_blob_cannot_silently_reduce_coverage(self):
        damaged = copy.deepcopy(self.archive)
        damaged['blobs'].pop(next(iter(damaged['blobs'])))
        with self.assertRaises(KeyError):
            suite.authenticated_sources(damaged)

    def test_changed_source_blob_fails_git_authentication(self):
        damaged = copy.deepcopy(self.archive)
        key = next(iter(damaged['blobs']))
        damaged['blobs'][key] = base64.b64encode(b'Wrong source code\n').decode()
        with self.assertRaises(AssertionError):
            suite.authenticated_sources(damaged)

    def test_changed_tree_or_commit_cannot_authorize_a_different_source(self):
        for kind in ('trees', 'commit'):
            damaged = copy.deepcopy(self.archive)
            if kind == 'trees':
                damaged['trees'][next(iter(damaged['trees']))] = base64.b64encode(b'').decode()
            else:
                damaged['commit'] = base64.b64encode(b'tree invalid\n').decode()
            with self.subTest(kind=kind), self.assertRaises(AssertionError):
                suite.authenticated_sources(damaged)

    def test_source_labels_and_conditional_branches_are_retained(self):
        blocks = suite.source_blocks('First::\n.local\n jr z, .local\nif DEF(FAITHFUL)\nSecond:\n ret\nelse\nThird:\n ret\nendc\n')
        self.assertEqual([b['label'] for b in blocks], ['First', 'Second', 'Third'])
        self.assertEqual(blocks[0]['branch_sites'], 1)
        self.assertTrue(all(b['behavior'] == 'unverified' for b in blocks))

    def test_all_existing_tests_are_retained_and_ai_is_not_source_choice_evidence(self):
        self.assertEqual(self.report['regression_tests'], execution.test_catalog())
        self.assertEqual(sum(t['name'].startswith('AI thinking time') for t in self.report['regression_tests']), 6)
        ai = next(d for d in self.report['domains'] if d['id'] == 'ai')
        self.assertEqual(ai['status'], 'open')
        self.assertIn('performance only', ai['obligations'][0])

    def test_passing_regressions_do_not_close_unverified_behavior(self):
        candidate = copy.deepcopy(self.report)
        candidate.update(execution='passed', complete_behavior_parity=True)
        with self.assertRaises(AssertionError):
            suite.require_complete(candidate)
        # Even relabeling every domain cannot hide missing block/effect evidence.
        for domain in candidate['domains']:
            domain['status'] = 'verified'
        with self.assertRaises(AssertionError):
            suite.require_complete(candidate)

    def test_generated_inventory_cannot_drop_a_file_label_or_test(self):
        generated = self.outputs[suite.MANIFEST]
        for key in ('source_files', 'regression_tests'):
            altered = copy.deepcopy(self.report)
            altered[key].pop()
            self.assertNotEqual(json.dumps(altered, indent=2)+'\n', generated)
        altered = copy.deepcopy(self.report)
        next(f for f in altered['source_files'] if f['blocks'])['blocks'].pop()
        self.assertNotEqual(json.dumps(altered, indent=2)+'\n', generated)


if __name__ == '__main__':
    unittest.main()
