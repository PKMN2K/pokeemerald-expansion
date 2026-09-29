#!/usr/bin/env python3
from pathlib import Path
import binascii
import struct
import sys
import zlib


TILES = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_tiles.png")
PALETTE = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_palette.NCLR")
TILEMAP = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_tilemap.NSCR")

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


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
    if (width, height) != (256, 32):
        raise ValueError(f"{path}: expected 256x32, got {width}x{height}")
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

    data_size = struct.unpack_from("<I", data, chunk + 0x10)[0]
    data_offset = struct.unpack_from("<I", data, chunk + 0x14)[0]
    start = chunk + 8 + data_offset
    raw = data[start:start + data_size]
    if len(raw) != data_size or data_size % 2:
        raise ValueError(f"{path}: malformed palette payload")

    return [
        struct.unpack_from("<H", raw, i)[0]
        for i in range(0, len(raw), 2)
    ]


def load_nscr(path):
    data = path.read_bytes()
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
        raise ValueError(
            f"{path}: expected {expected} tilemap bytes, got {len(raw)}"
        )

    entries = struct.unpack("<" + "H" * (len(raw) // 2), raw)
    return width, height, entries


def bgr555_to_rgb(value):
    r5 = value & 0x1F
    g5 = (value >> 5) & 0x1F
    b5 = (value >> 10) & 0x1F
    return (
        (r5 << 3) | (r5 >> 2),
        (g5 << 3) | (g5 >> 2),
        (b5 << 3) | (b5 >> 2),
    )


def png_chunk(chunk_type, payload):
    return (
        struct.pack(">I", len(payload))
        + chunk_type
        + payload
        + struct.pack(">I", binascii.crc32(chunk_type + payload) & 0xFFFFFFFF)
    )


def write_indexed_png4(path, rows, palette_rgb):
    height = len(rows)
    width = len(rows[0]) if rows else 0
    if width % 2:
        raise ValueError("4-bit PNG writer requires an even width")
    if len(palette_rgb) > 16:
        raise ValueError("4-bit PNG cannot contain more than 16 colors")

    plte = bytearray()
    for r, g, b in palette_rgb:
        plte.extend((r, g, b))
    while len(plte) < 16 * 3:
        plte.extend((0, 0, 0))

    raw = bytearray()
    for row in rows:
        if len(row) != width:
            raise ValueError("inconsistent PNG row width")
        raw.append(0)  # PNG filter type: None
        for x in range(0, width, 2):
            hi = row[x]
            lo = row[x + 1]
            if hi > 15 or lo > 15:
                raise ValueError("pixel index exceeds 4-bit range")
            raw.append((hi << 4) | lo)

    ihdr = struct.pack(">IIBBBBB", width, height, 4, 3, 0, 0, 0)
    encoded = (
        PNG_SIGNATURE
        + png_chunk(b"IHDR", ihdr)
        + png_chunk(b"PLTE", bytes(plte))
        + png_chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + png_chunk(b"IEND", b"")
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(encoded)


def main():
    if len(sys.argv) != 2:
        raise SystemExit(
            "usage: make_hgss_pokegear_app_switch.py OUTPUT.png"
        )

    out_path = Path(sys.argv[1])
    source_pixels = load_indexed_png4(TILES)
    palettes = load_nclr(PALETTE)
    width, height, entries = load_nscr(TILEMAP)
    if (width, height) != (256, 96):
        raise ValueError(
            f"{TILEMAP}: expected 256x96, got {width}x{height}"
        )

    # Reconstruct the retail normal-state app-switch strip from rows 0..3 of
    # member 54.  Each NSCR entry selects a tile, flip state, and palette bank.
    retail_values = [[0] * 256 for _ in range(32)]
    for tile_y in range(4):
        for tile_x in range(32):
            entry = entries[tile_y * 32 + tile_x]
            tile = entry & 0x03FF
            hflip = bool(entry & 0x0400)
            vflip = bool(entry & 0x0800)
            bank = (entry >> 12) & 0xF

            src_tile_x = (tile % 32) * 8
            src_tile_y = (tile // 32) * 8
            if src_tile_y + 7 >= len(source_pixels):
                raise ValueError(
                    f"{TILEMAP}: tile {tile} falls outside member-48 graphics"
                )

            for py in range(8):
                sy = 7 - py if vflip else py
                for px in range(8):
                    sx = 7 - px if hflip else px
                    color_index = source_pixels[src_tile_y + sy][src_tile_x + sx]
                    palette_index = bank * 16 + color_index
                    if palette_index >= len(palettes):
                        raise ValueError(
                            f"{PALETTE}: palette index {palette_index} out of range"
                        )
                    retail_values[tile_y * 8 + py][tile_x * 8 + px] = palettes[
                        palette_index
                    ]

    # Remove only the empty 8-pixel side margins from the 256-pixel DS strip.
    cropped_values = [row[8:248] for row in retail_values]

    # Reindex the exact retail BGR555 colors into one GBA-compatible 4bpp bank.
    color_to_index = {}
    palette_values = []
    indexed_rows = []
    for row in cropped_values:
        indexed_row = []
        for value in row:
            if value not in color_to_index:
                if len(palette_values) >= 16:
                    raise ValueError("retail crop exceeds 16 colors")
                color_to_index[value] = len(palette_values)
                palette_values.append(value)
            indexed_row.append(color_to_index[value])
        indexed_rows.append(indexed_row)

    palette_rgb = [bgr555_to_rgb(value) for value in palette_values]
    write_indexed_png4(out_path, indexed_rows, palette_rgb)


if __name__ == "__main__":
    main()
