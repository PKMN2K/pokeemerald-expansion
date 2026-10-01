#!/usr/bin/env python3
"""Convert Prism's PALETTE_NITE dungeon palettes to Expansion JASC palettes."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

RGB_RE = re.compile(r"^\s*RGB\s+(\d+),\s*(\d+),\s*(\d+)\s*$")


def rgb5_to_8(v: int) -> int:
    if not 0 <= v <= 31:
        raise ValueError(v)
    return (v << 3) | (v >> 2)


def parse_night_palettes(path: Path) -> list[list[tuple[int, int, int]]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    try:
        start = lines.index(";Night") + 1
    except ValueError as exc:
        raise ValueError("missing ;Night section") from exc

    colors: list[tuple[int, int, int]] = []
    for line in lines[start:]:
        if line.startswith(";") and colors:
            break
        match = RGB_RE.match(line)
        if match:
            colors.append(tuple(map(int, match.groups())))
            if len(colors) == 32:
                break

    if len(colors) != 32:
        raise ValueError(f"expected 32 Night colors, got {len(colors)}")
    return [colors[i:i + 4] for i in range(0, 32, 4)]


def write_jasc(path: Path, source_colors: list[tuple[int, int, int]]) -> None:
    # Prism's grayscale 2bpp PNG is inverted by gbagfx on 4bpp conversion,
    # mapping source color indices 0..3 to GBA palette indices 12..15.
    colors = [(0, 0, 0)] * 12 + [
        tuple(rgb5_to_8(v) for v in color) for color in source_colors
    ]
    text = "JASC-PAL\n0100\n16\n" + "".join(
        f"{r} {g} {b}\n" for r, g, b in colors
    )
    path.write_text(text, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="Prism tilesets/bg.pal")
    parser.add_argument("output_dir", type=Path)
    args = parser.parse_args()

    palettes = parse_night_palettes(args.source)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for index in range(16):
        source = palettes[index] if index < 8 else [(0, 0, 0)] * 4
        write_jasc(args.output_dir / f"{index:02d}.pal", source)


if __name__ == "__main__":
    main()
