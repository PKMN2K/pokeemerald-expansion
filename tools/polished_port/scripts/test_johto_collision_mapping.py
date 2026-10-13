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
    stager = load("stage_johto_metatile_collision")
    verifier = load("verify_johto_collision_source")
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

    staged_source = {
        "block_count": 253,
        "missing_blocks_in_256_capacity": [253, 254, 255],
        "blocks": [
            {"block": i, "quadrants": {
                "top_left": "FLOOR",
                "top_right": "WALL" if i == 0 else "FLOOR",
                "bottom_left": "DOOR" if i == 0 else "FLOOR",
                "bottom_right": "FLOOR",
            }} for i in range(253)
        ],
    }
    staged = stager.stage(staged_source)
    assert staged["source_block_count"] == 253
    assert staged["blocks"][0]["quadrants"][1]["physical"] == "blocked"
    assert staged["blocks"][0]["quadrants"][2]["requires_review"]
    assert staged["blocks"][0]["quadrants"][0]["emerald_behavior_candidate"] == "MB_NORMAL"
    assert "attributes" not in staged
    assert sum(verifier.EXPECTED.values()) == 253 * 4
    try:
        verifier.validate(staged_source)
    except ValueError as exc:
        assert "source collision symbol counts mismatch" in str(exc)
    else:
        raise AssertionError("source fingerprint mismatch was accepted")
    try:
        stager.stage({**staged_source, "missing_blocks_in_256_capacity": []})
    except ValueError:
        pass
    else:
        raise AssertionError("missing block mismatch not rejected")

    try:
        extractor.extract("tilecoll FLOOR, FLOOR, FLOOR, FLOOR ; 01\n")
    except ValueError:
        pass
    else:
        raise AssertionError("collision sequence gap not rejected")
    print("PASS: collision audit candidates exist; unresolved warp/wall cases remain unmapped")


if __name__ == "__main__":
    main()
