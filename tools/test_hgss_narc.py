#!/usr/bin/env python3
"""Standard-library regression tests for HGSS NARC inspection/extraction.

Run from repository root:
    python3 -m unittest discover -s tools -p 'test_hgss_narc.py' -v
"""
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import unittest

from inspect_hgss_narc import inspect
from extract_hgss_narc import extract


def block(tag, payload):
    return tag + struct.pack("<I", 8 + len(payload)) + payload


def make_narc(members):
    payload = b""
    offsets = []
    for member in members:
        start = len(payload)
        payload += member
        offsets.append((start, len(payload)))
    btaf = block(b"BTAF", struct.pack("<HH", len(members), 0)
                 + b"".join(struct.pack("<II", *entry) for entry in offsets))
    btnf = block(b"BTNF", struct.pack("<IHH", 8, 0, 1))
    gmif = block(b"GMIF", payload)
    body = btaf + btnf + gmif
    return b"NARC" + struct.pack("<HHIHH", 0xFFFE, 0x0100, 16 + len(body), 16, 3) + body


class NarcToolsTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.archive = self.base / "sample.narc"
        self.members = [b"RGCN" + b"\x00" * 12, b"RLCN" + b"\x01" * 8, b"unknown"]
        self.archive.write_bytes(make_narc(self.members))

    def test_inventory_identifies_members(self):
        info = inspect(self.archive)
        self.assertEqual(info["member_count"], 3)
        self.assertEqual(info["members"][0]["format_hint"], "NCGR graphics")
        self.assertEqual(info["members"][1]["format_hint"], "NCLR palette")
        self.assertEqual(info["members"][2]["format_hint"], "unknown")

    def test_lossless_extract_and_checksums(self):
        output = self.base / "out"
        self.assertEqual(extract(self.archive, output), 3)
        manifest = json.loads((output / "manifest.json").read_text())
        self.assertEqual(manifest["archive_sha256"],
                         hashlib.sha256(self.archive.read_bytes()).hexdigest())
        for i, original in enumerate(self.members):
            entry = manifest["members"][i]
            extracted = (output / entry["file"]).read_bytes()
            self.assertEqual(extracted, original)
            self.assertEqual(entry["sha256"], hashlib.sha256(original).hexdigest())

    def test_does_not_overwrite_existing_output(self):
        output = self.base / "out"
        output.mkdir()
        sentinel = output / "preserve.txt"
        sentinel.write_text("keep")
        with self.assertRaises(ValueError):
            extract(self.archive, output)
        self.assertEqual(sentinel.read_text(), "keep")

    def test_rejects_wrong_magic(self):
        data = bytearray(self.archive.read_bytes())
        data[0:4] = b"FAKE"
        self.archive.write_bytes(data)
        with self.assertRaises(ValueError):
            inspect(self.archive)

    def test_rejects_bad_member_bounds(self):
        data = bytearray(self.archive.read_bytes())
        # NARC 16-byte header + BTAF 8-byte chunk header + 4-byte count header
        struct.pack_into("<II", data, 28, 0, 0xFFFFFFFF)
        self.archive.write_bytes(data)
        with self.assertRaises(ValueError):
            inspect(self.archive)

    def test_rejects_truncated_archive(self):
        self.archive.write_bytes(self.archive.read_bytes()[:-1])
        with self.assertRaises(ValueError):
            inspect(self.archive)


if __name__ == "__main__":
    unittest.main()
