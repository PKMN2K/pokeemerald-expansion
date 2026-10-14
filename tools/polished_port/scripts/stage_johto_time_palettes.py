#!/usr/bin/env python3
"""Stage authentic Polished Crystal Johto outdoor RGB555 palettes for Emerald.

Reads four pinned text fixtures from the Polished Crystal source:
  gfx/tilesets/bg_tiles.pal
  data/maps/environment_colors.asm
  gfx/tilesets/violet_ecruteak.pal
  gfx/tilesets/bg_tiles_overcast.pal

Produces a *review-only* collection of 7 GBA 4bpp palette banks for each
outdoor time and palette family. Each original CGB palette has four colors
which are preserved bit-for-bit as RGB555; the remaining 12 colors in each
GBA bank are zero-filled (unused). No new pixel art or colors are drawn.

These outputs are not tileset-ready: Emerald uses global palette slots
0..5 (primary) and 6..12 (secondary); runtime roof map-group overrides,
dynamic weather, and transparency still need integration. In particular,
the default outdoor palette 6 is a BASE roof palette; Polished Crystal
changes its two roof shades for a particular map group in LoadMapPals.
"""
import argparse
import json
from pathlib import Path
import re
import struct

FIXTURES = Path(__file__).resolve().parents[1] / "fixtures"
TIME_NAMES = ("morn", "day", "nite", "eve")
FAMILIES = ("standard", "violet_ecruteak", "overcast")
BG_SOURCE_PALETTES = 44  # 8 each for four times + 8 indoor + 4 water
GBA_COLORS_PER_PALETTE = 16
CGB_COLORS_PER_PALETTE = 4
GBA_PALETTE_BYTES = 32
PALETTES_PER_VARIANT = 7
OUTPUT_BYTES = PALETTES_PER_VARIANT * GBA_PALETTE_BYTES


def parse_rgb_palettes(text, name, expected_count):
    # Palette sources use RGBDS 'RGB' directives with 4 triplets per line.
    # Only the color build branch is authoritative (never MONOCHROME).
    palettes = []
    for line in text.splitlines():
        raw = line.split(";", 1)[0].strip()
        if raw.lower() == "else" and name in ("bg_tiles", "overcast"):
            break
        if not raw.startswith("RGB "):
            continue
        channels = [int(piece.strip(), 10) for piece in raw[4:].split(",")]
        if len(channels) != CGB_COLORS_PER_PALETTE * 3:
            raise ValueError(f"{name}: expected 12 RGB channel values per line")
        if not all(0 <= ch <= 31 for ch in channels):
            raise ValueError(f"{name}: color channel outside 0..31")
        palettes.append(tuple(
            channels[n] | channels[n + 1] << 5 | channels[n + 2] << 10
            for n in range(0, len(channels), 3)
        ))
    if len(palettes) != expected_count:
        raise ValueError(f"{name}: expected {expected_count} palettes, found {len(palettes)}")
    return palettes


def parse_outdoor_colors(text):
    # The original environment lookup uses 8 palette references per time:
    # 0..6 = graphics; 7 = the text palette, loaded elsewhere.
    start = text.find(".OutdoorColors:")
    stop = text.find(".IndoorColors:", start)
    if start < 0 or stop <= start:
        raise ValueError("missing original .OutdoorColors table")
    rows = []
    for line in text[start:stop].splitlines():
        raw = line.split(";", 1)[0].strip()
        if not raw.startswith("db "):
            continue
        tokens = [token.strip() for token in raw[3:].split(",")]
        if len(tokens) != 8 or not all(re.fullmatch(r"\$[0-9a-fA-F]{2}", x) for x in tokens):
            raise ValueError("malformed outdoor source palette table")
        rows.append(tuple(int(x[1:], 16) for x in tokens))
    if len(rows) != 4:
        raise ValueError(f"expected 4 outdoor source palette rows, got {len(rows)}")
    # Source index 0x28..0x2B is special water at each time. Reject a
    # surprise change rather than inadvertently using standard water.
    for time, row in enumerate(rows):
        expected = (time * 8, time * 8 + 1, time * 8 + 2,
                    40 + time, time * 8 + 4, time * 8 + 5,
                    time * 8 + 6, time * 8 + 7)
        if row != expected:
            raise ValueError(f"unexpected outdoor palette mapping at {TIME_NAMES[time]}: {row}")
    return rows


