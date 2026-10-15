#!/usr/bin/env python3
"""Audit pinned Johto outdoor animation dispatch without activating it.

All five Johto outdoor tilesets share the original 22-entry schedule.
The original animation functions/frames are not yet ported. This check
prevents accidentally substituting static or fabricated animation art.
"""
from pathlib import Path
import re

FIXTURE = Path(__file__).resolve().parents[1] / "fixtures/johto_outdoor_animation_schedule.asm"
LABELS = tuple(f"TilesetJohto{i}Anim::" for i in range(1, 6))
EXPECTED_ACTIONS = (
    "AnimateWaterTile", "AnimateRainPuddleTile", "AnimateRainWaterTile",
    "DoNothing", "AnimateFlowerTile",
    "AnimateWhirlpoolTile", "AnimateWhirlpoolTile",
    "AnimateWhirlpoolTile", "AnimateWhirlpoolTile",
    "AnimateRainPuddleTile", "AnimateRainWaterTile",
    "StandingTileFrame8", "WriteTileToBuffer", "DoNothing",
    "ScrollTileDown", "DoNothing", "ScrollTileDown", "DoNothing",
    "ScrollTileDown", "DoNothing", "ReadTileFromBuffer",
    "DoneTileAnimation",
)
EXPECTED_TARGETS = ("$14", "$1c", "$1d", "$1c", "$1d", "$35", "$35")


def parse_schedule(text):
    labels = re.findall(r"^TilesetJohto\d+Anim::$", text, flags=re.M)
    if tuple(labels) != LABELS:
        raise ValueError("source Johto animation labels changed")
    entries = []
    for line in text.splitlines():
        m = re.match(r"^\s*dw\s+(.+?),\s*(\w+)\s*(?:;.*)?$", line)
        if m:
            entries.append((m.group(1).strip(), m.group(2)))
    if len(entries) != 22:
        raise ValueError(f"expected 22 source animation dispatch entries, got {len(entries)}")
    if tuple(action for _, action in entries) != EXPECTED_ACTIONS:
        raise ValueError("source animation dispatch order changed")
    targets = tuple(re.search(r"\$[0-9a-fA-F]+", p).group()
                    for p, _ in entries if re.search(r"\$[0-9a-fA-F]+", p))
    if targets != EXPECTED_TARGETS:
        raise ValueError(f"source animation destination tiles changed: {targets}")
    return entries


def main():
    source = FIXTURE.read_text(encoding="utf-8")
    entries = parse_schedule(source)
    assert len(entries) == 22
    assert len([a for _, a in entries if a == "AnimateWhirlpoolTile"]) == 4
    assert len([a for _, a in entries if a == "ScrollTileDown"]) == 3
    assert entries[-1] == ("NULL", "DoneTileAnimation")
    for changed in (
        source.replace("AnimateWaterTile", "DoNothing", 1),
        source.replace("tile $14", "tile $15", 1),
        source.replace("TilesetJohto5Anim::", "TilesetJohto6Anim::", 1),
    ):
        try:
            parse_schedule(changed)
        except ValueError:
            pass
        else:
            raise AssertionError("modified source animation schedule was accepted")
    print("PASS: 5 original Johto outdoor tilesets share 22 dispatch entries")
    print("PASS: water, rain, flower, whirlpool and buffer/scroll schedule pinned")
    print("REVIEW: source frame art and original tick timing still require port")
    print("REVIEW: no live animation hook or tileset registration enabled")


if __name__ == "__main__":
    main()
