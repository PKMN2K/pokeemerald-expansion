#!/usr/bin/env python3
"""Extract the live HGSS Pokédex counter OBJ tiles into a compact sheet.

The original HGSS interface PNG is 32x512 pixels (4x64 8x8 tiles) and mixes
many unrelated UI objects. The HGSS scrolling list needs only:
- tiles 64..127: SEEN / OWN labels
- tiles 160..175: HOENN / NATIONAL labels
- tiles 176..195: decimal digits used by the counters

Those ranges are copied without redrawing or recoloring and packed into 100
contiguous 4bpp OBJ tiles. Both normal and decapped source PNGs are supported.

Compact layout:
- 0..31   SEEN
- 32..63  OWN
- 64..71  HOENN
- 72..79  NATIONAL
- 80..99  digits 0..9 (two tiles per digit)
"""

import argparse
import struct
import zlib

EXPECTED_WIDTH = 32
EXPECTED_HEIGHT = 512
SOURCE_TILE_RANGES = ((64, 128), (160, 196))
EXPECTED_TILE_COUNT = 100


def paeth(a, b, c):
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def read_indexed_png(path):
    with open(path, "rb") as f:
        data = f.read()

    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise SystemExit(f"{path}: not a PNG")

    pos = 8
    width = height = bit_depth = color_type = interlace = None
    idat = bytearray()

    while pos < len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        chunk_type = data[pos + 4:pos + 8]
        payload = data[pos + 8:pos + 8 + length]
        pos += 12 + length

        if chunk_type == b"IHDR":
            width, height, bit_depth, color_type, _, _, interlace = struct.unpack(
                ">IIBBBBB", payload
            )
        elif chunk_type == b"IDAT":
            idat.extend(payload)
        elif chunk_type == b"IEND":
            break

    if (width, height) != (EXPECTED_WIDTH, EXPECTED_HEIGHT):
        raise SystemExit(
            f"{path}: expected {EXPECTED_WIDTH}x{EXPECTED_HEIGHT}, "
            f"got {width}x{height}"
        )
    if bit_depth != 8 or color_type != 3 or interlace != 0:
        raise SystemExit(
            f"{path}: expected non-interlaced 8-bit indexed PNG "
            f"(depth={bit_depth}, type={color_type}, interlace={interlace})"
        )

    raw = zlib.decompress(bytes(idat))
    stride = width
    expected = height * (stride + 1)
    if len(raw) != expected:
        raise SystemExit(f"{path}: decoded size {len(raw)} != {expected}")

    rows = []
    prev = bytearray(stride)
    offset = 0
    for _ in range(height):
        filter_type = raw[offset]
        scan = bytearray(raw[offset + 1:offset + 1 + stride])
        offset += stride + 1

        recon = bytearray(stride)
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
                recon[x] = (value + paeth(left, up, up_left)) & 0xFF
            else:
                raise SystemExit(f"{path}: unsupported PNG filter {filter_type}")

        if any(pixel > 15 for pixel in recon):
            raise SystemExit(f"{path}: interface sheet uses palette index > 15")

        rows.append(recon)
        prev = recon

    return rows


def extract_tile(rows, tile_index):
    tiles_per_row = EXPECTED_WIDTH // 8
    tx = tile_index % tiles_per_row
    ty = tile_index // tiles_per_row
    pixels = []
    for y in range(8):
        start = tx * 8
        pixels.extend(rows[ty * 8 + y][start:start + 8])
    return pixels


def encode_4bpp(pixels):
    out = bytearray()
    for i in range(0, 64, 2):
        out.append((pixels[i] & 0xF) | ((pixels[i + 1] & 0xF) << 4))
    return bytes(out)


def build_tiles(source):
    rows = read_indexed_png(source)
    tile_indices = []
    for start, end in SOURCE_TILE_RANGES:
        tile_indices.extend(range(start, end))

    if len(tile_indices) != EXPECTED_TILE_COUNT:
        raise SystemExit("counter tile-range definition is invalid")

    return b"".join(encode_4bpp(extract_tile(rows, i)) for i in tile_indices)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source")
    parser.add_argument("output")
    args = parser.parse_args()

    data = build_tiles(args.source)
    expected_size = EXPECTED_TILE_COUNT * 32
    if len(data) != expected_size:
        raise SystemExit(f"counter sheet has size {len(data)}, expected {expected_size}")

    with open(args.output, "wb") as f:
        f.write(data)

    print(
        f"{args.output}: compact HGSS Pokédex counter OBJ sheet, "
        f"{EXPECTED_TILE_COUNT} tiles / {len(data)} bytes"
    )


if __name__ == "__main__":
    main()
