#!/usr/bin/env python3
"""Stage the exact Johto Traditional quadrant collision declarations.

The pinned Polished Crystal source uses tilecoll with four symbolic
collision values per 32x32 block. Preserve these symbols rather than
incorrectly assuming GBC collision IDs equal Emerald behavior IDs.

Usage:
  python3 tools/polished_port/scripts/extract_johto_collision.py \
    --source /path/to/polishedcrystal --output /tmp/johto_collision.json

Output is a review manifest, NOT a live Emerald metatile attribute table.
"""
import argparse
import json
from pathlib import Path
import re

SOURCE = Path("data/tilesets/johto_traditional_collision.asm")
LINE = re.compile(
    r"^\s*tilecoll\s+([A-Z0-9_]+)\s*,\s*([A-Z0-9_]+)\s*,\s*"
    r"([A-Z0-9_]+)\s*,\s*([A-Z0-9_]+)\s*;\s*([0-9a-fA-F]{2})\s*$"
)
QUADRANTS = ("top_left", "top_right", "bottom_left", "bottom_right")


def extract(text):
    blocks = []
    for line in text.splitlines():
        if not line.strip() or line.lstrip().startswith(";"):
            continue
        m = LINE.match(line)
        if not m:
            raise ValueError(f"unrecognized source collision declaration: {line}")
        block_index = int(m.group(5), 16)
        if block_index != len(blocks):
            raise ValueError(f"collision block sequence gap: expected {len(blocks):02x}, got {block_index:02x}")
        blocks.append({"block": block_index, "quadrants": dict(zip(QUADRANTS, m.groups()[:4]))})
    if not blocks or len(blocks) > 256:
        raise ValueError(f"invalid block count: {len(blocks)}")
    return blocks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    blocks = extract((args.source / SOURCE).read_text(encoding="utf-8"))
    kinds = sorted({kind for block in blocks for kind in block["quadrants"].values()})
    result = {
        "source": str(SOURCE),
        "format": "source-faithful quadrant collision inventory; not Emerald behavior IDs",
        "block_count": len(blocks),
        "missing_blocks_in_256_capacity": list(range(len(blocks), 256)),
        "symbols": kinds,
        "blocks": blocks,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Staged {len(blocks)} collision blocks, {len(kinds)} symbolic behaviors")
    print("Live behavior mapping and map-specific warp/sign scripts remain pending.")


if __name__ == "__main__":
    main()
