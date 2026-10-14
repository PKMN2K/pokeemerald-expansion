#!/usr/bin/env python3
"""Stage authentic Johto map-group / overcast roof RGB555 overrides.

Polished Crystal's engine/gfx/color.asm LoadMapPals patches *exactly two*
16-bit colors at 'wBGPals1 palette PAL_BG_ROOF + 2': color indices 1 and 2
within CGB background palette 6. It chooses the row from RoofPals by the
original 1-based wMapGroup, or from OvercastRoofPals by the original
1-based GetOvercastIndex (Azalea, Lake of Rage, Goldenrod/Stormy Beach).
Morning/day share roof colors; night and evening have separate pairs.

This script patches the previously staged authentic seven-bank RGB555
GBA palette collection. It never guesses which Emerald map corresponds
to a Polished Crystal map group; the group or overcast index is explicit.
It does not implement weather, runtime clock events, Emerald map IDs or
live tileset registration.

Examples:
  python3 tools/polished_port/scripts/stage_johto_roof_palettes.py \
      --map-group 4 --family violet_ecruteak --time day \
      --output /tmp/ecruteak_day_roofs.staging.bin
  python3 tools/polished_port/scripts/stage_johto_roof_palettes.py \
      --overcast-index 1 --time nite \
      --output /tmp/azalea_overcast_nite_roofs.staging.bin
"""
import argparse
import json
from pathlib import Path
import re
import struct

from stage_johto_time_palettes import (
    FIXTURES,
    OUTPUT_BYTES,
    TIME_NAMES,
    load_source_palettes,
    stage,
)

SOURCE_MAP_GROUP_COUNT = 37
SOURCE_OVERCAST_COUNT = 3
ROOF_PALETTE_INDEX = 6
ROOF_COLOR_INDICES = (1, 2)
ROOF_RGB_TRIPLETS_PER_ROW = 6
ROOF_BYTES_PER_ROW = 12
OVERCAST_LABELS = ("Azalea Town", "Lake of Rage", "Goldenrod City / Stormy Beach")
MAP_GROUP_FIXTURE = "johto_map_constants.asm"


def parse_map_group_numbers(source):
    """Verify 37 source groups and their 1-based numeric order."""
    numbers = [
        int(m.group(1))
        for m in re.finditer(r"(?m)^\s*newgroup\s*;\s*(\d+)\s*$", source)
    ]
    if numbers != list(range(1, SOURCE_MAP_GROUP_COUNT + 1)):
        raise ValueError(f"unexpected original map group order: {numbers}")
    return numbers


def parse_roof_rows(source, expected_count, label):
    """Read RGBDS's six RGB triplets (= 3 roof color pairs) per row."""
    color_branch = source.split("\nelse\n", 1)[0]
    rows = []
    labels = []
    for line in color_branch.splitlines():
        code, _, comment = line.partition(";")
        match = re.fullmatch(r"\s*RGB\s+([\d,\s]+)\s*", code)
        if not match:
            if code.strip().startswith("RGB"):
                raise ValueError(f"{label}: malformed RGB directive: {line}")
            continue
        parts = [part.strip() for part in match.group(1).split(",")]
        if len(parts) != ROOF_RGB_TRIPLETS_PER_ROW * 3:
            raise ValueError(f"{label}: expected 18 RGB channels, got {len(parts)}")
        channels = [int(x, 10) for x in parts]
        if any(not 0 <= x <= 31 for x in channels):
            raise ValueError(f"{label}: RGB channel outside 0..31")
        if not comment.strip():
            raise ValueError(f"{label}: missing pinned source row label")
        rows.append(tuple(
            channels[i] | channels[i + 1] << 5 | channels[i + 2] << 10
            for i in range(0, len(channels), 3)
        ))
        labels.append(comment.strip())
    if len(rows) != expected_count:
        raise ValueError(f"{label}: expected {expected_count} rows, got {len(rows)}")
    return rows, labels


def load_roof_tables(fixture_dir=FIXTURES):
    group_source = (fixture_dir / MAP_GROUP_FIXTURE).read_text(encoding="utf-8")
    parse_map_group_numbers(group_source)
    standard, names = parse_roof_rows(
        (fixture_dir / "johto_roofs.pal").read_text(encoding="utf-8"),
        SOURCE_MAP_GROUP_COUNT + 1, "map group roofs",
    )
    overcast, weather_names = parse_roof_rows(
        (fixture_dir / "johto_roofs_overcast.pal").read_text(encoding="utf-8"),
        SOURCE_OVERCAST_COUNT, "overcast roofs",
    )
    # Row 0 is an unused source placeholder, not a playable group.
    if names[0] != "N/A":
        raise ValueError("map group zero must be the source N/A placeholder")
    for index, label in enumerate(OVERCAST_LABELS):
        if weather_names[index] != ("Goldenrod City" if index == 2 else label):
            raise ValueError("overcast source index meaning has changed")
    return {
        "groups": standard,
        "group_labels": names,
        "overcast": overcast,
        "overcast_labels": weather_names,
    }


