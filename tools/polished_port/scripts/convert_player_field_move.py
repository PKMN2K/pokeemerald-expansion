#!/usr/bin/env python3
"""Build a faithful Polished Crystal field-move sheet.

Polished Crystal v3.2.3 has no dedicated Chris field-move poses. This converter
places the authentic south/north/west standing frames from chris.png onto
Expansion-compatible 32x32 canvases. The custom C animation table holds the
appropriate directional frame for the normal field-move duration.
"""

from __future__ import annotations
import argparse
from pathlib import Path
from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png

def convert(source: Path, output: Path) -> None:
    width, height, src = _read_png_2bit_grayscale(source)
    if (width, height) != (16, 96):
        raise ValueError(f"expected 16x96 source, got {width}x{height}")

    dst = [[0 for _ in range(96)] for _ in range(32)]
    remap = (3, 2, 1, 0)

    for direction in range(3):
        frame = src[direction * 16:(direction + 1) * 16]
        x_base = direction * 32 + 8
        for y in range(16):
            for x in range(16):
                dst[16 + y][x_base + x] = remap[frame[y][x]]

    _write_indexed_4bit_png(output, dst)

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    convert(args.source, args.output)

if __name__ == "__main__":
    main()
