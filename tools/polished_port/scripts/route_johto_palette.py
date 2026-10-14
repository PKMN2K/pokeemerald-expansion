#!/usr/bin/env python3
"""Select authentic Polished Crystal Johto palettes by original map + Emerald time.

This is an import-time staging bridge. The original Polished Crystal maps
have 1-based source map groups and map numbers that are NOT interchangeable
with Emerald/Hoenn map group IDs. The source map header and constants are
both validated against pinned upstream fixtures.

Emerald TimeOfDay enum order is MORNING=0, DAY=1, EVENING=2, NIGHT=3.
Polished Crystal palette file order is morn, day, nite, eve. Resolve this
explicitly, not by indexing the palettes with the enum integer.

Only standard Johto Traditional outside maps and pinned Violet/Ecruteak
special colors are supported. Bellchime Trail requires its own special
source palette; Dragon's Den B1F is a night cave with non-outdoor semantics.
Overcast maps require an explicit original weather index; do not derive
weather from the name or randomly invent an event outcome.

No Emerald map headers, live tilesets, renderer hooks, RTC or weather events
are modified by this command. Output is review/staging only.

Example:
  python3 tools/polished_port/scripts/route_johto_palette.py \
    --source-map NewBarkTown --emerald-time TIME_EVENING \
    --output /tmp/new_bark_eve.staging.bin
"""
import argparse
import json
from pathlib import Path
import re

from stage_johto_time_palettes import FIXTURES, load_source_palettes
from stage_johto_roof_palettes import load_roof_tables, stage_roof_variant

MAP_HEADER_FIXTURE = FIXTURES / "johto_map_headers.asm"
MAP_CONSTANTS_FIXTURE = FIXTURES / "johto_map_constants.asm"
SPECIAL_PALETTE_FIXTURE = FIXTURES / "johto_special_map_palettes.asm"
WEATHER_FIXTURE = FIXTURES / "johto_overcast_conditions.asm"
RTC_HEADER = Path(__file__).resolve().parents[3] / "include/constants/rtc.h"

TRADITIONAL = "TILESET_JOHTO_TRADITIONAL"
OVERCAST = "TILESET_JOHTO_OVERCAST"
SUPPORTED_TILESETS = (TRADITIONAL, OVERCAST)
# Source map-index values are NOT Emerald map-index values.
EXPECTED_SOURCE_MAP_GROUPS = 37
EXPECTED_SOURCE_MAPS = 605
EXPECTED_TRADITIONAL_MAPS = 24
EXPECTED_OVERCAST_MAPS = 14

# Source special map palette *symbols*. Any other map-specific source
# palette must be imported first instead of silently using base colors.
SUPPORTED_SPECIAL_COLOR_TABLES = {
    "VioletEcruteakPalette": "violet_ecruteak",
}
# Source event logic is not linked to Emerald yet. These indices may be
# passed only after the caller determines original weather eligibility.
ORIGINAL_WEATHER_CANDIDATES = {
    "AzaleaTown": 1,
    "Route33": 1,
    "LakeOfRage": 2,
    "Route43": 2,
}
EMERALD_TIME_NAMES = ("TIME_MORNING", "TIME_DAY", "TIME_EVENING", "TIME_NIGHT")
EMERALD_TO_SOURCE_TIME = {
    "TIME_MORNING": "morn",
    "TIME_DAY": "day",
    "TIME_EVENING": "eve",
    "TIME_NIGHT": "nite",
}


def validate_emerald_time_enum(text):
    match = re.search(r"enum\s+TimeOfDay\s*\{([^}]+)\}", text, re.DOTALL)
    if not match:
        raise ValueError("missing Emerald TimeOfDay enum")
    order = [s.strip().split("=")[0].strip() for s in match.group(1).split(",")]
    order = [s for s in order if s and s != "TIMES_OF_DAY_COUNT"]
    if order != list(EMERALD_TIME_NAMES):
        raise ValueError(f"Emerald TimeOfDay ordering changed: {order}")
    return order


def source_time(emerald_time):
    if type(emerald_time) is int:
        if not 0 <= emerald_time < len(EMERALD_TIME_NAMES):
            raise ValueError(f"unsupported Emerald TimeOfDay integer: {emerald_time}")
        emerald_time = EMERALD_TIME_NAMES[emerald_time]
    if emerald_time not in EMERALD_TO_SOURCE_TIME:
        raise ValueError(f"unsupported Emerald TimeOfDay value: {emerald_time}")
    return EMERALD_TO_SOURCE_TIME[emerald_time]


