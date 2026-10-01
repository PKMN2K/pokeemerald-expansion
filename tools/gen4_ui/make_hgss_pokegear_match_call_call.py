#!/usr/bin/env python3
from pathlib import Path
import binascii
import hashlib
import struct
import sys
import zlib


TILES = Path("graphics/gen4_ui/hgss_pokegear/verified/pgphone_skin0_call_tiles.png")
PALETTE = Path("graphics/gen4_ui/hgss_pokegear/verified/pgphone_skin0_call_palette.NCLR")
TILEMAP = Path("graphics/gen4_ui/hgss_pokegear/verified/pgphone_skin0_call_tilemap.NSCR")

EXPECTED_GIT_BLOBS = {
    TILES: "f1c86ec62e6764d295678d2714de8d942253160c",
    PALETTE: "932d99a855a27912da0f074acf3b42642a2c4254",
    TILEMAP: "46641999f72d5f1b7348a1042a7e9b8b02be766c",
}

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
SOURCE_WIDTH_TILES = 32
SOURCE_HEIGHT_TILES = 24
SOURCE_ROWS = tuple(range(4, 24))
# Preserve the retail left edge, the complete 27-tile call window at x=2..28,
# and the retail right edge. Only two repeated filler columns are omitted.
SOURCE_COLUMNS = tuple(range(0, 29)) + (31,)
OUTPUT_WIDTH_TILES = 32
OUTPUT_HEIGHT_TILES = 20
GBA_TILE_BASE = 0x80
GBA_PALETTE_BANK = 9


def git_blob_sha(data):
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def verify_source(path):
    data = path.read_bytes()
    expected = EXPECTED_GIT_BLOBS[path]
    actual = git_blob_sha(data)
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
    data = verify_source(path)
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
    if (width, height) != (256, 8):
        raise ValueError(f"{path}: expected 256x8, got {width}x{height}")
    if bit_depth != 4 or color_type != 3:
        raise ValueError(f"{path}: expected 4-bit indexed PNG")
    if compression != 0 or filter_method != 0 or interlace != 0:
        raise ValueError(f"{path}: unsupported PNG encoding")

    row_bytes = width // 2
    raw = zlib.decompress(bytes(idat))
    if len(raw) != height * (row_bytes + 1):
        raise ValueError(f"{path}: unexpected decompressed size")

    rows = []
    prev = bytearray(row_bytes)
    off = 0
    for _ in range(height):
        filter_type = raw[off]
        scan = bytearray(raw[off + 1:off + 1 + row_bytes])
        off += row_bytes + 1
        recon = bytearray(row_bytes)
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
            pixels.extend((value >> 4, value & 0x0F))
        rows.append(pixels)
        prev = recon
    return rows


def load_nclr(path):
    data = verify_source(path)
    if data[:4] != b"RLCN":
        raise ValueError(f"{path}: not an NCLR")
    chunk = data.find(b"TTLP")
    if chunk < 0:
        raise ValueError(f"{path}: missing PLTT chunk")
    data_size = struct.unpack_from("<I", data, chunk + 0x10)[0]
    data_offset = struct.unpack_from("<I", data, chunk + 0x14)[0]
    start = chunk + 8 + data_offset
    raw = data[start:start + data_size]
    if len(raw) != data_size or data_size % 2:
        raise ValueError(f"{path}: malformed palette payload")
    return [struct.unpack_from("<H", raw, i)[0] for i in range(0, len(raw), 2)]


def load_nscr(path):
    data = verify_source(path)
    if data[:4] != b"RCSN":
        raise ValueError(f"{path}: not an NSCR")
    chunk = data.find(b"NRCS")
    if chunk < 0:
        raise ValueError(f"{path}: missing SCRN chunk")
    width = struct.unpack_from("<H", data, chunk + 8)[0]
    height = struct.unpack_from("<H", data, chunk + 10)[0]
    payload_size = struct.unpack_from("<I", data, chunk + 16)[0]
    start = chunk + 20
    raw = data[start:start + payload_size]
    expected = (width // 8) * (height // 8) * 2
    if len(raw) != expected:
        raise ValueError(f"{path}: expected {expected} tilemap bytes, got {len(raw)}")
    return width, height, struct.unpack("<" + "H" * (len(raw) // 2), raw)


def source_entries():
    width, height, entries = load_nscr(TILEMAP)
    if (width, height) != (256, 192):
        raise ValueError(f"{TILEMAP}: expected 256x192, got {width}x{height}")
    banks = {(entry >> 12) & 0xF for entry in entries}
    if banks != {0}:
        raise ValueError(f"{TILEMAP}: call screen palette banks changed: {sorted(banks)}")
    max_tile = max(entry & 0x03FF for entry in entries)
    if max_tile != 19:
        raise ValueError(f"{TILEMAP}: expected max call tile 19, got {max_tile}")

    # The four discarded source rows are completely blank.
    for y in range(4):
        row = entries[y * SOURCE_WIDTH_TILES:(y + 1) * SOURCE_WIDTH_TILES]
        if any(entry != 0 for entry in row):
            raise ValueError(f"{TILEMAP}: source row {y} is no longer blank")

    # Columns 29 and 30 are repeated filler in every retained nonblank row.
    for y in range(15, 24):
        row = entries[y * SOURCE_WIDTH_TILES:(y + 1) * SOURCE_WIDTH_TILES]
        if not (row[28] == row[29] == row[30]):
            raise ValueError(f"{TILEMAP}: removable filler changed on source row {y}")

    return entries


def make_tiles():
    pixels = load_indexed_png4(TILES)
    source_entries()
    out = bytearray()
    for tile in range(20):
        tx = tile * 8
        for py in range(8):
            for px in range(0, 8, 2):
                lo = pixels[py][tx + px]
                hi = pixels[py][tx + px + 1]
                out.append(lo | (hi << 4))
    return bytes(out)


def make_tilemap():
    entries = source_entries()
    out = []
    for y in SOURCE_ROWS:
        row = entries[y * SOURCE_WIDTH_TILES:(y + 1) * SOURCE_WIDTH_TILES]
        for x in SOURCE_COLUMNS:
            entry = row[x]
            tile = entry & 0x03FF
            mapped = GBA_TILE_BASE + tile
            if mapped > 0x03FF:
                raise ValueError("mapped call tile exceeds GBA text-BG range")
            out.append((entry & 0x0C00) | (GBA_PALETTE_BANK << 12) | mapped)
        out.extend((0, 0))

    if len(out) != OUTPUT_WIDTH_TILES * OUTPUT_HEIGHT_TILES:
        raise ValueError("unexpected adapted call tilemap size")
    return struct.pack("<" + "H" * len(out), *out)


def make_palette():
    colors = load_nclr(PALETTE)
    bank = colors[:16]
    if len(bank) != 16:
        raise ValueError(f"{PALETTE}: missing retail bank 0")
    return struct.pack("<16H", *bank)


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("--tiles", "--tilemap", "--palette"):
        raise SystemExit(
            "usage: make_hgss_pokegear_match_call_call.py "
            "(--tiles|--tilemap|--palette) OUTPUT"
        )
    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)
    if sys.argv[1] == "--tiles":
        output.write_bytes(make_tiles())
    elif sys.argv[1] == "--tilemap":
        output.write_bytes(make_tilemap())
    else:
        output.write_bytes(make_palette())


if __name__ == "__main__":
    main()
