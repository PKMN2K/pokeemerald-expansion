#!/usr/bin/env python3
"""Convert Polished Crystal's Chris bike sheet to Expansion's 32x32 frames."""

from __future__ import annotations
import argparse
from pathlib import Path
from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png

def convert(source: Path, output: Path) -> None:
    width, height, src = _read_png_2bit_grayscale(source)
    if (width, height) != (16, 96):
        raise ValueError(f"expected 16x96 source, got {width}x{height}")
    frames = [src[i * 16:(i + 1) * 16] for i in range(6)]
    mapping = ((0,False),(1,False),(2,False),(3,False),(3,True),(4,False),(4,True),(5,False),(5,False))
    dst = [[0 for _ in range(288)] for _ in range(32)]
    remap = (3, 2, 1, 0)
    for target_index, (source_index, hflip) in enumerate(mapping):
        frame = frames[source_index]
        x_base = target_index * 32 + 8
        for y in range(16):
            for x in range(16):
                sx = 15 - x if hflip else x
                dst[16 + y][x_base + x] = remap[frame[y][sx]]
    _write_indexed_4bit_png(output, dst)

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    convert(args.source, args.output)

if __name__ == "__main__":
    main()
