#!/usr/bin/env python3
"""Regression test for staged Johto collision mapping (no ROM writes)."""
import importlib.util
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
SCRIPTS = ROOT / "tools/polished_port/scripts"


def load(name):
    spec = importlib.util.spec_from_file_location(name, SCRIPTS / (name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    extractor = load("extract_johto_collision")
    auditor = load("audit_johto_collision_mapping")
    symbols = set(re.findall(r"\bMB_[A-Z0-9_]+\b",
                             (ROOT / "include/constants/metatile_behaviors.h").read_text()))
    unknown = set(auditor.CANDIDATES.values()) - symbols
    assert not unknown, f"unrecognized Emerald behavior candidates: {sorted(unknown)}"

    sample = (
        "tilecoll FLOOR, TALL_GRASS, WALL, DOOR ; 00\n"
        "tilecoll WATER, FLOOR, FLOOR, WALL ; 01\n"
    )
    blocks = extractor.extract(sample)
    assert len(blocks) == 2
    assert blocks[0]["quadrants"]["top_right"] == "TALL_GRASS"
    assert blocks[1]["quadrants"]["top_left"] == "WATER"

    report = auditor.audit({
        "blocks": blocks,
        "missing_blocks_in_256_capacity": list(range(2, 256)),
    })
    assert report["status"] == "review_only_not_for_live_tileset"
    assert report["candidates"]["TALL_GRASS"]["emerald"] == "MB_TALL_GRASS"
    assert report["unresolved"]["WALL"] == 2
    assert report["unresolved"]["DOOR"] == 1
    assert "WALL" not in report["candidates"]
    assert "DOOR" not in report["candidates"]
    assert report["physical"]["WALL"]["classification"] == "blocked"
    assert report["physical"]["DOOR"]["classification"] == "requires_map_event_review"
    assert report["physical"]["FLOOR"]["classification"] == "walkable_candidate"
    assert report["physical"]["WATER"]["classification"] == "unresolved"

    try:
        extractor.extract("tilecoll FLOOR, FLOOR, FLOOR, FLOOR ; 01\n")
    except ValueError:
        pass
    else:
        raise AssertionError("collision sequence gap not rejected")
    print("PASS: collision audit candidates exist; unresolved warp/wall cases remain unmapped")


if __name__ == "__main__":
    main()
