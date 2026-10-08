#!/usr/bin/env python3
from pathlib import Path
import sys

import make_hgss_pokegear_cursor as common


PHONE_STATUS_SEQUENCE = 3
PHONE_STATUS_CELLS = (18, 19)
PHONE_STATUS_TILES = (200, 204)
PHONE_STATUS_PALETTE_BANK = 3


def validate_phone_status_nanr(path):
    data = path.read_bytes()
    if data[:4] != b"RNAN":
        raise ValueError(f"{path}: not a NANR")

    chunk = data.find(b"KNBA")
    if chunk < 0:
        raise ValueError(f"{path}: missing ABNK block")

    base = chunk + 8
    sequence_count = common.u16(data, base)
    frame_count = common.u16(data, base + 2)
    seq_base = base + common.u32(data, base + 4)
    frame_base = base + common.u32(data, base + 8)
    frame_data_base = base + common.u32(data, base + 12)

    if sequence_count != 8 or frame_count != 28:
        raise ValueError(
            f"{path}: unexpected animation bank {sequence_count} sequences / "
            f"{frame_count} frames"
        )

    seq = seq_base + PHONE_STATUS_SEQUENCE * 16
    frame_total = common.u16(data, seq)
    frame_start_bytes = common.u32(data, seq + 12)
    if frame_total != 2:
        raise ValueError(
            f"{path}: phone-status sequence expected 2 frames, got {frame_total}"
        )

    resolved = []
    for frame_index in range(frame_total):
        frame = frame_base + frame_start_bytes + frame_index * 8
        frame_data_ptr = common.u32(data, frame)
        resolved.append(common.u16(data, frame_data_base + frame_data_ptr))

    if tuple(resolved) != PHONE_STATUS_CELLS:
        raise ValueError(
            f"{path}: phone-status sequence resolves to {tuple(resolved)}, "
            f"expected {PHONE_STATUS_CELLS}"
        )


def validate_phone_status_cells(path):
    data = path.read_bytes()
    if data[:4] != b"RECN":
        raise ValueError(f"{path}: not an NCER")

    chunk = data.find(b"KBEC")
    if chunk < 0:
        raise ValueError(f"{path}: missing CEBK block")

    base = chunk + 8
    cell_count = common.u16(data, base)
    bank_attributes = common.u16(data, base + 2)
    cell_data_base = base + common.u32(data, base + 4)
    mapping_mode = common.u32(data, base + 8)

    if cell_count != 24 or bank_attributes != 1 or mapping_mode != 0:
        raise ValueError(
            f"{path}: unexpected cell bank metadata "
            f"(cells={cell_count}, attr={bank_attributes}, mapping={mapping_mode})"
        )

    cell_struct_size = 16
    oam_base = cell_data_base + cell_count * cell_struct_size

    for cell_index, expected_tile in zip(PHONE_STATUS_CELLS, PHONE_STATUS_TILES):
        cell = cell_data_base + cell_index * cell_struct_size
        obj_count = common.u16(data, cell)
        oam_offset = common.u32(data, cell + 4)
        if obj_count != 1:
            raise ValueError(
                f"{path}: phone-status cell {cell_index} expected one OBJ, "
                f"got {obj_count}"
            )

        oam = oam_base + oam_offset
        attr0 = common.u16(data, oam)
        attr1 = common.u16(data, oam + 2)
        attr2 = common.u16(data, oam + 4)

        shape = (attr0 >> 14) & 3
        size = (attr1 >> 14) & 3
        tile = attr2 & 0x03FF
        palette_bank = (attr2 >> 12) & 0xF
        hflip = bool(attr1 & 0x1000)
        vflip = bool(attr1 & 0x2000)

        if shape != 0 or size != 1:
            raise ValueError(
                f"{path}: phone-status cell {cell_index} is not retail 16x16 square"
            )
        if tile != expected_tile or palette_bank != PHONE_STATUS_PALETTE_BANK:
            raise ValueError(
                f"{path}: phone-status cell {cell_index} uses tile {tile}, "
                f"palette {palette_bank}; expected tile {expected_tile}, "
                f"palette {PHONE_STATUS_PALETTE_BANK}"
            )
        if hflip or vflip:
            raise ValueError(
                f"{path}: phone-status cell {cell_index} unexpectedly uses flips"
            )


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("--tiles", "--palette"):
        raise SystemExit(
            "usage: make_hgss_pokegear_phone_status.py "
            "(--tiles|--palette) OUTPUT"
        )

    validate_phone_status_nanr(common.ANIMS)
    validate_phone_status_cells(common.CELLS)

    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)

    if sys.argv[1] == "--tiles":
        rows = common.load_indexed_png4(common.SPRITES)
        active = common.extract_base_cursor_tiles(rows, PHONE_STATUS_TILES[0])
        disabled = common.extract_base_cursor_tiles(rows, PHONE_STATUS_TILES[1])
        output.write_bytes(active + disabled)
    else:
        output.write_bytes(common.make_palette(PHONE_STATUS_PALETTE_BANK))


if __name__ == "__main__":
    main()
