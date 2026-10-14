#!/usr/bin/env python3
"""Regression-test original Johto map-to-Emerald-clock staging selection."""
from pathlib import Path
import struct

from route_johto_palette import (
    EMERALD_TO_SOURCE_TIME,
    ORIGINAL_WEATHER_CANDIDATES,
    OVERCAST,
    RTC_HEADER,
    TRADITIONAL,
    load_source_map_rules,
    parse_source_constants,
    parse_source_maps,
    select_map_palette,
    source_time,
    validate_emerald_time_enum,
    validate_source_weather_candidates,
)
from stage_johto_time_palettes import load_source_palettes, stage
from stage_johto_roof_palettes import load_roof_tables, roof_colors

FIXTURES = Path(__file__).resolve().parents[1] / "fixtures"


def rejects(fn):
    try:
        fn()
    except ValueError:
        return
    raise AssertionError("invalid map/time/weather/palette assignment accepted")


def main():
    rules, specials = load_source_map_rules()
    source_palettes = load_source_palettes()
    roofs = load_roof_tables()
    assert len(rules) == 605
    assert sum(m["source_tileset"] == TRADITIONAL for m in rules.values()) == 24
    assert sum(m["source_tileset"] == OVERCAST for m in rules.values()) == 14
    assert len({m["source_map_group"] for m in rules.values()}) == 37
    assert rules["NewBarkTown"]["source_map_group"] == 24
    assert rules["NewBarkTown"]["source_map_number"] == 4
    assert rules["CherrygroveCity"]["source_map_group"] == 26
    assert rules["CherrygroveCity"]["source_map_number"] == 4
    assert rules["EcruteakCity"]["source_map_group"] == 4
    assert rules["VioletCity"]["source_map_group"] == 10
    assert rules["DragonsDenB1F"]["source_palette_mode"] == "PALETTE_NITE"
    assert rules["DragonsDenB1F"]["source_environment"] == "CAVE"

    assert validate_emerald_time_enum(RTC_HEADER.read_text(encoding="utf-8")) == [
        "TIME_MORNING", "TIME_DAY", "TIME_EVENING", "TIME_NIGHT"
    ]
    assert [source_time(i) for i in range(4)] == [
        "morn", "day", "eve", "nite"
    ]
    assert [source_time(s) for s in EMERALD_TO_SOURCE_TIME] == [
        "morn", "day", "eve", "nite"
    ]
    assert validate_source_weather_candidates(
        (FIXTURES / "johto_overcast_conditions.asm").read_text(encoding="utf-8")
    )
    assert ORIGINAL_WEATHER_CANDIDATES == {
        "AzaleaTown": 1, "Route33": 1,
        "LakeOfRage": 2, "Route43": 2,
    }

    variants, _ = stage(source_palettes)
    # One resolved source map, every Emerald clock time. RGB555 index 6
    # retains the selected group roof pair; this is never a Hoenn map ID.
    for map_name in ("NewBarkTown", "CherrygroveCity", "EcruteakCity", "VioletCity"):
        record = rules[map_name]
        expected_family = "violet_ecruteak" if map_name in ("EcruteakCity", "VioletCity") else "standard"
        for idx, time in enumerate(("TIME_MORNING", "TIME_DAY", "TIME_EVENING", "TIME_NIGHT")):
            raw, review = select_map_palette(
                map_name, idx, source_maps=rules, special_overrides=specials,
                palette_sources=source_palettes, roof_tables=roofs,
            )
            baseline = variants[f"{expected_family}.{source_time(time)}"]
            assert len(raw) == len(baseline) == 224
            assert review["source_map"] == record
            assert review["source_time_palette"] == source_time(idx)
            assert review["palette_family"] == expected_family
            assert review["source_map_id_is_not_emerald_map_id"]
            assert not review["emerald_map_mapping_implemented"]
            assert not review["runtime_rtc_palette_switch_implemented"]
            assert not review["ready_for_live_registration"]
            assert raw[:194] == baseline[:194]  # Before first patched roof color.
            assert raw[198:] == baseline[198:]
            assert struct.unpack_from("<2H", raw, 194) == roof_colors(
                roofs, source_time(idx), map_group=record["source_map_group"]
            )[0]
            assert raw == select_map_palette(
                map_name, time, source_maps=rules, special_overrides=specials,
                palette_sources=source_palettes, roof_tables=roofs,
            )[0]

    # Specific guard against palette-index order swap: an evening label
    # should select 'eve', NOT Polished Crystal's third 'nite' block.
    raw_evening, _ = select_map_palette(
        "NewBarkTown", "TIME_EVENING", source_maps=rules,
        special_overrides=specials, palette_sources=source_palettes, roof_tables=roofs,
    )
    raw_night, _ = select_map_palette(
        "NewBarkTown", "TIME_NIGHT", source_maps=rules,
        special_overrides=specials, palette_sources=source_palettes, roof_tables=roofs,
    )
    assert raw_evening != raw_night

    for map_name, index in ORIGINAL_WEATHER_CANDIDATES.items():
        with_weather, review = select_map_palette(
            map_name, "TIME_DAY", source_overcast_index=index,
            source_maps=rules, special_overrides=specials,
            palette_sources=source_palettes, roof_tables=roofs,
        )
        assert review["palette_family"] == "overcast"
        assert review["original_overcast_index_provided_by_caller"] == index
        assert review["original_roof_selection"]["source_selection"] == "overcast"
        assert struct.unpack_from("<2H", with_weather, 194) == roof_colors(
            roofs, "day", overcast_index=index
        )[0]
        without_weather, normal = select_map_palette(
            map_name, "TIME_DAY", source_maps=rules,
            special_overrides=specials, palette_sources=source_palettes,
            roof_tables=roofs,
        )
        assert normal["palette_family"] == "standard"
        assert with_weather != without_weather

    def pick(name, time="TIME_DAY", weather=0):
        return select_map_palette(
            name, time, source_overcast_index=weather,
            source_maps=rules, special_overrides=specials,
            palette_sources=source_palettes, roof_tables=roofs,
        )
    rejects(lambda: pick("BellchimeTrail"))  # source special palette is not imported
    rejects(lambda: pick("DragonsDenB1F"))   # nighttime cave is not an outdoor slice
    rejects(lambda: pick("NewBarkTown", weather=1))
    rejects(lambda: pick("EcruteakCity", weather=1))
    rejects(lambda: pick("Route43", weather=1))
    rejects(lambda: pick("Route33", weather=2))
    rejects(lambda: pick("Route45", weather=2))
    rejects(lambda: pick("LakeOfRage", weather=3))
    rejects(lambda: pick("OlivineCity"))     # uses a different GBC tileset
    rejects(lambda: pick("NotARealTown"))
    rejects(lambda: pick("NewBarkTown", "TIME_DUSK"))
    rejects(lambda: pick("NewBarkTown", 4))
    rejects(lambda: pick("NewBarkTown", -1))
    rejects(lambda: pick("NewBarkTown", True))
    rejects(lambda: pick("NewBarkTown", weather=True))
    rejects(lambda: validate_emerald_time_enum(
        "enum TimeOfDay { TIME_MORNING,TIME_DAY,TIME_NIGHT,TIME_EVENING,TIMES_OF_DAY_COUNT,};"
    ))
    rejects(lambda: parse_source_constants("newgroup\nmap_const ONE, 1, 1"))
    rejects(lambda: parse_source_maps("MapGroup1:\n map One, OTHER",
                                     (FIXTURES / "johto_map_constants.asm").read_text()))
    rejects(lambda: validate_source_weather_candidates("GetOvercastIndex::"))

    print("PASS: 605 pinned map headers with 24 Johto Traditional + 14 overcast maps")
    print("PASS: source map groups, map numbers, and special colors resolved")
    print("PASS: Emerald MORNING/DAY/EVENING/NIGHT mapped to original palette names")
    print("PASS: original overcast index is caller-supplied and restricted to eligible maps")
    print("PASS: unmigrated cave/special palettes and invalid IDs remain blocked")


if __name__ == "__main__":
    main()
