#!/usr/bin/env python3
"""Build the custom Forms page from authentic HGSS Pokédex pixels."""

import struct
import sys
import zlib

from make_hgss_pokedex_stats import PALETTE, crop, decode_member, paste, png_chunk

OUT_WIDTH = 240
OUT_HEIGHT = 160


def build_layout():
    m59 = decode_member(59)
    m65 = decode_member(65)

    # Native HGSS grid as the 240x160 canvas.
    out = crop(m59, 0, 0, OUT_WIDTH, OUT_HEIGHT)

    # Authentic rounded Pokédex header.
    paste(out, crop(m65, 8, 4, 229, 30), 8, 0)

    # Preserve the existing two-row 34px form-icon grid. These authentic HGSS
    # rules frame row 1, row 2, and the bottom navigation area without moving
    # or scaling any dynamic icon/cursor coordinates.
    rule = crop(m65, 16, 57, 240, 62)
    paste(out, rule, 16, 48)
    paste(out, rule, 16, 87)
    paste(out, crop(m65, 16, 128, 240, 134), 16, 137)

    return out


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
        f"{path}: HGSS-derived Forms layout, {OUT_WIDTH}x{OUT_HEIGHT}, "
        f"{len(PALETTE)} colors, {len(png)} bytes"
    )


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} OUTPUT.png")
    write_png(sys.argv[1], build_layout())


if __name__ == "__main__":
    main()
