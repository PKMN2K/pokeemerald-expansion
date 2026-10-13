#!/usr/bin/env python3
"""Regression checks for real Polished Crystal source bytes and Emerald bank split."""
import base64
from pathlib import Path
import struct

from convert_johto_traditional_blocks import BLOCK_COUNT, convert
from extract_johto_collision import extract
from split_johto_metatile_banks import (
    PRIMARY_BYTES, SECONDARY_BYTES, BYTES_PER_METATILE, manifest,
    source_block_address, split_geometry,
)

ROOT = Path(__file__).resolve().parents[3]
FIXTURES = ROOT / "tools/polished_port/fixtures"
ORDER = ("top_left", "top_right", "bottom_left", "bottom_right")


def load_binary_fixture(stem):
    fixture = FIXTURES / f"johto_traditional_{stem}.bin.b64"
    encoded = "".join(line.strip() for line in fixture.read_text().splitlines()
                      if line.strip() and not line.startswith("#"))
    return base64.b64decode(encoded, validate=True)


def expected_entry(tile, attr):
    return (tile | (0x100 if attr & 0x08 else 0)
            | ((attr & 0x07) << 12)
            | (0x0400 if attr & 0x20 else 0)
            | (0x0800 if attr & 0x40 else 0))


def verify_source_geometry(tiles, attrs, converted):
    for block in range(BLOCK_COUNT):
        start = block * 16
        for slot, (by, bx) in enumerate(((0, 0), (0, 2), (2, 0), (2, 2))):
            # Source 8x8 positions preserved in TL, TR, BL, BR order.
            indices = [start + (by + y) * 4 + bx + x
                       for y in (0, 1) for x in (0, 1)]
            expected = tuple(expected_entry(tiles[i], attrs[i]) for i in indices)
            offset = (block * 4 + slot) * BYTES_PER_METATILE
            assert struct.unpack_from("<4H", converted, offset) == expected
            assert struct.unpack_from("<4H", converted, offset + 8) == (0, 0, 0, 0)


def main():
    assert BLOCK_COUNT == 253
    meta = load_binary_fixture("metatiles")
    attrs = load_binary_fixture("attributes")
    assert len(meta) == len(attrs) == 253 * 16 == 4048
    assert {a & 7 for a in attrs} == set(range(7))
    assert {bool(a & 8) for a in attrs} == {False, True}
    assert sum(bool(a & 0x80) for a in attrs) == 38  # GBC priority, not yet converted
    assert sum(bool(a & 0x20) for a in attrs) == 65
    assert not any(a & 0x40 for a in attrs)
    converted = convert(meta, attrs)
    verify_source_geometry(meta, attrs, converted)

    primary, secondary = split_geometry(converted)
    assert len(primary) == PRIMARY_BYTES == 8192
    assert len(secondary) == SECONDARY_BYTES == 8000
    assert primary + secondary == converted
    assert BYTES_PER_METATILE == 16

    assert source_block_address(127, "bottom_right") == {
        "bank": "primary", "map_metatile_id": 511, "bank_local_index": 511}
    assert source_block_address(128, "top_left") == {
        "bank": "secondary", "map_metatile_id": 512, "bank_local_index": 0}
    assert source_block_address(252, "bottom_right") == {
        "bank": "secondary", "map_metatile_id": 1011, "bank_local_index": 499}

    record = manifest()
    assert record["status"] == "staging_only_not_registered"
    assert record["source_blocks"] == 253
    assert record["primary_metatiles"] == 512
    assert record["secondary_source_metatiles"] == 500
    assert record["unused_secondary_map_metatile_ids"] == list(range(1012, 1024))
    addresses = [
        item["map_metatile_id"] for block in record["block_address"]
        for item in block["quadrants"].values()
    ]
    assert sorted(addresses) == list(range(1012))

    source = (FIXTURES / "johto_traditional_collision.asm").read_text(encoding="utf-8")
    source_blocks = extract(source)
    assert len(source_blocks) == BLOCK_COUNT
    for block in source_blocks:
        for i, q in enumerate(ORDER):
            assert source_block_address(block["block"], q)["map_metatile_id"] == block["block"] * 4 + i

    for invalid in (b"", converted[:-2], converted + b"\x00\x00"):
        try:
            split_geometry(invalid)
        except ValueError:
            pass
        else:
            raise AssertionError("invalid length accepted")
    try:
        convert(meta + bytes(48), attrs + bytes(48))
    except ValueError:
        pass
    else:
        raise AssertionError("invented 256-source-block table accepted")

    for invalid_block in (-1, 253, 255):
        try:
            source_block_address(invalid_block, "top_left")
        except ValueError:
            pass
        else:
            raise AssertionError(f"unavailable block {invalid_block} accepted")

    print("PASS: authentic 253 source blocks -> 512 primary + 500 secondary metatiles")
    print("PASS: 1012 original metatiles preserved; 12 source-absent IDs not invented")
    print("REVIEW: seven source palette IDs and 38 GBC priority flags need live mapping")


if __name__ == "__main__":
    main()
