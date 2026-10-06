#!/usr/bin/env python3
"""
Extract the exact retail HGSS PokeGear Map cursor for pokeemerald.

HGSS map resource set 1 is defined by resdat members 32/33/34/35 and header
80. Within that set, sSpriteTemplates[2] uses animation 0 for the live map
cursor. Retail animation 0 resolves to cells 0 and 1, each a 16x16 OBJ using
tiles 0 and 4 respectively with palette bank 15.

The retail player marker (animation 1) is deliberately not exported because
PKMN2K uses different protagonists.
"""

from pathlib import Path
import binascii
import hashlib
import struct
import sys
import zlib


ROOT = Path("graphics/gen4_ui/hgss_pokegear/verified/map")
PALETTE = ROOT / "pgmap_gra_00000000.NCLR"
SPRITES = ROOT / "pgmap_gra_00000001.png"
CELLS = ROOT / "pgmap_gra_00000002.NCER"
ANIMS = ROOT / "pgmap_gra_00000003.NANR"

EXPECTED_GIT_BLOBS = {
    PALETTE: "9e0e2984206e76c7b54ffac00c687d8753626497",
    SPRITES: "537eab25b10bf0803c3642df515bf3cdc289f3bc",
    CELLS: "579c414c22fe0775c539377488bd5b12f4501592",
    ANIMS: "9c446e6297aea062853fd9bd1c24c37b40bfd411",
}

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
CURSOR_SEQUENCE = 0
CURSOR_CELLS = (0, 1)
CURSOR_TILES = (0, 4)
CURSOR_FRAME_DURATIONS = (15, 15)
CURSOR_PALETTE_BANK = 15


def u16(data, off):
    return struct.unpack_from("<H", data, off)[0]


def u32(data, off):
    return struct.unpack_from("<I", data, off)[0]


def git_blob_sha(data):
    return hashlib.sha1(f"blob {len(data)}\0".encode("ascii") + data).hexdigest()


def verified_bytes(path):
    data = path.read_bytes()
    actual = git_blob_sha(data)
    expected = EXPECTED_GIT_BLOBS[path]
    if actual != expected:
        raise ValueError(f"{path}: Git blob {actual} != verified {expected}")
    return data


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
    data = verified_bytes(path)
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError(f"{path}: not a PNG")

    pos = len(PNG_SIGNATURE)
    ihdr = None
    idat = bytearray()
    while pos < len(data):
        length = struct.unpack_from(">I", data, pos)[0]
        chunk_type = data[pos + 4:pos + 8]
        payload_start = pos + 8
        payload_end = payload_start + length
        crc_end = payload_end + 4
        if crc_end > len(data):
            raise ValueError(f"{path}: truncated PNG")
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
    if (width, height) != (8, 456):
        raise ValueError(f"{path}: expected 8x456, got {width}x{height}")
    if bit_depth != 4 or color_type != 3:
        raise ValueError(f"{path}: expected 4-bit indexed PNG")
    if compression != 0 or filter_method != 0 or interlace != 0:
        raise ValueError(f"{path}: unsupported PNG encoding")

    packed_row_bytes = (width + 1) // 2
    raw = zlib.decompress(bytes(idat))
    if len(raw) != height * (packed_row_bytes + 1):
        raise ValueError(f"{path}: unexpected decompressed size")

    rows = []
    prev = bytearray(packed_row_bytes)
    off = 0
    for _ in range(height):
        filter_type = raw[off]
        scan = bytearray(raw[off + 1:off + 1 + packed_row_bytes])
        off += packed_row_bytes + 1
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
        row = []
        for value in recon:
            row.extend((value >> 4, value & 0x0F))
        rows.append(row[:width])
        prev = recon
    return rows


def validate_cursor_animation():
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

    seq = seq_base + CURSOR_SEQUENCE * 16
    total = u16(data, seq)
    frame_start_bytes = u32(data, seq + 12)
    if total != 2:
        raise ValueError(f"{ANIMS}: cursor sequence expected 2 frames, got {total}")

    cells = []
    durations = []
    for i in range(total):
        frame = frame_base + frame_start_bytes + i * 8
        frame_data_ptr = u32(data, frame)
        cells.append(u16(data, frame_data_base + frame_data_ptr))
        durations.append(u16(data, frame + 4))

    if tuple(cells) != CURSOR_CELLS or tuple(durations) != CURSOR_FRAME_DURATIONS:
        raise ValueError(
            f"{ANIMS}: cursor frames {tuple(cells)} durations {tuple(durations)} "
            f"!= expected {CURSOR_CELLS} / {CURSOR_FRAME_DURATIONS}"
        )


