#!/usr/bin/env python3
"""
Extract the exact retail HGSS PokéGear Map player-position marker frames.

Retail resource set 1 / animation sequence 1 contains two static 16x16 frames:
cell 2 (tile 8) and cell 3 (tile 12), both using palette bank 15. Retail selects
the displayed frame from playerGender and disables animation.

This tool prepares the authentic pixels for provenance/verification only.
PKMN2K does not use Ethan/Lyra as its player avatars, so these outputs are not
wired live unless that project policy is explicitly changed.
"""

from pathlib import Path
import struct
import sys

from make_hgss_pokegear_map_cursor import (
    ANIMS,
    CELLS,
    PALETTE,
    SPRITES,
    extract_16x16_frame,
    load_indexed_png4,
    load_nclr_bank,
    u16,
    u32,
    verified_bytes,
)

PLAYER_SEQUENCE = 1
PLAYER_CELLS = (2, 3)
PLAYER_TILES = (8, 12)
PLAYER_FRAME_DURATIONS = (1, 1)
PLAYER_PALETTE_BANK = 15


def validate_player_animation():
    data = verified_bytes(ANIMS)
    if data[:4] != b"RNAN":
        raise ValueError(f"{ANIMS}: not a NANR")
    chunk = data.find(b"KNBA")
    if chunk < 0:
        raise ValueError(f"{ANIMS}: missing ABNK block")

    base = chunk + 8
    sequence_count = u16(data, base)
    frame_count = u16(data, base + 2)
    seq_base = base + u32(data, base + 4)
    frame_base = base + u32(data, base + 8)
    frame_data_base = base + u32(data, base + 12)
    if sequence_count != 12 or frame_count != 23:
        raise ValueError(
            f"{ANIMS}: expected 12 sequences / 23 frames, "
            f"got {sequence_count} / {frame_count}"
        )

    seq = seq_base + PLAYER_SEQUENCE * 16
    total = u16(data, seq)
    frame_start_bytes = u32(data, seq + 12)
    if total != 2:
        raise ValueError(f"{ANIMS}: player sequence expected 2 frames, got {total}")

    cells = []
    durations = []
    for i in range(total):
        frame = frame_base + frame_start_bytes + i * 8
        frame_data_ptr = u32(data, frame)
        cells.append(u16(data, frame_data_base + frame_data_ptr))
        durations.append(u16(data, frame + 4))

    if tuple(cells) != PLAYER_CELLS or tuple(durations) != PLAYER_FRAME_DURATIONS:
        raise ValueError(
            f"{ANIMS}: player frames {tuple(cells)} durations {tuple(durations)} "
            f"!= expected {PLAYER_CELLS} / {PLAYER_FRAME_DURATIONS}"
        )


def validate_player_cells():
    data = verified_bytes(CELLS)
    if data[:4] != b"RECN":
        raise ValueError(f"{CELLS}: not an NCER")
    chunk = data.find(b"KBEC")
    if chunk < 0:
        raise ValueError(f"{CELLS}: missing CEBK block")

    base = chunk + 8
    cell_count = u16(data, base)
    cell_data_base = base + u32(data, base + 4)
    cell_struct_size = 16
    oam_base = cell_data_base + cell_count * cell_struct_size

    for cell_index, expected_tile in zip(PLAYER_CELLS, PLAYER_TILES):
        cell = cell_data_base + cell_index * cell_struct_size
        obj_count = u16(data, cell)
        oam_offset = u32(data, cell + 4)
        if obj_count != 1:
            raise ValueError(f"{CELLS}: player cell {cell_index} has {obj_count} OBJs")

        oam = oam_base + oam_offset
        attr0 = u16(data, oam)
        attr1 = u16(data, oam + 2)
        attr2 = u16(data, oam + 4)
        shape = (attr0 >> 14) & 3
        size = (attr1 >> 14) & 3
        tile = attr2 & 0x03FF
        palette_bank = (attr2 >> 12) & 0xF
        hflip = bool(attr1 & 0x1000)
        vflip = bool(attr1 & 0x2000)

        if shape != 0 or size != 1:
            raise ValueError(f"{CELLS}: player cell {cell_index} is not 16x16")
        if tile != expected_tile or palette_bank != PLAYER_PALETTE_BANK:
            raise ValueError(
                f"{CELLS}: player cell {cell_index} uses tile {tile}, "
                f"palette {palette_bank}; expected {expected_tile}, "
                f"{PLAYER_PALETTE_BANK}"
            )
        if hflip or vflip:
            raise ValueError(f"{CELLS}: player cell {cell_index} unexpectedly flips")


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("--tiles", "--palette"):
        raise SystemExit(
            "usage: make_hgss_pokegear_map_player_marker.py "
            "(--tiles|--palette) OUTPUT"
        )

    validate_player_animation()
    validate_player_cells()
    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)

    if sys.argv[1] == "--tiles":
        rows = load_indexed_png4(SPRITES)
        output.write_bytes(
            b"".join(extract_16x16_frame(rows, tile) for tile in PLAYER_TILES)
        )
    else:
        output.write_bytes(load_nclr_bank(PLAYER_PALETTE_BANK))


if __name__ == "__main__":
    main()
