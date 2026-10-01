#!/usr/bin/env python3
"""Convert the supplemental Polished Crystal Chris running sheet.

The running source has the same six-frame 16x96 layout as chris.png, so it
uses the already-validated walking-sheet converter and writes a 144x32
Expansion-compatible running sheet.
"""

from pathlib import Path
import argparse

from convert_player_walking import convert


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="Polished Crystal gfx/sprites/chris_run.png")
    parser.add_argument("output", type=Path, help="output polished_chris/running.png")
    args = parser.parse_args()
    convert(args.source, args.output)


if __name__ == "__main__":
    main()
