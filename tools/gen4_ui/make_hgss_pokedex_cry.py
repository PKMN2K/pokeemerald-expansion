#!/usr/bin/env python3
"""Rebuild the authentic HGSS member 066 background for the GBA Cry page."""

import base64
import hashlib
import struct
import sys
import zlib

SOURCE_WIDTH = 256
SOURCE_HEIGHT = 192
OUT_WIDTH = 240
OUT_HEIGHT = 160
EXPECTED_SOURCE_SHA256 = "2d691402b6f4cb8f57c6b396201ce6f7842a60d68247c42432cdfcd3dfe8c869"
EXPECTED_LAYOUT_SHA256 = "6abe916e3e9c8dcdcea55a47c4d128b4aed9f0be5d842c1a6c2ec77458878371"

PALETTE = [
    (255, 172, 172),
    (123, 90, 180),
    (213, 180, 255),
    (205, 222, 255),
    (255, 148, 49),
    (49, 115, 197),
    (255, 197, 148),
    (255, 255, 255),
    (172, 115, 255),
    (197, 106, 32),
]

PIXELS_ZLIB_B64 = """eNrt3OFugjAQAGA4XNne/4XnFDKnLCHxJMh9FxJ/WAsfHF7Fpl33dPQHacdf0B+Voi/uP5+B+xgKxWkhBYah2BlI9mcl5kb9PiTAs/zTR0qcYpt+zyegtUz/R1LENv1e/I2/kD8u2x9/e94/dVnYH8fxP5afVXUv2z/Xq58jTvZfDzaW/Uv1d1Xdz/XPBTte4o8h9u2f7fn9/m57zv8591/Q77xF3e+/q71s/Yvs6/9m45/p8kdB/8L4l7/0799E/xs+/8j1v93zr1bXf3n+eeev9vy7VfZP+lt/+yfGcfxs6yK1XfZ+F/vr+Cv7uxX+bNde2o1juf/1++LzGvj5+fn5+fn5+W/fLB7yn5+fn5+fn5+fn7+u/+vgwS+E/OdX//j5+fn5+fn5+fn5+fn5+fn5+fmP3c78P/nPz8/Pz7/qc/3dGpo7cfXRb+OPaRHVuXju5DUe1nZ9sX/e305e+bfz7zT45T+/+mf8Y/zLz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz89f2m/+n/zn5+e/hvU/rP8h6ob73/2v/vPz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz879jO/P/5D8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz//cdp9Awp4uWE="""


def png_chunk(chunk_type, data):
    crc = zlib.crc32(chunk_type + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + chunk_type + data + struct.pack(">I", crc)


def build_layout():
    pixels = zlib.decompress(base64.b64decode(PIXELS_ZLIB_B64))
    if len(pixels) != SOURCE_WIDTH * SOURCE_HEIGHT:
        raise SystemExit(f"bad member 066 payload size: {len(pixels)}")
    if hashlib.sha256(pixels).hexdigest() != EXPECTED_SOURCE_SHA256:
        raise SystemExit("HGSS member 066 source checksum mismatch")

    rows = [
        bytearray(pixels[y * SOURCE_WIDTH:y * SOURCE_WIDTH + OUT_WIDTH])
        for y in range(OUT_HEIGHT)
    ]

    flattened = b"".join(bytes(row) for row in rows)
    if hashlib.sha256(flattened).hexdigest() != EXPECTED_LAYOUT_SHA256:
        raise SystemExit("HGSS Cry 240x160 layout checksum mismatch")
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
        f"{path}: authentic HGSS member 066 Cry layout, "
        f"{OUT_WIDTH}x{OUT_HEIGHT}, {len(PALETTE)} colors, {len(png)} bytes"
    )


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} OUTPUT.png")
    write_png(sys.argv[1], build_layout())


if __name__ == "__main__":
    main()