def roof_colors(tables, time, *, map_group=None, overcast_index=None):
    """Return (RGB555 color 1, color 2) and source provenance."""
    if time not in TIME_NAMES:
        raise ValueError(f"unsupported source time: {time!r}")
    if (map_group is None) == (overcast_index is None):
        raise ValueError("select exactly one original map group or overcast index")
    if map_group is not None:
        if not isinstance(map_group, int) or not 1 <= map_group <= SOURCE_MAP_GROUP_COUNT:
            raise ValueError(f"invalid original map group: {map_group}")
        row = tables["groups"][map_group]
        name = tables["group_labels"][map_group]
        kind, source_index = "map_group", map_group
    else:
        if not isinstance(overcast_index, int) or not 1 <= overcast_index <= SOURCE_OVERCAST_COUNT:
            raise ValueError(f"invalid original overcast index: {overcast_index}")
        row = tables["overcast"][overcast_index - 1]
        name = tables["overcast_labels"][overcast_index - 1]
        kind, source_index = "overcast", overcast_index
    # LoadMapPals: morning and daytime use the first pair.
    # Nighttime uses the second pair; evening the third pair.
    offset = 0 if time in ("morn", "day") else (2 if time == "nite" else 4)
    return row[offset:offset + 2], {
        "source_selection": kind,
        "original_index": source_index,
        "source_label": name,
        "source_time": time,
        "source_roof_pair_index": offset // 2,
    }


def apply_roof_colors(source_seven_banks, colors):
    """Modify ONLY colors 1 and 2 of palette 6 (4 bytes in 224)."""
    if len(source_seven_banks) != OUTPUT_BYTES:
        raise ValueError(f"expected {OUTPUT_BYTES} bytes of seven GBA palettes")
    if len(colors) != 2 or any(not isinstance(v, int) or not 0 <= v < 0x8000 for v in colors):
        raise ValueError("roof override needs exactly two RGB555 color words")
    output = bytearray(source_seven_banks)
    for index, color in zip(ROOF_COLOR_INDICES, colors):
        struct.pack_into("<H", output, ROOF_PALETTE_INDEX * 32 + 2 * index, color)
    return bytes(output)


def stage_roof_variant(source_palettes, roof_tables, *, time, family=None,
                       map_group=None, overcast_index=None):
    if (map_group is None) == (overcast_index is None):
        raise ValueError("select either source map group or source overcast index")
    if family is None:
        family = "overcast" if overcast_index is not None else "standard"
    if overcast_index is not None and family != "overcast":
        raise ValueError("overcast roof indices require the original overcast palette family")
    if map_group is not None and family not in ("standard", "violet_ecruteak"):
        raise ValueError("ordinary source map group must use standard or violet_ecruteak")
    if time not in TIME_NAMES:
        raise ValueError(f"unknown original time: {time}")
    seven, _ = stage(source_palettes)
    if f"{family}.{time}" not in seven:
        raise ValueError(f"unknown family: {family}")
    colors, source_record = roof_colors(
        roof_tables, time, map_group=map_group, overcast_index=overcast_index
    )
    before = seven[f"{family}.{time}"]
    after = apply_roof_colors(before, colors)
    return after, {
        "status": "staging_only_not_registered",
        "source_family": family,
        "palette_global_slot": ROOF_PALETTE_INDEX,
        "source_gbc_colors_overridden": list(ROOF_COLOR_INDICES),
        "gba_rgb555_values": [f"0x{v:04X}" for v in colors],
        "roof_override_bytes_offset": [ROOF_PALETTE_INDEX * 32 + 2,
                                       ROOF_PALETTE_INDEX * 32 + 4],
        "source": source_record,
        "runtime_map_resolution_implemented": False,
        "runtime_time_switching_implemented": False,
        "overcast_event_conditions_implemented": False,
        "ready_for_live_registration": False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument("--map-group", type=int,
                           help="Original Polished Crystal map-group index, 1..37")
    selection.add_argument("--overcast-index", type=int,
                           help="Original Polished Crystal overcast index, 1..3")
    parser.add_argument("--family", choices=("standard", "violet_ecruteak", "overcast"),
                        help="Defaults to standard for normal groups, overcast for weather indices")
    parser.add_argument("--time", choices=TIME_NAMES, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    raw, review = stage_roof_variant(
        load_source_palettes(), load_roof_tables(),
        time=args.time, family=args.family,
        map_group=args.map_group, overcast_index=args.overcast_index,
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(raw)
    args.output.with_suffix(args.output.suffix + ".review.json").write_text(
        json.dumps(review, indent=2) + "\n", encoding="utf-8"
    )
    print(f"Staged {len(raw)} bytes with authentic {review['source']['source_label']} roof colors.")
    print("NOT registered: original map/event/time conditions need Emerald integration.")


if __name__ == "__main__":
    main()
