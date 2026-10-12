#!/usr/bin/env python3
"""Extract source-faithful Johto outdoor time-of-day RGB555 palettes.

Reads the pinned Polished Crystal RGBDS text palette source directly.
Writes four named staging files, each eight GBC 4-colour banks.
Sources: gfx/tilesets/bg_tiles.pal (default) and map-specific overrides.
No new artwork is generated. Does not register a live Emerald tileset.
"""
import argparse
from pathlib import Path
import re
import struct

PERIODS = ("morn", "day", "nite", "eve")
RGB_LINE = re.compile(r"^\s*RGB\s+(.+?)(?:\s*;.*)?$", re.IGNORECASE)
HEADING = re.compile(r"^\s*;\s*(morn|day|nite|eve)\s*$", re.IGNORECASE)


def parse_palettes(source: str):
    groups = {period: [] for period in PERIODS}
    current = None
    for line in source.splitlines():
        heading = HEADING.match(line)
        if heading:
            current = heading.group(1).lower()
            continue
        rgb = RGB_LINE.match(line)
        if rgb is None or current is None:
            continue
        values = [int(part.strip(), 10) for part in rgb.group(1).split(",")]
        if len(values) != 12 or any(value < 0 or value > 31 for value in values):
            raise ValueError(f"invalid RGBDS four-colour palette: {line}")
        colors = [values[i] | (values[i + 1] << 5) | (values[i + 2] << 10)
                  for i in range(0, 12, 3)]
        groups[current].append(colors)
    for period, banks in groups.items():
        if len(banks) < 8:
            raise ValueError(f"{period}: found {len(banks)} palette banks, expected at least 8")
    return {period: banks[:8] for period, banks in groups.items()}


def convert(banks):
    result = bytearray()
    for colors in banks:
        result.extend(struct.pack("<16H", *colors, *([0] * 12)))
    return bytes(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True,
                        help="Polished Crystal source checkout, pinned at 3fa43192379d")
    parser.add_argument("--output", type=Path, required=True,
                        help="staging output directory (not live tileset assets)")
    parser.add_argument("--palette-file", default="gfx/tilesets/bg_tiles.pal",
                        help="relative path for map-specific palette override")
    args = parser.parse_args()
    source_file = args.source / args.palette_file
    banks = parse_palettes(source_file.read_text(encoding="utf-8"))
    args.output.mkdir(parents=True, exist_ok=True)
    for period in PERIODS:
        output = args.output / f"{period}.gbapal.staging.bin"
        output.write_bytes(convert(banks[period]))
        print(f"{period}: {len(banks[period])} banks staged at {output}")
    print("Note: individual maps may use overrides; no automatic time-of-day runtime wiring.")


if __name__ == "__main__":
    main()
