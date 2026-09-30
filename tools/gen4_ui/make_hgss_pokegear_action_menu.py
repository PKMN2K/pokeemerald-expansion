#!/usr/bin/env python3
"""Build the authentic HGSS Phone context-menu chrome from sbox_gra.

The retail Phone contact context menu is the generic HGSS TouchscreenListMenu
with three entries (Call / Sort / Quit). Its border/button art is the shared
sbox_gra resource. This generator preserves all 27 retail 8x8 indexed tiles
and the complete 16-color source palette exactly in source order.

No scaling, redrawing, recoloring, interpolation, or tile synthesis occurs.
"""

from pathlib import Path
import argparse
import binascii
import hashlib
import struct
import zlib


SOURCE = Path("graphics/gen4_ui/hgss_pokegear/verified/sbox_gra.png")
EXPECTED_GIT_BLOB = "177f5c242ba0eacc1f74dead410c526bc5a46267"
EXPECTED_SOURCE_SHA256 = "5cd355b6456624d9657def9d65b190ff2bedba789e7e44606f1df4affb6d8643"
EXPECTED_TILES_SHA256 = "90b7f49abadd0d748af5ea360695679a8d81fd11552c1582fd93aad4ca8111c0"
EXPECTED_PALETTE_SHA256 = "c38bf510c31c48bf2583bacf5120787f7768b7479d6f2bfcac940d3dd99d8739"

WIDTH = 24
HEIGHT = 72
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def git_blob_sha(data):
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


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


def load_source():
    data = SOURCE.read_bytes()
    if git_blob_sha(data) != EXPECTED_GIT_BLOB:
        raise SystemExit(f"{SOURCE}: retail Git blob identity mismatch")
    if hashlib.sha256(data).hexdigest() != EXPECTED_SOURCE_SHA256:
        raise SystemExit(f"{SOURCE}: retail SHA-256 mismatch")
    if not data.startswith(PNG_SIGNATURE):
        raise SystemExit(f"{SOURCE}: not a PNG")

    pos = len(PNG_SIGNATURE)
    ihdr = None
    palette = None
    idat = bytearray()

    while pos < len(data):
        if pos + 12 > len(data):
            raise SystemExit(f"{SOURCE}: truncated PNG chunk")

        length = struct.unpack_from(">I", data, pos)[0]
        chunk_type = data[pos + 4:pos + 8]
        payload_start = pos + 8
        payload_end = payload_start + length
        crc_end = payload_end + 4
        if crc_end > len(data):
            raise SystemExit(f"{SOURCE}: truncated {chunk_type!r} chunk")

        payload = data[payload_start:payload_end]
        expected_crc = struct.unpack_from(">I", data, payload_end)[0]
        actual_crc = binascii.crc32(chunk_type + payload) & 0xFFFFFFFF
        if expected_crc != actual_crc:
            raise SystemExit(f"{SOURCE}: invalid {chunk_type!r} CRC")

        if chunk_type == b"IHDR":
            ihdr = struct.unpack(">IIBBBBB", payload)
        elif chunk_type == b"PLTE":
            palette = payload
        elif chunk_type == b"IDAT":
            idat.extend(payload)
        elif chunk_type == b"IEND":
            break

        pos = crc_end

    if ihdr is None or palette is None:
        raise SystemExit(f"{SOURCE}: missing IHDR/PLTE")

    width, height, bit_depth, color_type, compression, filter_method, interlace = ihdr
    if (width, height) != (WIDTH, HEIGHT):
        raise SystemExit(f"{SOURCE}: expected {WIDTH}x{HEIGHT}, got {width}x{height}")
    if bit_depth != 4 or color_type != 3:
        raise SystemExit(
            f"{SOURCE}: expected 4-bit indexed PNG, got "
            f"bit_depth={bit_depth}, color_type={color_type}"
        )
    if compression != 0 or filter_method != 0 or interlace != 0:
        raise SystemExit(f"{SOURCE}: unsupported PNG encoding")
    if len(palette) != 16 * 3:
        raise SystemExit(f"{SOURCE}: expected exactly 16 PLTE colors")

    packed_row_bytes = (width + 1) // 2
    raw = zlib.decompress(bytes(idat))
    if len(raw) != height * (packed_row_bytes + 1):
        raise SystemExit(f"{SOURCE}: unexpected decompressed size")

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
                raise SystemExit(f"{SOURCE}: unsupported PNG filter {filter_type}")

        pixels = []
        for value in recon:
            pixels.extend((value >> 4, value & 0x0F))
        rows.append(pixels[:width])
        prev = recon

    colors = [
        tuple(palette[i:i + 3])
        for i in range(0, len(palette), 3)
    ]
    return rows, colors


def encode_tiles(rows):
    out = bytearray()
    for ty in range(0, HEIGHT, 8):
        for tx in range(0, WIDTH, 8):
            for y in range(8):
                for x in range(0, 8, 2):
                    lo = rows[ty + y][tx + x]
                    hi = rows[ty + y][tx + x + 1]
                    out.append((lo & 0xF) | ((hi & 0xF) << 4))

    data = bytes(out)
    if len(data) != 27 * 32:
        raise SystemExit(f"expected 27 4bpp tiles, got {len(data)} bytes")
    if hashlib.sha256(data).hexdigest() != EXPECTED_TILES_SHA256:
        raise SystemExit("authentic sbox_gra tile output checksum mismatch")
    return data


def rgb555(rgb):
    r, g, b = rgb
    return (
        min(31, (r + 4) // 8)
        | (min(31, (g + 4) // 8) << 5)
        | (min(31, (b + 4) // 8) << 10)
    )


def encode_palette(colors):
    if len(colors) != 16:
        raise SystemExit(f"expected 16 source colors, got {len(colors)}")
    data = b"".join(struct.pack("<H", rgb555(color)) for color in colors)
    if hashlib.sha256(data).hexdigest() != EXPECTED_PALETTE_SHA256:
        raise SystemExit("authentic sbox_gra palette output checksum mismatch")
    return data


def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--tiles", action="store_true")
    group.add_argument("--palette", action="store_true")
    parser.add_argument("output")
    args = parser.parse_args()

    rows, colors = load_source()
    if args.tiles:
        data = encode_tiles(rows)
        label = "27 exact retail 8x8 menu tiles"
    else:
        data = encode_palette(colors)
        label = "exact retail 16-color menu palette"

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(data)
    print(f"{output}: authentic HGSS sbox_gra, {label}")


if __name__ == "__main__":
    main()