def load_source_palettes(fixture_dir=FIXTURES):
    sources = {
        "bg_tiles": (fixture_dir / "johto_bg_tiles.pal").read_text(encoding="utf-8"),
        "environment": (fixture_dir / "johto_environment_colors.asm").read_text(encoding="utf-8"),
        "violet_ecruteak": (fixture_dir / "johto_violet_ecruteak.pal").read_text(encoding="utf-8"),
        "overcast": (fixture_dir / "johto_bg_tiles_overcast.pal").read_text(encoding="utf-8"),
    }
    return {
        "bg_tiles": parse_rgb_palettes(sources["bg_tiles"], "bg_tiles", BG_SOURCE_PALETTES),
        "environment": parse_outdoor_colors(sources["environment"]),
        "violet_ecruteak": parse_rgb_palettes(sources["violet_ecruteak"], "violet_ecruteak", 32),
        "overcast": parse_rgb_palettes(sources["overcast"], "overcast", 32),
    }


def palette_for(source, family, time_index):
    if family == "standard":
        return tuple(source["bg_tiles"][i] for i in source["environment"][time_index][:7])
    if family not in ("violet_ecruteak", "overcast"):
        raise ValueError(f"unsupported palette family: {family}")
    # Special full time-of-day palettes have 8 CGB banks per time, but
    # LoadSevenTimeOfDayBGPalettes copies exactly the first 7.
    return tuple(source[family][time_index * 8 + i] for i in range(7))


def expand_seven_palettes(palettes):
    if len(palettes) != PALETTES_PER_VARIANT:
        raise ValueError("expected exactly seven CGB palette banks")
    out = bytearray()
    for colors in palettes:
        if len(colors) != 4 or any(not 0 <= v < 0x8000 for v in colors):
            raise ValueError("expected four genuine RGB555 colors per CGB bank")
        out.extend(struct.pack("<16H", *colors, *([0] * 12)))
    assert len(out) == OUTPUT_BYTES
    return bytes(out)


def stage(source):
    outputs = {}
    report = {
        "status": "staged_rgb555_only_not_registered",
        "time_names": list(TIME_NAMES),
        "palette_families": list(FAMILIES),
        "source_banks_per_time": 7,
        "gba_bytes_per_family_time": OUTPUT_BYTES,
        "source_to_global_palette_slot": {str(i): i for i in range(7)},
        "primary_slots": list(range(6)),
        "secondary_slots": list(range(6, 13)),
        "variants": {},
        "source_color_order": "RGBDS RGB 5-bit R, 5-bit G, 5-bit B -> GBA RGB555 little endian",
        "unresolved": [
            "Authentic graphics banks/VRAM tile ordering must be loaded separately",
            "Slot 6 is supplied by the secondary tileset and used by both banks",
            "Polished Crystal replaces two roof colors in slot 6 by map group and time",
            "Special palettes are selected by map/landmark/tileset and sometimes weather",
            "GBC per-8x8 priority, tile-0 transparency and collision require review",
            "No time-of-day map palette switching has been added to Emerald yet",
        ],
        "ready_for_live_registration": False,
    }
    for family in FAMILIES:
        for time_idx, name in enumerate(TIME_NAMES):
            pals = palette_for(source, family, time_idx)
            raw = expand_seven_palettes(pals)
            key = f"{family}.{name}"
            outputs[key] = raw
            report["variants"][key] = {
                "filename": key + ".staging.bin",
                "palettes": 7,
                "colors_per_gba_palette": 16,
                "original_rgb555": [[f"0x{v:04X}" for v in pal] for pal in pals],
                "source_palette_indices": (
                    list(source["environment"][time_idx][:7]) if family == "standard"
                    else list(range(time_idx * 8, time_idx * 8 + 7))
                ),
                "primary_loaded_global_slots": [0, 1, 2, 3, 4, 5],
                "secondary_loaded_global_slot": 6,
                "map_specific_roof_override_pending": True,
            }
    return outputs, report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path,
                        help="staging directory for 12 palette binaries + review JSON")
    args = parser.parse_args()
    outputs, report = stage(load_source_palettes())
    args.output.mkdir(parents=True, exist_ok=True)
    for key, raw in outputs.items():
        (args.output / (key + ".staging.bin")).write_bytes(raw)
    (args.output / "johto_palettes.review.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    print(f"Staged {len(outputs)} original RGB555 palette variants: {args.output}")
    print("No registered tileset; roof group overrides and time switching unresolved.")


if __name__ == "__main__":
    main()