def parse_source_constants(text):
    group, mapnum = 0, 0
    records = {}
    for line in text.splitlines():
        if re.match(r"^\s*newgroup\b", line):
            group += 1
            mapnum = 0
        match = re.match(r"^\s*map_const\s+([A-Z0-9_]+)\s*,", line)
        if match:
            mapnum += 1
            records[(group, mapnum)] = match.group(1)
    if group != EXPECTED_SOURCE_MAP_GROUPS or len(records) != EXPECTED_SOURCE_MAPS:
        raise ValueError(f"original map constant count changed: {group} groups, {len(records)} maps")
    return records


def parse_source_maps(map_headers, map_constants):
    expected = parse_source_constants(map_constants)
    group, mapnum = 0, 0
    maps, seen = {}, set()
    for line in map_headers.splitlines():
        match = re.match(r"^MapGroup(\d+):\s*$", line)
        if match:
            if int(match.group(1)) != group + 1:
                raise ValueError("source group header sequence changed")
            group += 1
            mapnum = 0
            continue
        match = re.match(r"^\s*map\s+([A-Za-z0-9_]+)\s*,\s*(.*)$", line)
        if not match:
            continue
        mapnum += 1
        name = match.group(1)
        fields = [part.strip() for part in match.group(2).split(",")]
        if len(fields) != 7 or (group, mapnum) not in expected:
            raise ValueError(f"unexpected map header field count for {name}")
        if name in seen:
            raise ValueError(f"duplicate source map: {name}")
        seen.add(name)
        maps[name] = {
            "source_map_name": name,
            "source_map_constant": expected[(group, mapnum)],
            "source_map_group": group,
            "source_map_number": mapnum,
            "source_tileset": fields[0],
            "source_environment": fields[1],
            "source_sign_type": fields[2],
            "source_landmark": fields[3],
            "source_music": fields[4],
            "source_phone_flag": fields[5],
            "source_palette_mode": fields[6],
        }
    if len(maps) != len(expected) or group != EXPECTED_SOURCE_MAP_GROUPS:
        raise ValueError(f"map headers do not cover all original maps: {len(maps)}")
    source_count = sum(m["source_tileset"] == TRADITIONAL for m in maps.values())
    overcast_count = sum(m["source_tileset"] == OVERCAST for m in maps.values())
    if (source_count, overcast_count) != (EXPECTED_TRADITIONAL_MAPS, EXPECTED_OVERCAST_MAPS):
        raise ValueError(f"source Johto tileset populations changed: {source_count}, {overcast_count}")
    return maps


def parse_map_palette_overrides(text):
    overrides = {}
    for line in text.splitlines():
        match = re.match(
            r"^\s*special_bg_pal\s+map\s*,\s*([A-Z0-9_]+)\s*,\s*"
            r"(PAL_TIMEOFDAY|PAL_SINGLE|PAL_SPECIAL)\s*,\s*([A-Za-z0-9_]+)", line
        )
        if match:
            name, mode, symbol = match.groups()
            if name in overrides:
                raise ValueError(f"duplicate special map palette for {name}")
            overrides[name] = {"mode": mode, "source_palette_symbol": symbol}
    for name in ("ECRUTEAK_CITY", "VIOLET_CITY"):
        if overrides.get(name) != {
            "mode": "PAL_TIMEOFDAY", "source_palette_symbol": "VioletEcruteakPalette"
        }:
            raise ValueError(f"pinned Violet/Ecruteak override changed: {name}")
    if overrides.get("BELLCHIME_TRAIL", {}).get("source_palette_symbol") != "BellchimeTrailPalette":
        raise ValueError("pinned Bellchime Trail special palette changed")
    return overrides


def validate_source_weather_candidates(text):
    expected = {
        "AzaleaTown": ("MAP_AZALEA_TOWN", "AZALEA_OVERCAST"),
        "Route33": ("MAP_ROUTE_33", "AZALEA_OVERCAST"),
        "LakeOfRage": ("MAP_LAKE_OF_RAGE", "LAKE_OF_RAGE_OVERCAST"),
        "Route43": ("MAP_ROUTE_43", "LAKE_OF_RAGE_OVERCAST"),
    }
    if set(expected) != set(ORIGINAL_WEATHER_CANDIDATES):
        raise ValueError("original weather routing list changed")
    for map_name, (map_constant, weather_symbol) in expected.items():
        if not re.search(r"\b" + map_constant + r"\b", text):
            raise ValueError(f"source weather selector missing {map_name}")
        if not re.search(r"\b" + weather_symbol + r"\b", text):
            raise ValueError(f"source weather type missing {weather_symbol}")
    return True


