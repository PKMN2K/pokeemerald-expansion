#!/usr/bin/env python3
"""
Extract the exact retail HGSS PokeGear Map fly-point indicator for pokeemerald.

The reusable fly-point marker is resource set 1, animation sequence 9.
Retail sequence 9 resolves to cell 17, a single 16x16 OBJ using tile 53 and
palette bank 0. This adapter exports that exact frame and palette without
redrawing, recoloring, scaling, or importing any Johto/Kanto geography.
"""

from pathlib import Path
import sys

import make_hgss_pokegear_map_cursor as mapres

FLYPOINT_SEQUENCE = 9
FLYPOINT_CELL = 17
FLYPOINT_TILE = 53
FLYPOINT_DURATION = 1
FLYPOINT_PALETTE_BANK = 0


def validate_flypoint_animation():
    data = mapres.verified_bytes(mapres.ANIMS)
    if data[:4] != b"RNAN":
        raise ValueError(f"{mapres.ANIMS}: not a NANR")
    chunk = data.find(b"KNBA")
    if chunk < 0:
        raise ValueError(f"{mapres.ANIMS}: missing ABNK block")

    base = chunk + 8
    sequence_count = mapres.u16(data, base)
    frame_count = mapres.u16(data, base + 2)
    seq_base = base + mapres.u32(data, base + 4)
    frame_base = base + mapres.u32(data, base + 8)
    frame_data_base = base + mapres.u32(data, base + 12)
    if sequence_count != 12 or frame_count != 23:
        raise ValueError(
            f"{mapres.ANIMS}: expected 12 sequences / 23 frames, "
            f"got {sequence_count} / {frame_count}"
        )

    seq = seq_base + FLYPOINT_SEQUENCE * 16
    total = mapres.u16(data, seq)
    frame_start_bytes = mapres.u32(data, seq + 12)
    if total != 1:
        raise ValueError(
            f"{mapres.ANIMS}: fly-point sequence expected 1 frame, got {total}"
        )

    frame = frame_base + frame_start_bytes
    frame_data_ptr = mapres.u32(data, frame)
    cell = mapres.u16(data, frame_data_base + frame_data_ptr)
    duration = mapres.u16(data, frame + 4)
    if cell != FLYPOINT_CELL or duration != FLYPOINT_DURATION:
        raise ValueError(
            f"{mapres.ANIMS}: fly-point frame cell/duration "
            f"{cell}/{duration} != {FLYPOINT_CELL}/{FLYPOINT_DURATION}"
        )


def validate_flypoint_cell():
    data = mapres.verified_bytes(mapres.CELLS)
    if data[:4] != b"RECN":
        raise ValueError(f"{mapres.CELLS}: not an NCER")
    chunk = data.find(b"KBEC")
    if chunk < 0:
        raise ValueError(f"{mapres.CELLS}: missing CEBK block")

    base = chunk + 8
    cell_count = mapres.u16(data, base)
    bank_attributes = mapres.u16(data, base + 2)
    cell_data_base = base + mapres.u32(data, base + 4)
    mapping_mode = mapres.u32(data, base + 8)
    if cell_count != 18 or bank_attributes != 1 or mapping_mode != 0:
        raise ValueError(
            f"{mapres.CELLS}: unexpected cell bank "
            f"(cells={cell_count}, attr={bank_attributes}, mapping={mapping_mode})"
        )

    cell_struct_size = 16
    oam_base = cell_data_base + cell_count * cell_struct_size
    cell = cell_data_base + FLYPOINT_CELL * cell_struct_size
    obj_count = mapres.u16(data, cell)
    oam_offset = mapres.u32(data, cell + 4)
    if obj_count != 1:
        raise ValueError(
            f"{mapres.CELLS}: fly-point cell {FLYPOINT_CELL} has {obj_count} OBJs"
        )

    oam = oam_base + oam_offset
    attr0 = mapres.u16(data, oam)
    attr1 = mapres.u16(data, oam + 2)
    attr2 = mapres.u16(data, oam + 4)
    shape = (attr0 >> 14) & 3
    size = (attr1 >> 14) & 3
    tile = attr2 & 0x03FF
    palette_bank = (attr2 >> 12) & 0xF
    hflip = bool(attr1 & 0x1000)
    vflip = bool(attr1 & 0x2000)

    if shape != 0 or size != 1:
        raise ValueError(
            f"{mapres.CELLS}: fly-point cell {FLYPOINT_CELL} is not 16x16"
        )
    if tile != FLYPOINT_TILE or palette_bank != FLYPOINT_PALETTE_BANK:
        raise ValueError(
            f"{mapres.CELLS}: fly-point cell uses tile {tile}, palette "
            f"{palette_bank}; expected {FLYPOINT_TILE}, "
            f"{FLYPOINT_PALETTE_BANK}"
        )
    if hflip or vflip:
        raise ValueError(
            f"{mapres.CELLS}: fly-point cell unexpectedly flips"
        )


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("--tiles", "--palette"):
        raise SystemExit(
            "usage: make_hgss_pokegear_map_flypoint.py "
            "(--tiles|--palette) OUTPUT"
        )

    validate_flypoint_animation()
    validate_flypoint_cell()
    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)

    if sys.argv[1] == "--tiles":
        rows = mapres.load_indexed_png4(mapres.SPRITES)
        output.write_bytes(mapres.extract_16x16_frame(rows, FLYPOINT_TILE))
    else:
        output.write_bytes(mapres.load_nclr_bank(FLYPOINT_PALETTE_BANK))


if __name__ == "__main__":
    main()
