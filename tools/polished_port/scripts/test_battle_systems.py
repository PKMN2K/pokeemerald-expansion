#!/usr/bin/env python3
"""Independent pinned-source vectors and negative controls for shared systems."""
import json
import unittest
from unittest.mock import patch

import build_battle_systems as systems
import build_trainer_attributes as trainers


class BattleSystemsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.systems = json.loads(systems.build()[systems.ROOT/'tools/polished_port/manifests/battle_systems.json'])
        cls.trainers = json.loads(trainers.build()[trainers.MANIFEST])
        cls.parties = {r['target_trainer']: r for r in cls.trainers['bindings']}

    def test_source_ownership_keeps_cosmetics_together_and_variants_separate(self):
        indices = self.systems['ownership_indices']
        self.assertEqual(len(indices), 391)
        self.assertEqual(len(set(indices.values())), 335)
        self.assertEqual(self.systems['counted_species'], 289)
        self.assertEqual(indices['SPECIES_UNOWN'], indices['SPECIES_UNOWN_B'])
        self.assertNotEqual(indices['SPECIES_RATTATA'], indices['SPECIES_RATTATA_ALOLA'])
        self.assertEqual(max(indices.values()), 337)

    def test_current_trainer_roster_and_class_defaults(self):
        self.assertEqual(len(self.trainers['classes']), 147)
        self.assertEqual(len(self.parties), 21)
        self.assertEqual(sum(len(r['mons']) for r in self.parties.values()), 43)
        lyra = self.parties['TRAINER_POLISHED_LYRA_CYNDAQUIL']['mons'][0]
        self.assertEqual(lyra['ivs'], [22]*6)
        self.assertEqual(lyra['evs'], [0]*6)
        self.assertEqual((lyra['ability_slot'], lyra['gender']), (0, 'FEMALE'))
        for party in self.parties.values():
            self.assertTrue(all(mon['friendship'] == 160 for mon in party['mons']))

    def test_custom_personality_and_party_wide_ev_flags(self):
        joey = self.parties['TRAINER_POLISHED_JOEY']['mons'][0]
        self.assertEqual(joey['ivs'], [30]*6)
        self.assertEqual((joey['nature'], joey['ability']), ('NATURE_ADAMANT', 'ABILITY_GUTS'))
        finch = self.parties['TRAINER_POLISHED_FINCH']
        self.assertEqual(finch['items'], ['ITEM_FULL_HEAL', 'ITEM_MAX_POTION'])
        self.assertEqual([m['nature'] for m in finch['mons']], ['NATURE_RELAXED', 'NATURE_RASH'])
        self.assertEqual([m['evs'] for m in finch['mons']], [[12, 0, 0, 0, 0, 0]]*2)
        falkner = self.parties['TRAINER_POLISHED_FALKNER']
        self.assertEqual(falkner['base_reward'], 25)
        self.assertEqual([m['evs'] for m in falkner['mons']], [[32, 0, 0, 0, 0, 0]]*3)
        self.assertEqual([m['ability_slot'] for m in falkner['mons']], [1, 0, 1])

    def test_a_registered_trainer_cannot_silently_lose_its_source_binding(self):
        missing = dict(trainers.BINDINGS)
        missing.pop('FALKNER')
        with patch.object(trainers, 'BINDINGS', missing), self.assertRaises(AssertionError):
            trainers.build()

    def test_source_systems_cannot_be_reported_as_complete_from_vectors(self):
        self.assertFalse(self.systems['complete_behavior_parity'])
        self.assertTrue(self.systems['remaining'])
        self.assertIn('source AI', self.trainers['remaining'])
        self.assertEqual(self.systems['whiteout_base_payouts'], [8, 16, 24, 36, 48, 64, 80, 100, 120])


if __name__ == '__main__':
    unittest.main()
