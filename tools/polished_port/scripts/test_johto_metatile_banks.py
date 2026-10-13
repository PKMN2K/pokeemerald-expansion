#!/usr/bin/env python3
"""Regression checks for authentic block ordering across Emerald's two banks."""
from pathlib import Path
import struct

from convert_johto_traditional_blocks import convert
from extract_johto_collision import extract
from split_johto_metatile_banks import (
    BANK_BYTES, BYTES_PER_METATILE, manifest, source_block_address,
    split_geometry,
)

ROOT = Path(__file__).resolve().parents[3]


def main():
    # Use distinguishable tile IDs for each source 32x32 block.
    gb_tile_ids = bytes(block for block in range(256) for _ in range(16))
    gb_attrs = bytearray(4096)
    # Palette 7 + bank 1 + both flips on first tile; exact GBA tile entry.
    gb_attrs[0] = 0x6F
    converted = convert(gb_tile_ids, bytes(gb_attrs))
    primary, secondary = split_geometry(converted)
    assert len(primary) == len(secondary) == BANK_BYTES == 8192
    assert primary + secondary == converted
    assert BYTES_PER_METATILE == 16

    assert struct.unpack_from("<H", primary, 0)[0] == 0x7D00
    # For each quadrant, lower 4 words are source tiles. Upper 4 are
    # currently placeholders, which are not yet approved for live use.
    assert struct.unpack_from("<4H", primary, 8) == (0, 0, 0, 0)
    assert struct.unpack_from("<H", primary, 508 * 16)[0] == 127
    assert struct.unpack_from("<H", secondary, 0)[0] == 128
    assert struct.unpack_from("<H", secondary, 511 * 16)[0] == 255

    assert source_block_address(127, "bottom_right") == {
        "bank": "primary", "map_metatile_id": 511, "bank_local_index": 511}
    assert source_block_address(128, "top_left") == {
        "bank": "secondary", "map_metatile_id": 512, "bank_local_index": 0}
    assert source_block_address(255, "bottom_right") == {
        "bank": "secondary", "map_metatile_id": 1023, "bank_local_index": 511}
    record = manifest()
    assert record["status"] == "staging_only_not_registered"
    addresses = [
        item["map_metatile_id"]
        for block in record["block_address"]
        for item in block["quadrants"].values()
    ]
    assert sorted(addresses) == list(range(1024))

    # Cross-check the actual pinned source collision declarations align
    # with the first 253 source geometry block IDs.
    source = (ROOT / "tools/polished_port/fixtures/johto_traditional_collision.asm")
    source_blocks = extract(source.read_text(encoding="utf-8"))
    assert len(source_blocks) == 253
    for block in source_blocks:
        index = block["block"]
        for quadrant in ("top_left", "top_right", "bottom_left", "bottom_right"):
            assert source_block_address(index, quadrant)["map_metatile_id"] == (
                index * 4 + ("top_left", "top_right", "bottom_left", "bottom_right").index(quadrant))

    for invalid in (b"", converted[:-2], converted + b"\x00\x00"):
        try:
            split_geometry(invalid)
        except ValueError:
            pass
        else:
            raise AssertionError("invalid length accepted")
    print("PASS: exact 512+512 geometry split, block boundary, and 253 collision block IDs")


if __name__ == "__main__":
    main()
