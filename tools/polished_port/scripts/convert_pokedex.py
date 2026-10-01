#!/usr/bin/env python3
"""Extract Polished Crystal's Pokédex standing-object frame for pokeemerald-expansion."""

from __future__ import annotations

import argparse
from pathlib import Path

from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png


def convert(source: Path, output: Path) -> None:
    width, height, src = _read_png_2bit_grayscale(source)
    if (width, height) != (16, 48):
        raise ValueError(f"expected 16x48 source, got {width}x{height}")

    # Standing-sprite directional frames are down, up, left. Oak's Lab uses
    # SPRITEMOVEDATA_STANDING_LEFT for the Pokédex object, so use frame 3.
    pokedex = src[32:48]

    remap = (3, 2, 1, 0)
    dst = [[remap[pixel] for pixel in row] for row in pokedex]

    _write_indexed_4bit_png(output, dst)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "source",
        type=Path,
        help="Polished Crystal gfx/sprites/book_paper_pokedex.png",
    )
    parser.add_argument(
        "output",
        type=Path,
        help="output graphics/object_events/pics/misc/polished_pokedex.png",
    )
    args = parser.parse_args()
    convert(args.source, args.output)


if __name__ == "__main__":
    main()
