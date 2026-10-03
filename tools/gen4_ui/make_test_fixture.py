#!/usr/bin/env python3
"""Generate the deterministic 256x192 indexed PNG used by the Gen 4 UI test."""

import struct
import sys
import zlib

WIDTH = 256
HEIGHT = 192

PALETTE = [
    (232, 240, 240),
    (32, 48, 72),
    (64, 96, 120),
    (96, 144, 168),
    (128, 176, 192),
    (160, 200, 208),
    (192, 216, 216),
    (224, 232, 232),
    (240, 192, 72),
    (224, 128, 64),
    (200, 72, 72),
    (120, 184, 104),
    (80, 144, 96),
    (80, 112, 160),
    (128, 96, 176),
    (248, 248, 248),
]


def png_chunk(chunk_type, data):
    crc = zlib.crc32(chunk_type + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + chunk_type + data + struct.pack(">I", crc)


def pixel(x, y):
    # Rightmost 16 px and bottom 32 px are deliberately distinct so scrolling
    # can prove the complete 256x192 DS-sized source survived the import.
    if x >= 240:
        return 8 + ((y // 16) & 3)
    if y >= 160:
        return 12 + ((x // 32) & 3)
    if x % 32 == 0 or y % 32 == 0:
        return 2
    return 0 if ((x // 8 + y // 8) & 1) == 0 else 1


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} OUTPUT.png")

    rows = []
    for y in range(HEIGHT):
        rows.append(bytes([0]) + bytes(pixel(x, y) for x in range(WIDTH)))

    png = bytearray(b"\x89PNG\r\n\x1a\n")
    png += png_chunk(b"IHDR", struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 3, 0, 0, 0))
    png += png_chunk(b"PLTE", b"".join(bytes(rgb) for rgb in PALETTE))
    png += png_chunk(b"IDAT", zlib.compress(b"".join(rows), 9))
    png += png_chunk(b"IEND", b"")

    with open(sys.argv[1], "wb") as f:
        f.write(png)

    print(f"{sys.argv[1]}: wrote {len(png)} bytes")


if __name__ == "__main__":
    main()