def load_source_map_rules(fixture_dir=FIXTURES, rtc_header=RTC_HEADER):
    validate_emerald_time_enum(rtc_header.read_text(encoding="utf-8"))
    validate_source_weather_candidates(
        (fixture_dir / "johto_overcast_conditions.asm").read_text(encoding="utf-8")
    )
    maps = parse_source_maps(
        (fixture_dir / "johto_map_headers.asm").read_text(encoding="utf-8"),
        (fixture_dir / "johto_map_constants.asm").read_text(encoding="utf-8"),
    )
    special = parse_map_palette_overrides(
        (fixture_dir / "johto_special_map_palettes.asm").read_text(encoding="utf-8")
    )
    return maps, special


def select_map_palette(source_map_name, emerald_time, *, source_overcast_index=0,
                       source_maps=None, special_overrides=None,
                       palette_sources=None, roof_tables=None):
    """Return (224 authenticated palette bytes, immutable review metadata).

    source_overcast_index is *already* resolved by the original weather
    conditions. The caller must not treat this function as a weather solver.
    """
    time = source_time(emerald_time)
    if type(source_overcast_index) is not int or source_overcast_index not in (0, 1, 2, 3):
        raise ValueError("original overcast index must be an integer from 0 to 3")
    if source_maps is None or special_overrides is None:
        source_maps, special_overrides = load_source_map_rules()
    if source_map_name not in source_maps:
        raise ValueError(f"unknown pinned source map: {source_map_name}")
    source_map = source_maps[source_map_name]
    if source_map["source_tileset"] not in SUPPORTED_TILESETS:
        raise ValueError(f"source map {source_map_name} uses another tileset")
    if source_map["source_environment"] not in ("TOWN", "ROUTE", "ISOLATED"):
        raise ValueError(f"source map {source_map_name} is not an outdoor map")
    if source_map["source_palette_mode"] != "PALETTE_AUTO":
        raise ValueError(f"source map {source_map_name} requires a fixed palette mode")
    special = special_overrides.get(source_map["source_map_constant"])
    family = "standard"
    if special:
        if special["mode"] != "PAL_TIMEOFDAY":
            raise ValueError(f"unconverted single/special palette on {source_map_name}")
        symbol = special["source_palette_symbol"]
        if symbol not in SUPPORTED_SPECIAL_COLOR_TABLES:
            raise ValueError(f"unconverted original map palette {symbol} on {source_map_name}")
        family = SUPPORTED_SPECIAL_COLOR_TABLES[symbol]

    if source_overcast_index:
        if source_map["source_tileset"] != OVERCAST:
            raise ValueError(f"original overcast weather not supported on {source_map_name}")
        expected = ORIGINAL_WEATHER_CANDIDATES.get(source_map_name)
        if expected != source_overcast_index:
            raise ValueError(f"source weather index {source_overcast_index} not valid for {source_map_name}")
        if special:
            raise ValueError("overcast palette interaction with special map palette not yet resolved")
        family = "overcast"
    elif source_map["source_tileset"] == OVERCAST and source_map_name in ORIGINAL_WEATHER_CANDIDATES:
        # Not a blocker: original GetOvercastIndex can legitimately return 0.
        family = "standard"

    if palette_sources is None:
        palette_sources = load_source_palettes()
    if roof_tables is None:
        roof_tables = load_roof_tables()
    out, review = stage_roof_variant(
        palette_sources, roof_tables, family=family, time=time,
        map_group=source_map["source_map_group"] if not source_overcast_index else None,
        overcast_index=source_overcast_index or None,
    )
    return out, {
        "status": "source_map_palette_staging_only",
        "source_map": source_map,
        "emerald_time_enum": emerald_time,
        "source_time_palette": time,
        "original_overcast_index_provided_by_caller": source_overcast_index,
        "palette_family": family,
        "original_roof_selection": review["source"],
        "global_palette_slots": list(range(7)),
        "tile_graphics_registered": False,
        "source_map_id_is_not_emerald_map_id": True,
        "emerald_map_mapping_implemented": False,
        "runtime_rtc_palette_switch_implemented": False,
        "weather_event_logic_implemented": False,
        "ready_for_live_registration": False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-map", required=True)
    parser.add_argument("--emerald-time", required=True, choices=EMERALD_TIME_NAMES)
    parser.add_argument("--overcast-index", type=int, default=0)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    raw, meta = select_map_palette(
        args.source_map, args.emerald_time, source_overcast_index=args.overcast_index
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(raw)
    args.output.with_suffix(args.output.suffix + ".review.json").write_text(
        json.dumps(meta, indent=2) + "\n", encoding="utf-8"
    )
    print(
        f"Staged source {args.source_map} ({meta['source_map']['source_map_group']}:"
        f"{meta['source_map']['source_map_number']}) in {meta['source_time_palette']} "
        f"({meta['palette_family']}): {args.output}"
    )
    print("NOT LIVE: no Emerald map IDs or runtime clock/weather hooks are registered.")


if __name__ == "__main__":
    main()