def validate_cursor_cells():
    data = verified_bytes(CELLS)
    if data[:4] != b"RECN":
        raise ValueError(f"{CELLS}: not an NCER")
    chunk = data.find(b"KBEC")
    if chunk < 0:
        raise ValueError(f"{CELLS}: missing CEBK block")

    base = chunk + 8
    cell_count = u16(data, base)
    bank_attributes = u16(data, base + 2)
    cell_data_base = base + u32(data, base + 4)
    mapping_mode = u32(data, base + 8)
    if cell_count != 18 or bank_attributes != 1 or mapping_mode != 0:
        raise ValueError(
            f"{CELLS}: unexpected cell bank "
            f"(cells={cell_count}, attr={bank_attributes}, mapping={mapping_mode})"
        )

    cell_struct_size = 16
    oam_base = cell_data_base + cell_count * cell_struct_size
    for cell_index, expected_tile in zip(CURSOR_CELLS, CURSOR_TILES):
        cell = cell_data_base + cell_index * cell_struct_size
        obj_count = u16(data, cell)
        oam_offset = u32(data, cell + 4)
        if obj_count != 1:
            raise ValueError(f"{CELLS}: cursor cell {cell_index} has {obj_count} OBJs")

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
            raise ValueError(f"{CELLS}: cursor cell {cell_index} is not 16x16")
        if tile != expected_tile or palette_bank != CURSOR_PALETTE_BANK:
            raise ValueError(
                f"{CELLS}: cursor cell {cell_index} uses tile {tile}, "
                f"palette {palette_bank}; expected {expected_tile}, "
                f"{CURSOR_PALETTE_BANK}"
            )
        if hflip or vflip:
            raise ValueError(f"{CELLS}: cursor cell {cell_index} unexpectedly flips")


def extract_16x16_frame(rows, tile_index):
    # Retail resource uses DS 1D 32K OBJ mapping. Four consecutive 8x8 tiles
    # form the 16x16 cell in TL, TR, BL, BR order.
    tiles = []
    for offset in range(4):
        source_tile = tile_index + offset
        src_y = source_tile * 8
        if src_y + 8 > len(rows):
            raise ValueError("cursor tile exceeds source image")
        tile = [rows[src_y + y][:8] for y in range(8)]
        tiles.append(tile)

    out = [[0] * 16 for _ in range(16)]
    for index, tile in enumerate(tiles):
        dst_x = (index % 2) * 8
        dst_y = (index // 2) * 8
        for y in range(8):
            out[dst_y + y][dst_x:dst_x + 8] = tile[y]

    packed = bytearray()
    for tile_y in range(2):
        for tile_x in range(2):
            for y in range(8):
                row = out[tile_y * 8 + y]
                for x in range(0, 8, 2):
                    lo = row[tile_x * 8 + x]
                    hi = row[tile_x * 8 + x + 1]
                    packed.append(lo | (hi << 4))
    if len(packed) != 128:
        raise ValueError(f"unexpected frame size {len(packed)}")
    return bytes(packed)


def load_nclr_bank(bank):
    data = verified_bytes(PALETTE)
    if data[:4] != b"RLCN":
        raise ValueError(f"{PALETTE}: not an NCLR")
    chunk = data.find(b"TTLP")
    if chunk < 0:
        raise ValueError(f"{PALETTE}: missing PLTT block")
    data_size = u32(data, chunk + 0x10)
    data_offset = u32(data, chunk + 0x14)
    start = chunk + 8 + data_offset
    raw = data[start:start + data_size]
    colors = [u16(raw, i) for i in range(0, len(raw), 2)]
    result = colors[bank * 16:(bank + 1) * 16]
    if len(result) != 16:
        raise ValueError(f"{PALETTE}: missing palette bank {bank}")
    return struct.pack("<16H", *result)


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("--tiles", "--palette"):
        raise SystemExit(
            "usage: make_hgss_pokegear_map_cursor.py "
            "(--tiles|--palette) OUTPUT"
        )

    validate_cursor_animation()
    validate_cursor_cells()
    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)

    if sys.argv[1] == "--tiles":
        rows = load_indexed_png4(SPRITES)
        output.write_bytes(b"".join(extract_16x16_frame(rows, tile)
                                    for tile in CURSOR_TILES))
    else:
        output.write_bytes(load_nclr_bank(CURSOR_PALETTE_BANK))


if __name__ == "__main__":
    main()
