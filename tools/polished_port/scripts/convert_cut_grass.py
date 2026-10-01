#!/usr/bin/env python3
"""Convert Polished Crystal's Cut grass leaf tile to Expansion's 8x8 field-effect format."""

from __future__ import annotations

import argparse
from pathlib import Path

from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png


def convert(source: Path, output: Path) -> None:
    width, height, src = _read_png_2bit_grayscale(source)
    if (width, height) != (16, 16):
        raise ValueError(f"expected 16x16 source, got {width}x{height}")

    # In Polished Crystal, SPRITE_ANIM_OAMSET_LEAF references tile $00 from
    # gfx/overworld/cut_grass.png. That is the top-left 8x8 tile.
    #
    # RGBDS grayscale convention is white=3, black=0. Re-index so white becomes
    # OBJ palette index 0 (transparent) while preserving the three visible shades.
    remap = (3, 2, 1, 0)
    dst = [
        [remap[src[y][x]] for x in range(8)]
        for y in range(8)
    ]

    _write_indexed_4bit_png(output, dst)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="Polished Crystal gfx/overworld/cut_grass.png")
    parser.add_argument("output", type=Path, help="output polished_cut_grass.png")
    args = parser.parse_args()
    convert(args.source, args.output)


if __name__ == "__main__":
    main()
