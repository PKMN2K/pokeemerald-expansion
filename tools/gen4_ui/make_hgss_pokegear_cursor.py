#!/usr/bin/env python3
from pathlib import Path
import binascii
import struct
import sys
import zlib


SPRITES = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_ui_sprites.png")
PALETTE = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_ui_palette.NCLR")
CELLS = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_ui_cells.NCER")
ANIMS = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_ui_anims.NANR")

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
CURSOR_SEQUENCES = (4, 5, 6, 7)
CURSOR_CELLS = (20, 21, 22, 23)
BASE_CELL = 20
EXPECTED_TILE = 208
EXPECTED_PALETTE_BANK = 1


def u16(data, off):
    return struct.unpack_from("<H", data, off)[0]


def u32(data, off):
    return struct.unpack_from("<I", data, off)[0]


def paeth_predictor(a, b, c):
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def load_indexed_png4(path):
    data = path.read_bytes()
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError(f"{path}: not a PNG")

    pos = len(PNG_SIGNATURE)
    ihdr = None
    idat = bytearray()

    while pos < len(data):
        if pos + 12 > len(data):
            raise ValueError(f"{path}: truncated PNG chunk")
        length = struct.unpack_from(">I", data, pos)[0]
        chunk_type = data[pos + 4:pos + 8]
        payload_start = pos + 8
        payload_end = payload_start + length
        crc_end = payload_end + 4
        if crc_end > len(data):
            raise ValueError(f"{path}: truncated {chunk_type!r} chunk")
        payload = data[payload_start:payload_end]

        expected_crc = struct.unpack_from(">I", data, payload_end)[0]
        actual_crc = binascii.crc32(chunk_type + payload) & 0xFFFFFFFF
        if expected_crc != actual_crc:
            raise ValueError(f"{path}: invalid {chunk_type!r} CRC")

        if chunk_type == b"IHDR":
            ihdr = struct.unpack(">IIBBBBB", payload)
        elif chunk_type == b"IDAT":
            idat.extend(payload)
        elif chunk_type == b"IEND":
            break

        pos = crc_end

    if ihdr is None:
        raise ValueError(f"{path}: missing IHDR")

    width, height, bit_depth, color_type, compression, filter_method, interlace = ihdr
    if (width, height) != (32, 424):
        raise ValueError(f"{path}: expected 32x424, got {width}x{height}")
    if bit_depth != 4 or color_type != 3:
        raise ValueError(
            f"{path}: expected 4-bit indexed PNG, got bit_depth={bit_depth}, "
            f"color_type={color_type}"
        )
    if compression != 0 or filter_method != 0 or interlace != 0:
        raise ValueError(f"{path}: unsupported PNG encoding")

    packed_row_bytes = (width + 1) // 2
    raw = zlib.decompress(bytes(idat))
    expected_size = height * (packed_row_bytes + 1)
    if len(raw) != expected_size:
        raise ValueError(
            f"{path}: expected {expected_size} decompressed bytes, got {len(raw)}"
        )

    rows = []
    prev = bytearray(packed_row_bytes)
    offset = 0
    for _ in range(height):
        filter_type = raw[offset]
        scan = bytearray(raw[offset + 1:offset + 1 + packed_row_bytes])
        offset += packed_row_bytes + 1

        recon = bytearray(packed_row_bytes)
        for x, value in enumerate(scan):
            left = recon[x - 1] if x else 0
            up = prev[x]
            up_left = prev[x - 1] if x else 0

            if filter_type == 0:
                recon[x] = value
            elif filter_type == 1:
                recon[x] = (value + left) & 0xFF
            elif filter_type == 2:
                recon[x] = (value + up) & 0xFF
            elif filter_type == 3:
                recon[x] = (value + ((left + up) // 2)) & 0xFF
            elif filter_type == 4:
                recon[x] = (value + paeth_predictor(left, up, up_left)) & 0xFF
            else:
                raise ValueError(f"{path}: unsupported PNG filter {filter_type}")

        pixels = []
        for value in recon:
            pixels.append(value >> 4)
            pixels.append(value & 0x0F)
        rows.append(pixels[:width])
        prev = recon

    return rows


def load_nclr(path):
    data = path.read_bytes()
    if data[:4] != b"RLCN":
        raise ValueError(f"{path}: not an NCLR")

    chunk = data.find(b"TTLP")
    if chunk < 0:
        raise ValueError(f"{path}: missing PLTT chunk")

    data_size = u32(data, chunk + 0x10)
    data_offset = u32(data, chunk + 0x14)
    start = chunk + 8 + data_offset
    raw = data[start:start + data_size]
    if len(raw) != data_size or data_size % 2:
        raise ValueError(f"{path}: malformed palette payload")

    return [
        u16(raw, i)
        for i in range(0, len(raw), 2)
    ]


def validate_nanr(path):
    data = path.read_bytes()
    if data[:4] != b"RNAN":
        raise ValueError(f"{path}: not a NANR")

    chunk = data.find(b"KNBA")
    if chunk < 0:
        raise ValueError(f"{path}: missing ABNK block")

    base = chunk + 8
    sequence_count = u16(data, base)
    frame_count = u16(data, base + 2)
    seq_base = base + u32(data, base + 4)
    frame_base = base + u32(data, base + 8)
    frame_data_base = base + u32(data, base + 12)

    if sequence_count != 8 or frame_count != 28:
        raise ValueError(
            f"{path}: unexpected animation bank {sequence_count} sequences / "
            f"{frame_count} frames"
        )

    resolved = []
    for seq_index in CURSOR_SEQUENCES:
        seq = seq_base + seq_index * 16
        frame_total = u16(data, seq)
        frame_start_bytes = u32(data, seq + 12)
        if frame_total != 2:
            raise ValueError(
                f"{path}: cursor sequence {seq_index} expected 2 frames, "
                f"got {frame_total}"
            )

        first_frame = frame_base + frame_start_bytes
        frame_data_ptr = u32(data, first_frame)
        cell = u16(data, frame_data_base + frame_data_ptr)
        resolved.append(cell)

    if tuple(resolved) != CURSOR_CELLS:
        raise ValueError(
            f"{path}: cursor sequences resolve to {tuple(resolved)}, "
            f"expected {CURSOR_CELLS}"
        )


def load_cursor_cell(path):
    data = path.read_bytes()
    if data[:4] != b"RECN":
        raise ValueError(f"{path}: not an NCER")

    chunk = data.find(b"KBEC")
    if chunk < 0:
        raise ValueError(f"{path}: missing CEBK block")

    base = chunk + 8
    cell_count = u16(data, base)
    bank_attributes = u16(data, base + 2)
    cell_data_base = base + u32(data, base + 4)
    mapping_mode = u32(data, base + 8)

    if cell_count != 24 or bank_attributes != 1 or mapping_mode != 0:
        raise ValueError(
            f"{path}: unexpected cell bank metadata "
            f"(cells={cell_count}, attr={bank_attributes}, mapping={mapping_mode})"
        )

    cell_struct_size = 16
    oam_base = cell_data_base + cell_count * cell_struct_size
    expected_flips = (
        (False, False),
        (False, True),
        (True, False),
        (True, True),
    )

    result = None
    for cell_index, expected_flip in zip(CURSOR_CELLS, expected_flips):
        cell = cell_data_base + cell_index * cell_struct_size
        obj_count = u16(data, cell)
        oam_offset = u32(data, cell + 4)
        if obj_count != 1:
            raise ValueError(
                f"{path}: cursor cell {cell_index} expected one OBJ, got {obj_count}"
            )

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
            raise ValueError(
                f"{path}: cursor cell {cell_index} is not retail 16x16 square"
            )
        if tile != EXPECTED_TILE or palette_bank != EXPECTED_PALETTE_BANK:
            raise ValueError(
                f"{path}: cursor cell {cell_index} uses tile {tile}, "
                f"palette {palette_bank}; expected tile {EXPECTED_TILE}, "
                f"palette {EXPECTED_PALETTE_BANK}"
            )
        if (hflip, vflip) != expected_flip:
            raise ValueError(
                f"{path}: cursor cell {cell_index} flip {(hflip, vflip)} "
                f"does not match {expected_flip}"
            )

        if cell_index == BASE_CELL:
            result = (tile, palette_bank)

    return result


def extract_base_cursor_tiles(rows, tile_index):
    tiles_per_row = len(rows[0]) // 8
    out = [[0] * 16 for _ in range(16)]

    # In 1D 32K OBJ mapping, the retail 16x16 cell consumes four consecutive
    # 8x8 character tiles: TL, TR, BL, BR.
    for offset in range(4):
        source_tile = tile_index + offset
        src_x = (source_tile % tiles_per_row) * 8
        src_y = (source_tile // tiles_per_row) * 8
        dst_x = (offset % 2) * 8
        dst_y = (offset // 2) * 8

        if src_y + 8 > len(rows):
            raise ValueError("cursor tile falls outside member-6 source image")

        for y in range(8):
            for x in range(8):
                value = rows[src_y + y][src_x + x]
                if value > 15:
                    raise ValueError("member-6 pixel exceeds 4-bit range")
                out[dst_y + y][dst_x + x] = value

    packed = bytearray()
    for tile_y in range(2):
        for tile_x in range(2):
            for y in range(8):
                row = out[tile_y * 8 + y]
                for x in range(0, 8, 2):
                    p0 = row[tile_x * 8 + x]
                    p1 = row[tile_x * 8 + x + 1]
                    packed.append(p0 | (p1 << 4))

    if len(packed) != 128:
        raise ValueError(f"unexpected cursor tile size {len(packed)}")
    return bytes(packed)


def make_palette(palette_bank):
    colors = load_nclr(PALETTE)
    start = palette_bank * 16
    bank = colors[start:start + 16]
    if len(bank) != 16:
        raise ValueError(
            f"{PALETTE}: missing palette bank {palette_bank}"
        )
    return struct.pack("<16H", *bank)


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("--tiles", "--palette"):
        raise SystemExit(
            "usage: make_hgss_pokegear_cursor.py "
            "(--tiles|--palette) OUTPUT"
        )

    validate_nanr(ANIMS)
    tile, palette_bank = load_cursor_cell(CELLS)

    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)

    if sys.argv[1] == "--tiles":
        rows = load_indexed_png4(SPRITES)
        output.write_bytes(extract_base_cursor_tiles(rows, tile))
    else:
        output.write_bytes(make_palette(palette_bank))


if __name__ == "__main__":
    main()
