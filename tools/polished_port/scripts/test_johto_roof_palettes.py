#!/usr/bin/env python3
"""Regression checks for exact pinned Johto source roof RGB555 overlays."""
from pathlib import Path
import struct

from stage_johto_roof_palettes import (
    ROOF_PALETTE_INDEX,
    SOURCE_MAP_GROUP_COUNT,
    SOURCE_OVERCAST_COUNT,
    apply_roof_colors,
    load_roof_tables,
    parse_map_group_numbers,
    parse_roof_rows,
    roof_colors,
    stage_roof_variant,
)
from stage_johto_time_palettes import (
    OUTPUT_BYTES, TIME_NAMES, load_source_palettes, stage,
)

ROOT = Path(__file__).resolve().parents[3]


def reject(action):
    try:
        action()
    except ValueError:
        return
    raise AssertionError("invalid or unresolved roof mapping accepted")


def main():
    sources = load_source_palettes()
    roofs = load_roof_tables()
    assert len(roofs["groups"]) == SOURCE_MAP_GROUP_COUNT + 1 == 38
    assert len(roofs["overcast"]) == SOURCE_OVERCAST_COUNT == 3
    assert roofs["group_labels"][0] == "N/A"
    assert roofs["group_labels"][1] == "Olivine City"
    assert roofs["group_labels"][4] == "Ecruteak City"
    assert roofs["group_labels"][10] == "Violet City"
    assert roofs["group_labels"][37] == "Ecruteak Shrine Outside"
    assert roofs["overcast_labels"] == [
        "Azalea Town", "Lake of Rage", "Goldenrod City"
    ]
    assert ROOF_PALETTE_INDEX == 6
    variants, _ = stage(sources)

    # Cover every original numbered map group, across all four time names.
    # This is a pure palette patch and does NOT assert corresponding Emerald
    # mapGroup/mapNum IDs. That binding remains outside the staging tool.
    for group in range(1, 38):
        for time in TIME_NAMES:
            family = "violet_ecruteak" if group in (4, 10) else "standard"
            after, review = stage_roof_variant(
                sources, roofs, time=time, family=family, map_group=group
            )
            before = variants[f"{family}.{time}"]
            assert len(after) == len(before) == OUTPUT_BYTES == 224
            expected_pair = roof_colors(roofs, time, map_group=group)[0]
            assert struct.unpack_from("<2H", after, 6 * 32 + 2) == expected_pair
            assert review["source"]["original_index"] == group
            assert review["source"]["source_selection"] == "map_group"
            assert review["source"]["source_label"] == roofs["group_labels"][group]
            assert not review["runtime_time_switching_implemented"]
            assert not review["ready_for_live_registration"]
            # Do not modify other colors, including roof color 0 and color 3.
            assert after[:6 * 32 + 2] == before[:6 * 32 + 2]
            assert after[6 * 32 + 6:] == before[6 * 32 + 6:]
            assert after[6 * 32] == before[6 * 32]
            assert after[6 * 32 + 30:] == before[6 * 32 + 30:]

    # source morning and day use the same pair, but their base palette tints
    # still remain independently staged by the earlier palette converter.
    for group in (1, 4, 10, 37):
        m = roof_colors(roofs, "morn", map_group=group)[0]
        d = roof_colors(roofs, "day", map_group=group)[0]
        n = roof_colors(roofs, "nite", map_group=group)[0]
        e = roof_colors(roofs, "eve", map_group=group)[0]
        assert m == d
        assert len(m) == len(n) == len(e) == 2

    # Each numbered overcast regime has a distinct pinned source pair.
    for weather in range(1, SOURCE_OVERCAST_COUNT + 1):
        for time in TIME_NAMES:
            after, review = stage_roof_variant(
                sources, roofs, time=time, overcast_index=weather
            )
            before = variants[f"overcast.{time}"]
            assert len(after) == OUTPUT_BYTES
            assert struct.unpack_from("<2H", after, 6 * 32 + 2) == (
                roof_colors(roofs, time, overcast_index=weather)[0]
            )
            assert after[:6 * 32 + 2] == before[:6 * 32 + 2]
            assert after[6 * 32 + 6:] == before[6 * 32 + 6:]
            assert review["source_family"] == "overcast"
            assert review["source"]["original_index"] == weather
            assert review["source"]["source_selection"] == "overcast"
            assert not review["overcast_event_conditions_implemented"]

    # A normal map group cannot be inferred from weather, and vice versa.
    reject(lambda: roof_colors(roofs, "day"))
    reject(lambda: roof_colors(roofs, "day", map_group=2, overcast_index=1))
    reject(lambda: roof_colors(roofs, "day", map_group=0))
    reject(lambda: roof_colors(roofs, "day", map_group=38))
    reject(lambda: roof_colors(roofs, "day", overcast_index=0))
    reject(lambda: roof_colors(roofs, "day", overcast_index=4))
    reject(lambda: roof_colors(roofs, "moon", map_group=1))
    reject(lambda: apply_roof_colors(b"\0" * 223, (0, 0)))
    reject(lambda: apply_roof_colors(b"\0" * 224, (0x8000, 0)))
    reject(lambda: apply_roof_colors(b"\0" * 224, (1,)))
    reject(lambda: stage_roof_variant(sources, roofs, time="day",
                                     family="overcast", map_group=1))
    reject(lambda: stage_roof_variant(sources, roofs, time="day",
                                     family="standard", overcast_index=1))
    reject(lambda: stage_roof_variant(sources, roofs, time="day",
                                     family="violet_ecruteak", overcast_index=2))
    reject(lambda: parse_roof_rows("RGB 32,0,0", 1, "invalid"))
    reject(lambda: parse_roof_rows("RGB " + ",".join(["32", "0", "0"] * 6)
                                  + " ; invalid", 1, "invalid"))
    reject(lambda: parse_roof_rows("RGB " + ",".join(["1", "2", "3"] * 6)
                                  + " ; valid", 2, "truncated"))
    reject(lambda: parse_map_group_numbers("newgroup ; 1\nnewgroup ; 3"))

    # Explicitly verify the original engine patches the two roof shades,
    # rather than replacing the whole palette or palette 0.
    provenance = (ROOT / "tools/polished_port/manifests/johto_traditional_outdoor.json").read_text()
    assert "3fa43192379df5c3e7b09a08e4d5d79af4f02f42" in provenance

    print("PASS: 37 authentic 1-based map-group roofs across four times")
    print("PASS: 3 original overcast roof sets across four times")
    print("PASS: only 4 roof shade bytes changed in 224-byte staging palette")
    print("PASS: unknown map/weather/time values fail closed; no live registration")


if __name__ == "__main__":
    main()
