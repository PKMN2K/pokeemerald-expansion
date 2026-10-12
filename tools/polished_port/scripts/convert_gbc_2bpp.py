#!/usr/bin/env python3
"""Losslessly expand Game Boy 2bpp tile pixels into GBA 4bpp tiles.

Input must be *decompressed*, planar 16-byte-per-8x8 Game Boy tiles.
No palettes are baked into the output. Each pixel index (0..3) survives
unaltered; GBA palette-bank selection is supplied by metatile attributes.

Usage:
  python3 tools/polished_port/scripts/convert_gbc_2bpp.py source.2bpp output.4bpp
The source file must be produced from the pinned Polished Crystal
graphics build, with original tile-bank ordering preserved.
"""
import argparse
from pathlib import Path


def convert(data: bytes) -> bytes:
    if len(data) % 16:
        raise ValueError("GBC 2bpp tile data must contain whole 16-byte tiles")
    output = bytearray()
    for offset in range(0, len(data), 16):
        tile = data[offset:offset + 16]
        # GBA 4bpp is row-major, low nibble = leftmost pixel.
        for row in range(8):
            lo, hi = tile[2 * row], tile[2 * row + 1]
            for x in (0, 2, 4, 6):
                def pixel(col):
                    bit = 7 - col
                    return ((lo >> bit) & 1) | (((hi >> bit) & 1) << 1)
                output.append(pixel(x) | (pixel(x + 1) << 4))
    assert len(output) == (len(data) // 16) * 32
    return bytes(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data = convert(args.source.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"Converted {len(data) // 32} tiles; palette banks not yet assigned")


if __name__ == "__main__":
    main()
