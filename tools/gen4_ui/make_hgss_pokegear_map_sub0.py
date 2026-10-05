#!/usr/bin/env python3
"""
Adapt the exact retail HGSS PokeGear Map SUB_0 layer for pokeemerald.

The three inputs are byte-for-byte copies of the members loaded by retail HGSS
in FlyMap_LoadBGGraphics:
  palette  pgmap_gra_00000062.NCLR
  tiles    pgmap_gra_00000064.NCGR (decompiled as indexed PNG)
  tilemap  pgmap_gra_00000065.NSCR

No replacement art is drawn here. The adapter preserves the retail tile pixels,
palette colors, flips, and screen composition. It only remaps tile IDs into a
free GBA BG1 character range and extends the retail uniform bottom row to fill
the 32x32 GBA text-screen buffer used by the existing Region Map.
"""

from pathlib import Path
import binascii
import hashlib
import struct
import sys
import zlib


ROOT = Path("graphics/gen4_ui/hgss_pokegear/verified/map")
TILES = ROOT / "pgmap_gra_00000064.png"
PALETTE = ROOT / "pgmap_gra_00000062.NCLR"
TILEMAP = ROOT / "pgmap_gra_00000065.NSCR"

EXPECTED_GIT_BLOBS = {
    TILES: "db869c4590aae5e32170edf1abb14d5104900cd3",
    PALETTE: "907fcbae7aef50adf87c6eb1323ab6f2b83c0e77",
    TILEMAP: "0b0e6ee70d201bf1241e612b1a16c657e5a25c17",
}

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
SOURCE_WIDTH_TILES = 32
SOURCE_HEIGHT_TILES = 24
OUTPUT_WIDTH_TILES = 32
OUTPUT_HEIGHT_TILES = 32
SOURCE_PALETTE_BANK = 4
GBA_PALETTE_BANK = 4
GBA_TILE_BASE = 0x1E0


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


def retail_entries():
    width, height, entries = load_nscr(TILEMAP)
    if (width, height) != (256, 192):
        raise ValueError(f"{TILEMAP}: expected 256x192, got {width}x{height}")
    if len(entries) != SOURCE_WIDTH_TILES * SOURCE_HEIGHT_TILES:
        raise ValueError(f"{TILEMAP}: unexpected entry count {len(entries)}")

    banks = {(entry >> 12) & 0xF for entry in entries}
    if banks != {SOURCE_PALETTE_BANK}:
        raise ValueError(f"{TILEMAP}: expected palette bank 4, got {sorted(banks)}")
    if max(entry & 0x03FF for entry in entries) != 8:
        raise ValueError(f"{TILEMAP}: expected max tile 8")
    if min(entry & 0x03FF for entry in entries) != 1:
        raise ValueError(f"{TILEMAP}: expected minimum used tile 1")
    return entries


def make_tiles():
    pixels = load_indexed_png4(TILES)
    max_tile = max(entry & 0x03FF for entry in retail_entries())

    out = bytearray()
    for tile in range(max_tile + 1):
        tx = tile * 8
        for py in range(8):
            for px in range(0, 8, 2):
                lo = pixels[py][tx + px]
                hi = pixels[py][tx + px + 1]
                out.append(lo | (hi << 4))
    return bytes(out)


def remap_entry(entry):
    tile = entry & 0x03FF
    mapped = GBA_TILE_BASE + tile
    if mapped > 0x03FF:
        raise ValueError("mapped Map SUB_0 tile exceeds GBA text-BG range")
    return (entry & 0x0C00) | (GBA_PALETTE_BANK << 12) | mapped


def make_tilemap():
    entries = retail_entries()
    out = [remap_entry(entry) for entry in entries]

    # The retail rows 8..23 are the same uniform pale field. Extend that exact
    # final retail row to fill the 32x32 GBA map buffer used while BG1 scrolls.
    final_row = entries[-SOURCE_WIDTH_TILES:]
    if len(set(final_row)) != 1 or (final_row[0] & 0x03FF) != 8:
        raise ValueError(f"{TILEMAP}: final row is no longer the retail uniform field")
    mapped_final_row = [remap_entry(entry) for entry in final_row]
    for _ in range(OUTPUT_HEIGHT_TILES - SOURCE_HEIGHT_TILES):
        out.extend(mapped_final_row)

    if len(out) != OUTPUT_WIDTH_TILES * OUTPUT_HEIGHT_TILES:
        raise ValueError("unexpected adapted Map SUB_0 tilemap size")
    return struct.pack("<" + "H" * len(out), *out)


def make_palette():
    colors = load_nclr(PALETTE)
    start = SOURCE_PALETTE_BANK * 16
    bank = colors[start:start + 16]
    if len(bank) != 16:
        raise ValueError(f"{PALETTE}: missing retail palette bank 4")
    return struct.pack("<16H", *bank)


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("--tiles", "--tilemap", "--palette"):
        raise SystemExit(
            "usage: make_hgss_pokegear_map_sub0.py "
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
