#!/usr/bin/env python3
"""Build Expansion fishing sheets from Polished Crystal's fishing graphics."""

from __future__ import annotations
import argparse
from pathlib import Path
from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png


def convert(base_path: Path, overlay_path: Path, rod_path: Path, output: Path) -> None:
    bw, bh, base = _read_png_2bit_grayscale(base_path)
    ow, oh, overlay = _read_png_2bit_grayscale(overlay_path)
    rw, rh, rod = _read_png_2bit_grayscale(rod_path)
    if (bw, bh) != (16, 96):
        raise ValueError(f"expected 16x96 base, got {bw}x{bh}")
    if (ow, oh) != (16, 24):
        raise ValueError(f"expected 16x24 fishing overlay, got {ow}x{oh}")
    if (rw, rh) != (8, 16):
        raise ValueError(f"expected 8x16 fishing rod, got {rw}x{rh}")

    # Source standing frames: south, north, west.
    poses = []
    for direction in range(3):
        frame = [row[:] for row in base[direction * 16:(direction + 1) * 16]]
        strip = overlay[direction * 8:(direction + 1) * 8]
        frame[8:16] = [row[:] for row in strip]
        poses.append(frame)

    rod_vertical = [row[:] for row in rod[0:8]]
    rod_horizontal = [row[:] for row in rod[8:16]]

    # Expansion fishing physical order: west x4, north x4, south x4.
    order = (2,2,2,2, 1,1,1,1, 0,0,0,0)
    dst = [[0 for _ in range(384)] for _ in range(32)]
    remap = (3, 2, 1, 0)

    for target_index, direction in enumerate(order):
        frame = poses[direction]
        frame_x = target_index * 32

        # Center the 16x16 player body inside the 32x32 frame. Polished Crystal's
        # fifth fishing OAM tile extends one tile outside the body, so a centered
        # 16x16 origin at (8, 8) leaves room on every side for the rod.
        for y in range(16):
            for x in range(16):
                dst[8 + y][frame_x + 8 + x] = remap[frame[y][x]]

        if direction == 0:
            # South: tile $7a at (y=16, x=0), translated into the 32x32 canvas.
            for y in range(8):
                for x in range(8):
                    dst[24 + y][frame_x + 8 + x] = remap[rod_vertical[y][x]]
        elif direction == 1:
            # North: tile $7a at (y=-8, x=0).
            for y in range(8):
                for x in range(8):
                    dst[y][frame_x + 8 + x] = remap[rod_vertical[y][x]]
        else:
            # West: tile $7b at (y=5, x=-8) with OAM_XFLIP.
            # Runtime horizontal flip of the whole frame produces East.
            for y in range(8):
                for x in range(8):
                    dst[13 + y][frame_x + x] = remap[rod_horizontal[y][7 - x]]

    _write_indexed_4bit_png(output, dst)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("base", type=Path)
    parser.add_argument("overlay", type=Path)
    parser.add_argument("rod", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    convert(args.base, args.overlay, args.rod, args.output)


if __name__ == "__main__":
    main()
