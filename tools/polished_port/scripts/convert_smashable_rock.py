#!/usr/bin/env python3
"""Extract Polished Crystal's smashable rock into Expansion's four Rock Smash frame slots."""

from __future__ import annotations

import argparse
from pathlib import Path

from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png


def convert(source: Path, output: Path) -> None:
    width, height, src = _read_png_2bit_grayscale(source)
    if (width, height) != (16, 48):
        raise ValueError(f"expected 16x48 source, got {width}x{height}")

    # gfx/sprites/boulder_rock_fossil.png stacks:
    # 0..15 boulder, 16..31 smashable rock, 32..47 fossil.
    rock = src[16:32]

    # Expansion's breakable-rock pic tables address four 16x16 frames in a
    # 2x2 (32x32) sheet. Polished Crystal has one static rock image and
    # shakes/removes the object at runtime, so repeat that exact image.
    remap = (3, 2, 1, 0)
    frame = [[remap[pixel] for pixel in row] for row in rock]
    dst = [[0 for _ in range(32)] for _ in range(32)]
    for frame_y in (0, 16):
        for frame_x in (0, 16):
            for y in range(16):
                for x in range(16):
                    dst[frame_y + y][frame_x + x] = frame[y][x]

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
        help="output graphics/object_events/pics/misc/polished_smashable_rock.png",
    )
    args = parser.parse_args()
    convert(args.source, args.output)


if __name__ == "__main__":
    main()
