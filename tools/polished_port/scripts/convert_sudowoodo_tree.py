#!/usr/bin/env python3
"""Convert Polished Crystal's weird-tree Sudowoodo sprite to Expansion layout."""

from __future__ import annotations

import argparse
from pathlib import Path

from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png


def convert(source: Path, output: Path) -> None:
    width, height, src = _read_png_2bit_grayscale(source)
    if (width, height) != (16, 48):
        raise ValueError(f"expected 16x48 source, got {width}x{height}")

    remap = (3, 2, 1, 0)
    dst = [[0 for _ in range(48)] for _ in range(32)]

    for frame_index in range(3):
        frame = src[frame_index * 16:(frame_index + 1) * 16]
        x0 = frame_index * 16
        for y in range(16):
            for x in range(16):
                dst[16 + y][x0 + x] = remap[frame[y][x]]

    _write_indexed_4bit_png(output, dst)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="Polished Crystal gfx/sprites/weird_tree.png")
    parser.add_argument(
        "output",
        type=Path,
        help="output graphics/object_events/pics/pokemon_old/polished_sudowoodo_tree.png",
    )
    args = parser.parse_args()
    convert(args.source, args.output)


if __name__ == "__main__":
    main()
