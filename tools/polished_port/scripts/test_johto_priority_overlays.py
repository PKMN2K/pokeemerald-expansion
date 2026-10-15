#!/usr/bin/env python3
"""Regress per-8x8 GBC bit7 priority, color0 punch-through, and Emerald BG layers.

GBC BG attribute bit7 prioritizes BG color indices 1..3 over sprites, but
color0 pixels still let sprites through. A whole-tile opaque BG1 copy would
incorrectly cover sprite pixels where original source color0 was used.

This tests stage-only graphics/metatiles. BG/OBJ behavior for raised sprites
(Emerald OAM priority 1 vs BG1 priority 1) still requires live review.
"""
from collections import Counter
from pathlib import Path
import struct

from audit_johto_graphics_mapping import load_fixture
from convert_johto_traditional_blocks import convert
from stage_johto_tile_graphics import (
    build_graphics, gba_tile_id, priority_overlay_tile_ids,
    mask_source_zero_for_priority,
)

ROOT = Path(__file__).resolve().parents[3]
EXPECTED_FLAGGED_BLOCKS = {20: 2, 145: 8, 146: 8, 210: 8, 211: 8, 214: 2, 215: 2}
EXPECTED_FLAGGED_METATILES = {81, 582, 583, 586, 587, 842, 843, 846, 847, 856, 861}
EXPECTED_OVERLAYS = {12: 289, 174: 290, 189: 291,
                     257: 292, 258: 293, 259: 294, 260: 295,
                     261: 296, 273: 297, 274: 298, 275: 299,
                     276: 300}


def reject(call):
    try:
        call()
    except ValueError:
        return
    raise AssertionError("an invalid priority tile/attribute was accepted")


def main():
    meta = load_fixture("metatiles")
    attrs = load_fixture("attributes")
    overlay_ids = priority_overlay_tile_ids(meta, attrs)
    assert overlay_ids == EXPECTED_OVERLAYS

    foreground = convert(meta, attrs)
    primary, secondary, report = build_graphics()
    assert len(foreground) == 1012 * 16
    assert len(primary) == len(secondary) == 512 * 32
    assert secondary == b"\0" * (512 * 32)
    assert report["priority_source_flag_mapping_staged"]
    assert not report["priority_sprite_elevation_parity_verified"]
    assert not report["emerald_tileset_registered"]

    occurrences = []
    blocks = Counter()
    metatile_ids = set()
    flipped = 0
    # Each original GBC block is 4x4 subtiles. Each 2x2 quadrant maps
    # to one Emerald 16x16 metatile with bottom entries 0..3/top 4..7.
    for source_index, (tile, attr) in enumerate(zip(meta, attrs)):
        block, local_index = divmod(source_index, 16)
        y, x = divmod(local_index, 4)
        metatile_id = block * 4 + 2 * (y // 2) + (x // 2)
        gba_subtile = 2 * (y % 2) + x % 2
        base = struct.unpack_from("<H", foreground, metatile_id * 16 + gba_subtile * 2)[0]
        top = struct.unpack_from("<H", foreground, metatile_id * 16 + (4 + gba_subtile) * 2)[0]
        expected_base_id = gba_tile_id(tile, int(bool(attr & 0x08)))
        assert base & 0x3FF == expected_base_id
        assert (base >> 12) == (attr & 7)
        assert bool(base & 0x400) == bool(attr & 0x20)
        assert bool(base & 0x800) == bool(attr & 0x40)

        if attr & 0x80:
            blocks[block] += 1
            metatile_ids.add(metatile_id)
            occurrences.append((source_index, base, top))
            assert top & 0x3FF == overlay_ids[expected_base_id]
            # Tile ID differs, but original source palette and flip bits
            # match on both BG2 opaque tile and BG1 masked foreground.
            assert (base & ~0x3FF) == (top & ~0x3FF)
            assert (attr & 7) == 6  # Pinned priority entries all use roof bank
            flipped += bool(attr & 0x20)
            base_pixels = primary[expected_base_id * 32:(expected_base_id + 1) * 32]
            mask_pixels = primary[(top & 0x3FF) * 32:((top & 0x3FF) + 1) * 32]
            assert mask_pixels == mask_source_zero_for_priority(base_pixels)
        else:
            assert top == 0

    assert len(occurrences) == 38
    assert dict(blocks) == EXPECTED_FLAGGED_BLOCKS
    assert metatile_ids == EXPECTED_FLAGGED_METATILES
    assert flipped == 10
    assert len(overlay_ids) == 12

    # Source CGB color0 remains solid on the base field layer at palette
    # index4, but is transparent on the foreground overlay at pixel 0.
    original_zero_pixels = 0
    for base_id, top_id in overlay_ids.items():
        for a, b in zip(primary[base_id * 32:(base_id + 1) * 32],
                        primary[top_id * 32:(top_id + 1) * 32]):
            for shift in (0, 4):
                source_pixel = (a >> shift) & 15
                masked_pixel = (b >> shift) & 15
                if source_pixel == 4:
                    original_zero_pixels += 1
                    assert masked_pixel == 0
                else:
                    assert masked_pixel == source_pixel
                    assert source_pixel in (1, 2, 3)
    assert original_zero_pixels > 0

    # Source ordering and tile0 are both mandatory for this staged layout.
    assert primary[:32] == bytes(32)
    assert primary[301 * 32:] == bytes(211 * 32)

    # Emerald NORMAL places bottom tiles on BG2 and top tiles on BG1.
    # A COVERED layer would wrongly move the foreground to BG2.
    engine = (ROOT / "src/field_camera.c").read_text(encoding="utf-8")
    field_h = (ROOT / "include/global.fieldmap.h").read_text(encoding="utf-8")
    assert "case METATILE_LAYER_TYPE_NORMAL:" in engine
    assert "gOverworldTilemapBuffer_Bg2[offset] = tiles[0]" in engine
    assert "gOverworldTilemapBuffer_Bg1[offset] = tiles[4]" in engine
    assert "METATILE_LAYER_TYPE_NORMAL,  // Metatile uses middle and top bg layers" in field_h

    # Any changed source flag or invalid tile ID must refuse to reuse the
    # audited 12-mask assignment without a fresh source-graphic review.
    changed_attrs = bytearray(attrs)
    changed_attrs[20 * 16 + 6] &= ~0x80
    reject(lambda: priority_overlay_tile_ids(meta, changed_attrs))
    changed_tiles = bytearray(meta)
    changed_tiles[20 * 16 + 6] = 255
    reject(lambda: priority_overlay_tile_ids(changed_tiles, attrs))
    reject(lambda: mask_source_zero_for_priority(bytes(31)))
    reject(lambda: mask_source_zero_for_priority(bytes(32)))

    print("PASS: 38 original CGB bit7 entries mapped across seven source blocks")
    print("PASS: 11 Emerald metatiles carry 12 distinct masked graphics tiles")
    print("PASS: CGB color0 remains opaque on BG2 but transparent on BG1")
    print("PASS: all palette/flip bits survive priority layering")
    print("PASS: unflagged subtiles use empty top, not forced priority")
    print("REVIEW: sprite elevation and animation parity still required before live maps")


if __name__ == "__main__":
    main()
