#!/usr/bin/env python3
"""Convert Polished Crystal's Chris surf sheet to Expansion's surf frame layout."""

from __future__ import annotations
import argparse
from pathlib import Path
from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png

def convert(source: Path, output: Path) -> None:
    width, height, src = _read_png_2bit_grayscale(source)
    if (width, height) != (16, 96):
        raise ValueError(f"expected 16x96 source, got {width}x{height}")

    frames = [src[i * 16:(i + 1) * 16] for i in range(6)]
    # Expansion physical surf order:
    # south idle, south step, north idle, north step, west idle, west step.
    order = (0, 3, 1, 4, 2, 5)

    dst = [[0 for _ in range(192)] for _ in range(32)]
    remap = (3, 2, 1, 0)

    for target_index, source_index in enumerate(order):
        frame = frames[source_index]
        x_base = target_index * 32 + 8
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
