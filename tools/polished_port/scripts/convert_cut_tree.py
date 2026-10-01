#!/usr/bin/env python3
"""Convert Polished Crystal's Cut tree into four 32x16 Expansion frames.

Polished Crystal animates Cut by keeping the same four 8x8 tree quadrants and
moving the left/right halves progressively farther apart. Expansion expects
four ordinary sprite frames, so this converter precomposes those OAM positions
onto four 32x16 canvases.
"""

from __future__ import annotations

import argparse
from pathlib import Path

from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png


# Polished Crystal data/sprite_anims/oam.asm positions, in pixels relative to
# the sprite origin. The 32x16 target canvas spans x=-16..15 and y=-8..7.
FRAME_OBJECTS = (
    ((-8, -8, 0), ( 0, -8, 1), (-8,  0, 2), ( 0,  0, 3)),
    ((-10,-8, 0), ( 2, -8, 1), (-10, 0, 2), ( 2,  0, 3)),
    ((-12,-8, 0), ( 4, -8, 1), (-12, 0, 2), ( 4,  0, 3)),
    ((-16,-8, 0), ( 8, -8, 1), (-16, 0, 2), ( 8,  0, 3)),
)


def convert(source: Path, output: Path) -> None:
    width, height, src = _read_png_2bit_grayscale(source)
    if (width, height) != (16, 16):
        raise ValueError(f"expected 16x16 source, got {width}x{height}")

    # Source tile order is top-left, top-right, bottom-left, bottom-right.
    tiles = (
        [row[0:8] for row in src[0:8]],
        [row[8:16] for row in src[0:8]],
        [row[0:8] for row in src[8:16]],
        [row[8:16] for row in src[8:16]],
    )

    # RGBDS grayscale white=3, black=0 -> OBJ transparent index 0, darkest 3.
    remap = (3, 2, 1, 0)
    dst = [[0 for _ in range(128)] for _ in range(16)]

    for frame_index, objects in enumerate(FRAME_OBJECTS):
        frame_x = frame_index * 32
        for x, y, tile_index in objects:
            tile = tiles[tile_index]
            out_x = frame_x + x + 16
            out_y = y + 8
            for ty in range(8):
                for tx in range(8):
                    dst[out_y + ty][out_x + tx] = remap[tile[ty][tx]]

    _write_indexed_4bit_png(output, dst)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="Polished Crystal gfx/overworld/cut_tree.png")
    parser.add_argument("output", type=Path, help="output polished_cut_tree.png")
    args = parser.parse_args()
    convert(args.source, args.output)


if __name__ == "__main__":
    main()
