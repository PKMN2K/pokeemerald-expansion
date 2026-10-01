#!/usr/bin/env python3
"""Convert Polished Crystal's overworld Poké Ball to Expansion's 5-frame sheet."""

from __future__ import annotations

import argparse
from pathlib import Path

from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png


def convert(source: Path, output: Path) -> None:
    width, height, src = _read_png_2bit_grayscale(source)
    if (width, height) != (16, 48):
        raise ValueError(f"expected 16x48 source, got {width}x{height}")

    # ball_cut_fruit.png stacks three 16x16 objects. The first is the Poké Ball.
    ball = src[0:16]

    # Expansion addresses five 16x32 physical frames for gObjectEventPic_PokeBall.
    # Crystal's Poké Ball is static, so repeat the exact source frame and bottom-align it.
    remap = (3, 2, 1, 0)
    frame = [[remap[pixel] for pixel in row] for row in ball]
    dst = [[0 for _ in range(80)] for _ in range(32)]

    for frame_index in range(5):
        x0 = frame_index * 16
        for y in range(16):
            for x in range(16):
                dst[16 + y][x0 + x] = frame[y][x]

    _write_indexed_4bit_png(output, dst)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "source",
        type=Path,
        help="Polished Crystal gfx/sprites/ball_cut_fruit.png",
    )
    parser.add_argument(
        "output",
        type=Path,
        help="output graphics/object_events/pics/misc/polished_item_ball.png",
    )
    args = parser.parse_args()
    convert(args.source, args.output)


if __name__ == "__main__":
    main()
