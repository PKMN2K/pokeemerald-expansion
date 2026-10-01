#!/usr/bin/env python3
"""Extract Polished Crystal's Strength boulder into a GBA OBJ-compatible PNG."""

from __future__ import annotations

import argparse
from pathlib import Path

from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png


def convert(source: Path, output: Path) -> None:
    width, height, src = _read_png_2bit_grayscale(source)
    if (width, height) != (16, 48):
        raise ValueError(f"expected 16x48 source, got {width}x{height}")

    # gfx/sprites/boulder_rock_fossil.png stacks three 16x16 objects:
    # boulder, smashable rock, fossil. Strength uses the first object.
    boulder = src[0:16]

    # RGBDS grayscale convention is white=3, black=0. Re-index so white becomes
    # OBJ palette index 0 (transparent) while preserving the three visible shades.
    remap = (3, 2, 1, 0)
    dst = [[remap[pixel] for pixel in row] for row in boulder]

    _write_indexed_4bit_png(output, dst)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "source",
        type=Path,
        help="Polished Crystal gfx/sprites/boulder_rock_fossil.png",
    )
    parser.add_argument(
        "output",
        type=Path,
        help="output graphics/object_events/pics/misc/polished_strength_boulder.png",
    )
    args = parser.parse_args()
    convert(args.source, args.output)


if __name__ == "__main__":
    main()
