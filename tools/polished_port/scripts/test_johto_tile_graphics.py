#!/usr/bin/env python3
"""Verify byte-perfect original Johto pixel conversion and tile/VRAM ordering."""
import struct

from audit_johto_graphics_mapping import load_fixture
from stage_johto_tile_graphics import (
    GBA_PRIMARY_TILE_CAPACITY,
    GBA_SECONDARY_TILE_CAPACITY,
    GBA_TILE_BYTES,
    SOURCE_COMMON_TILES,
    SOURCE_TOTAL_TILES,
    SOURCE_TRADITIONAL_TILES,
    FIRST_PRIORITY_GBA_TILE,
    EXPECTED_PRIORITY_REFERENCES,
    EXPECTED_PRIORITY_UNIQUE_TILES,
    priority_overlay_tile_ids,
    mask_source_zero_for_priority,
    SOURCES,
    build_graphics,
    encode_gba_tile,
    gba_tile_id,
    load_pinned_png,
    parse_grayscale2_png,
    png_to_tiles,
)
from stage_johto_time_palettes import load_source_palettes, stage


def rejects(f):
    try:
        f()
    except ValueError:
        return
    raise AssertionError("invalid graphics tile/palette/pixel accepted")


def main():
    primary, secondary, review = build_graphics()
    assert len(primary) == GBA_PRIMARY_TILE_CAPACITY * GBA_TILE_BYTES == 16384
    assert len(secondary) == GBA_SECONDARY_TILE_CAPACITY * GBA_TILE_BYTES == 16384
    assert primary[:32] == b"\x00" * 32
    assert primary[301 * 32:] == b"\x00" * (211 * 32)
    assert secondary == b"\x00" * 16384
    assert review["source_png_tile_counts"] == {
        "common_vram0": SOURCE_COMMON_TILES,
        "traditional_vram1": SOURCE_TRADITIONAL_TILES,
    }
    assert (SOURCE_COMMON_TILES, SOURCE_TRADITIONAL_TILES, SOURCE_TOTAL_TILES) == (
        128, 160, 288
    )
    assert review["gba_source_common_tile_ids"] == [1, 128]
    assert review["gba_source_traditional_tile_ids"] == [129, 288]
    assert review["gba_4bpp_reserved_transparent_tile_id"] == 0
    assert review["gba_priority_overlay_tile_count"] == 12
    assert review["gba_priority_overlay_ids"] == [289, 300]
    assert review["priority_source_subtile_references"] == 38
    assert review["priority_mask_gbc_color_zero_transparent"]
    assert review["priority_masks_keep_gbc_colors_1_to_3_unchanged"]
    assert review["priority_source_flag_mapping_staged"]
    assert not review["priority_sprite_elevation_parity_verified"]
    assert review["gba_padded_tile_count"] == 724
    assert review["palette_gba_index_4_must_duplicate_source_color_0"]
    assert not review["emerald_tileset_registered"]
    assert not review["ready_for_live_registration"]
    assert not review["gbc_priority_layers_implemented"]

    originals = []
    for filename, width, height, blob_sha in SOURCES:
        data = load_pinned_png(filename, blob_sha)
        assert len(data) in (1703, 1999)
        rows = parse_grayscale2_png(data, expected_width=width, expected_height=height)
        assert len(rows) == height
        assert all(len(row) == width for row in rows)
        source_tiles = png_to_tiles(rows)
        assert len(source_tiles) == (width // 8) * (height // 8)
        originals.extend(source_tiles)

        # Byte-perfect reconstruction of the source grayscale pixels:
        # 3 white (CGB 0) -> GBA4, 2->1, 1->2, 0 black->3.
        encoded_bytes = b"".join(source_tiles)
        assert all(((x & 15) in (1, 2, 3, 4)
                    and (x >> 4) in (1, 2, 3, 4)) for x in encoded_bytes)
    assert len(originals) == 288
    assert primary[32:289 * 32] == b"".join(originals)

    # Original GBC BG bit7 pixels 1..3 cover sprites, but index 0 does not.
    # Masked BG1 clones preserve that behavior; opaque BG2 art is unchanged.
    source_metatiles = load_fixture("metatiles")
    source_attributes = load_fixture("attributes")
    mask_map = priority_overlay_tile_ids(source_metatiles, source_attributes)
    assert len(mask_map) == EXPECTED_PRIORITY_UNIQUE_TILES == 12
    assert FIRST_PRIORITY_GBA_TILE == 289
    assert list(mask_map.values()) == list(range(289, 301))
    assert sorted(mask_map) == [12, 174, 189, 257, 258, 259, 260,
                                261, 273, 274, 275, 276]
    assert sum(bool(a & 0x80) for a in source_attributes) == EXPECTED_PRIORITY_REFERENCES == 38
    for normal_id, mask_id in mask_map.items():
        ordinary = primary[normal_id * 32:(normal_id + 1) * 32]
        overlay = primary[mask_id * 32:(mask_id + 1) * 32]
        assert overlay == mask_source_zero_for_priority(ordinary)
        for bottom, top in zip(ordinary, overlay):
            for shift in (0, 4):
                opaque = (bottom >> shift) & 15
                front = (top >> shift) & 15
                assert front == (0 if opaque == 4 else opaque)
                assert opaque in (1, 2, 3, 4)
                assert front in (0, 1, 2, 3)

    metatiles = load_fixture("metatiles")
    attrs = load_fixture("attributes")
    assert len(metatiles) == len(attrs) == 4048
    banks = set()
    used_ids = set()
    for source_tile, attr in zip(metatiles, attrs):
        bank = int(bool(attr & 0x08))
        banks.add(bank)
        new_id = gba_tile_id(source_tile, bank)
        assert 1 <= new_id <= SOURCE_TOTAL_TILES
        assert primary[new_id * 32: (new_id + 1) * 32] == originals[new_id - 1]
        used_ids.add(new_id)
    assert banks == {0, 1}
    assert 0 not in used_ids
    assert gba_tile_id(0, 0) == 1
    assert gba_tile_id(127, 0) == 128
    assert gba_tile_id(0, 1) == 129
    assert gba_tile_id(159, 1) == 288

    # Synthetic pixels must remain in the GBA 1..4 range, not color 0.
    assert encode_gba_tile([(0, 1, 2, 3, 0, 1, 2, 3)] * 8) == (
        bytes([0x23, 0x41, 0x23, 0x41] * 8)
    )
    for sample in (0, 1, 2, 3):
        tile = encode_gba_tile([(sample,) * 8] * 8)
        assert len(set(tile)) == 1
        assert tile[0] != 0

    # The 4-bit palette index really exists in BOTH the Python and the
    # compiled C palette sources; no authored or modified RGB555 values.
    variants, report = stage(load_source_palettes())
    assert report["gba_palette_index_4_is_duplicate_of_source_color_0"]
    for raw in variants.values():
        assert len(raw) == 224
        for bank in range(7):
            vals = struct.unpack_from("<16H", raw, 32 * bank)
            assert vals[4] == vals[0]
            assert vals[5:] == (0,) * 11
    assert len(variants) == 12

    # Fail closed on truncated PNG, CRC corruption, or malformed source IDs.
    filename, width, height, sha = SOURCES[0]
    source = load_pinned_png(filename, sha)
    rejects(lambda: parse_grayscale2_png(source[:-1],
                                         expected_width=width, expected_height=height))
    rejects(lambda: parse_grayscale2_png(
        source[:40] + b"\x00" + source[41:],
        expected_width=width, expected_height=height
    ))
    rejects(lambda: parse_grayscale2_png(
        source, expected_width=width + 8, expected_height=height
    ))
    for bank, tile in ((0, 128), (1, 160), (2, 1), (0, -1), (1, -1)):
        rejects(lambda bank=bank, tile=tile: gba_tile_id(tile, bank))
    rejects(lambda: encode_gba_tile([(0,) * 8] * 7))
    rejects(lambda: encode_gba_tile([(4,) * 8] * 8))

    print("PASS: pinned Git blobs give exactly 128 shared + 160 Traditional tiles")
    print("PASS: 288 source tiles preserved plus 12 source-derived priority overlays")
    print("PASS: 724 GBA tile slots remain blank padding")
    print("PASS: all 4048 original source subtile references map to authentic 4bpp art")
    print("PASS: opaque GBC color0 uses GBA index4, never transparent index0")
    print("PASS: malformed assets and nonexistent tiles fail closed")
    print("PASS: all 38 bit7 positions use per-pixel color0 transparency")
    print("REVIEW: live sprite elevation parity and animations still pending")


if __name__ == "__main__":
    main()
