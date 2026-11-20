#!/usr/bin/env python3
"""Independent boundary checks for the source escape contract."""
import json
import unittest

import build_escape as escape


class EscapeTest(unittest.TestCase):
    def test_inclusive_byte_comparison_and_first_attempt(self):
        self.assertEqual(escape.threshold(31, 100, 0), 40)
        self.assertEqual(escape.threshold(31, 103, 0), 40)
        self.assertEqual(escape.threshold(31, 104, 0), 39)
        self.assertEqual(escape.threshold(31, 100, 1), 70)

    def test_word_and_divisor_wrapping(self):
        self.assertEqual(escape.threshold(1, 1024, 0), 256)
        self.assertEqual(escape.threshold(1, 1028, 0), 33)
        self.assertEqual(escape.threshold(2048, 4100, 0), 1)
        self.assertEqual(escape.threshold(2049, 4100, 0), 33)

    def test_guarantees_and_counter_overflow(self):
        for attempts in range(256):
            self.assertEqual(escape.threshold(100, 100, attempts), 256)
            self.assertEqual(escape.threshold(1, 3, attempts), 256)
        self.assertEqual(escape.threshold(1, 1000, 0), 1)
        self.assertEqual(escape.threshold(1, 1000, 8), 241)
        self.assertEqual(escape.threshold(1, 1000, 9), 256)
        self.assertEqual(escape.threshold(1, 1000, 255), 256)

    def test_authentication_and_scoped_evidence(self):
        result = escape.build()
        manifest = json.loads(result[escape.MANIFEST])
        self.assertEqual(manifest['vectors'], 5184)
        self.assertEqual(manifest['source_commit'], escape.source.source.PIN)
        self.assertFalse(manifest['complete_behavior_parity'])
        self.assertTrue(manifest['remaining'])


if __name__ == '__main__':
    unittest.main()
