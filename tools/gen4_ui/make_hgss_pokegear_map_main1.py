#!/usr/bin/env python3
"""
Adapt the exact retail HGSS PokeGear Map MAIN_1 layer for pokeemerald.

Retail HGSS loads pgmap_gra members 63/66 to GF_BG_LYR_MAIN_1 and copies the
top 32x24 cells of member 67 to that layer. The GBA adapter preserves those
retail pixels, flips and screen positions. Only tile IDs and the two palette
banks used by the retail 32x24 region are remapped into free GBA BG1 slots.
"""

from pathlib import Path
import binascii
import hashlib
import struct
import sys
import zlib


ROOT = Path("graphics/gen4_ui/hgss_pokegear/verified/map")
PALETTE = ROOT / "pgmap_gra_00000063.NCLR"
TILES = ROOT / "pgmap_gra_00000066.png"
TILEMAP = ROOT / "pgmap_gra_00000067.NSCR"

EXPECTED_GIT_BLOBS = {
    PALETTE: "d55d134d59e48ce4fbf177d3024307b06a0b1fa4",
    TILES: "0020479f6cf2100d6eeb8fdaa3d456b41e1897db",
    TILEMAP: "0f43ed11f57f5ec3fee9c9adfb5af4867cb070d1",
}

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
SOURCE_WIDTH_TILES = 32
SOURCE_HEIGHT_TILES = 32
RETAIL_COPY_HEIGHT_TILES = 24
OUTPUT_HEIGHT_TILES = 32
GBA_TILE_BASE = 0x1C0

# Only these two source banks occur in the retail 32x24 MAIN_1 copy.
SOURCE_TO_GBA_PALETTE = {
    5: 5,
    7: 6,
}


def git_blob_sha(data):
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def verify_source(path):
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
    data = verify_source(path)
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
    if (width, height) != (256, 32):
        raise ValueError(f"{path}: expected 256x32, got {width}x{height}")
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


def retail_entries():
    width, height, entries = load_nscr(TILEMAP)
    if (width, height) != (256, 256):
        raise ValueError(f"{TILEMAP}: expected 256x256, got {width}x{height}")
    if len(entries) != SOURCE_WIDTH_TILES * SOURCE_HEIGHT_TILES:
        raise ValueError(f"{TILEMAP}: unexpected entry count {len(entries)}")

    visible = entries[:SOURCE_WIDTH_TILES * RETAIL_COPY_HEIGHT_TILES]
    banks = {(entry >> 12) & 0xF for entry in visible}
    if banks != set(SOURCE_TO_GBA_PALETTE):
        raise ValueError(f"{TILEMAP}: MAIN_1 palette banks changed: {sorted(banks)}")
    if max(entry & 0x03FF for entry in visible) != 22:
        raise ValueError(f"{TILEMAP}: expected max MAIN_1 tile 22")
    return visible


def make_tiles():
    pixels = load_indexed_png4(TILES)
    max_tile = max(entry & 0x03FF for entry in retail_entries())

    # Retail tile 0 fills the large map opening. It must remain fully
    # transparent so the live affine region map can show through on GBA.
    tile0 = []
    for py in range(8):
        tile0.extend(pixels[py][:8])
    if any(tile0):
        raise ValueError(f"{TILES}: retail MAIN_1 tile 0 is no longer transparent")

    out = bytearray()
    for tile in range(max_tile + 1):
        tx = (tile % 32) * 8
        ty = (tile // 32) * 8
        for py in range(8):
            for px in range(0, 8, 2):
                lo = pixels[ty + py][tx + px]
                hi = pixels[ty + py][tx + px + 1]
                out.append(lo | (hi << 4))
    return bytes(out)


def remap_entry(entry):
    tile = entry & 0x03FF
    source_bank = (entry >> 12) & 0xF
    mapped_tile = GBA_TILE_BASE + tile
    if mapped_tile > 0x03FF:
        raise ValueError("mapped Map MAIN_1 tile exceeds GBA text-BG range")
    return (
        (entry & 0x0C00)
        | (SOURCE_TO_GBA_PALETTE[source_bank] << 12)
        | mapped_tile
    )


def make_tilemap():
    visible = retail_entries()
    out = [remap_entry(entry) for entry in visible]

    # GBA BG1 uses a 32x32 text screen while retail copies exactly 32x24.
    # Extend with the exact transparent retail opening tile, not new artwork.
    transparent = remap_entry(5 << 12)
    out.extend([transparent] * (SOURCE_WIDTH_TILES * (OUTPUT_HEIGHT_TILES - RETAIL_COPY_HEIGHT_TILES)))
    if len(out) != SOURCE_WIDTH_TILES * OUTPUT_HEIGHT_TILES:
        raise ValueError("unexpected adapted MAIN_1 tilemap size")
    return struct.pack("<" + "H" * len(out), *out)


def make_palette():
    colors = load_nclr(PALETTE)
    out = []
    for source_bank in (5, 7):
        bank = colors[source_bank * 16:(source_bank + 1) * 16]
        if len(bank) != 16:
            raise ValueError(f"{PALETTE}: missing retail palette bank {source_bank}")
        out.extend(bank)
    return struct.pack("<" + "H" * len(out), *out)


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("--tiles", "--tilemap", "--palette"):
        raise SystemExit(
            "usage: make_hgss_pokegear_map_main1.py "
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
