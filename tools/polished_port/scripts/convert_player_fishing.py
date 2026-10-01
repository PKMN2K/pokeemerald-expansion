#!/usr/bin/env python3
"""Build Expansion fishing sheets from Polished Crystal's fishing overlays."""

from __future__ import annotations
import argparse
from pathlib import Path
from convert_player_walking import _read_png_2bit_grayscale, _write_indexed_4bit_png

def convert(base_path: Path, overlay_path: Path, output: Path) -> None:
    bw, bh, base = _read_png_2bit_grayscale(base_path)
    ow, oh, overlay = _read_png_2bit_grayscale(overlay_path)
    if (bw, bh) != (16, 96):
        raise ValueError(f"expected 16x96 base, got {bw}x{bh}")
    if (ow, oh) != (16, 24):
        raise ValueError(f"expected 16x24 fishing overlay, got {ow}x{oh}")

    # Source standing frames: south, north, west.
    poses = []
    for direction in range(3):
        frame = [row[:] for row in base[direction * 16:(direction + 1) * 16]]
        strip = overlay[direction * 8:(direction + 1) * 8]
        frame[8:16] = [row[:] for row in strip]
        poses.append(frame)

    # Expansion fishing physical order: west x4, north x4, south x4.
    order = (2,2,2,2, 1,1,1,1, 0,0,0,0)
    dst = [[0 for _ in range(384)] for _ in range(32)]
    remap = (3, 2, 1, 0)

    for target_index, direction in enumerate(order):
        frame = poses[direction]
        x_base = target_index * 32 + 8
        for y in range(16):
            for x in range(16):
                dst[16 + y][x_base + x] = remap[frame[y][x]]

    _write_indexed_4bit_png(output, dst)

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("base", type=Path)
    parser.add_argument("overlay", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    convert(args.base, args.overlay, args.output)

if __name__ == "__main__":
    main()
