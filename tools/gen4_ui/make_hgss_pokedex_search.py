#!/usr/bin/env python3
"""Build the HGSS Pokédex Search background from authentic member 068."""

import base64
import hashlib
import struct
import sys
import zlib

SOURCE_WIDTH = 256
SOURCE_HEIGHT = 192
OUT_WIDTH = 240
OUT_HEIGHT = 160
EXPECTED_SOURCE_SHA256 = "d62dae07b2e01b8b4c7b48ed068bccb314e36fb44ed618b1b611c10cfbfe2363"
EXPECTED_LAYOUT_SHA256 = "39d3d381c12389ba4b8ce9d3ff3043bc9a531c449d63f25b9c3570bf6a0fda55"

PALETTE = [
    (255, 172, 172),
    (123, 90, 180),
    (213, 180, 255),
    (255, 148, 49),
    (205, 222, 255),
    (172, 115, 255),
    (49, 115, 197),
    (255, 197, 148),
    (115, 123, 123),
    (255, 115, 115),
    (255, 255, 255),
    (90, 98, 98),
]

PIXELS_ZLIB_B64 = """eNrt2tFuozAQBVCGxnW7+///u6rURLuB2E4aMBufeR7EHONECN9pulkxtdWr9PHzF6+LuWvFFv5or7l7xfOred2++G+njvV2vQX23f9b68tP/rwC3fzxPcNm/PReqDR/r1H08m/++N+LddkAOffyn/r7T1/+zD+2P/Pzd/VvsBj/k//yQsL/hNe+sff/eSlH/f8776WR/TO//T+uf/D/P++//Pz8/Pz8x/Uf5ft3H/9Rzj96+Q9y/pW7+Y9x/tnPPx3h/Dt39PfPP+Tc1z/l1Uq5rZ7VN/H3yj8dwb/x+Xf5N7hYgUr/0lXv/9dfmefe+av8+ChUzD/oj8b+2Gmedf9Hseb8eH809sdO8zzmzw/3R2N/7DTPav6vdn1Kr9U/+vPn5/+7PsvXf15ff0d/NPbHTvPw8/Pz8/Pz8/Pz8/Pz8w/r9/1ncP+vYi3v194fjf2x0zz8a/7fxVrer70/Gvtjp3n41/ypdHla3q+9Pxr7Y6d5Hjn/zovr2/ujsT92mudG1XIIi/478wq1+MN1XqGWk6hmNFJ5/psLkJr405Ta+BdXbblialuAdGOesv9H+ZeX7uPn5+cf279d3i4a+2OneeT/5P/k/+T/fP/j53f+wc/Pz8/Pz8/Pz8/Pz88v/+f7j/yf/J/8n/yf/J/8n/yf/J/8n/wPPz8/v/yf/J/8n/yf/J/8n++f/PJ/zn/4+fn5+fn5+fn5+fn5+eX/fP+R/5P/k/+T/5P/k/+T/5P/k/+T/+Hn5+fn5+fn37wvBi/7n5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn/+V+v4Al2ZkBQ=="""


def png_chunk(chunk_type, data):
    crc = zlib.crc32(chunk_type + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + chunk_type + data + struct.pack(">I", crc)


def build_layout():
    pixels = zlib.decompress(base64.b64decode(PIXELS_ZLIB_B64))
    if len(pixels) != SOURCE_WIDTH * SOURCE_HEIGHT:
        raise SystemExit(f"bad member 068 payload size: {len(pixels)}")
    if hashlib.sha256(pixels).hexdigest() != EXPECTED_SOURCE_SHA256:
        raise SystemExit("HGSS member 068 source checksum mismatch")

    rows = [
        bytearray(pixels[y * SOURCE_WIDTH:y * SOURCE_WIDTH + OUT_WIDTH])
        for y in range(OUT_HEIGHT)
    ]

    flattened = b"".join(bytes(row) for row in rows)
    if hashlib.sha256(flattened).hexdigest() != EXPECTED_LAYOUT_SHA256:
        raise SystemExit("HGSS Search 240x160 layout checksum mismatch")
    return rows


def write_png(path, rows):
    scanlines = [bytes([0]) + bytes(row) for row in rows]
    palette = b"".join(bytes(rgb) for rgb in PALETTE)
    palette += bytes(768 - len(palette))

    png = bytearray(b"\x89PNG\r\n\x1a\n")
    png += png_chunk(b"IHDR", struct.pack(">IIBBBBB", OUT_WIDTH, OUT_HEIGHT, 8, 3, 0, 0, 0))
    png += png_chunk(b"PLTE", palette)
    png += png_chunk(b"IDAT", zlib.compress(b"".join(scanlines), 9))
    png += png_chunk(b"IEND", b"")

    with open(path, "wb") as f:
        f.write(png)

    print(
        f"{path}: authentic HGSS member 068 Search layout, "
        f"{OUT_WIDTH}x{OUT_HEIGHT}, {len(PALETTE)} colors, {len(png)} bytes"
    )


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} OUTPUT.png")
    write_png(sys.argv[1], build_layout())


if __name__ == "__main__":
    main()
