#!/usr/bin/env python3
"""Emit canonical C RGB555 tables directly from pinned Polished Crystal sources.

The emitted file is *data only*. The separate Emerald-side composer selects
colors by map group and time. Do not hand-edit the output or invent colors.

    python3 tools/polished_port/scripts/emit_johto_palette_c_tables.py \
        --output src/data/tilesets/polished_johto_palette_tables.h

Pass --check to compare an existing generated file byte-for-byte without
writing anything. CI runs --check, guarding against source/data drift.
"""
import argparse
from pathlib import Path

from stage_johto_time_palettes import FAMILIES, TIME_NAMES, load_source_palettes, palette_for
from stage_johto_roof_palettes import load_roof_tables, roof_colors

PROJECT_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_OUTPUT = PROJECT_ROOT / "src/data/tilesets/polished_johto_palette_tables.h"


def emit_color_row(colors, indentation):
    return (" " * indentation + "{ " + ", ".join(f"0x{x:04X}" for x in colors) + " },")


def generate(sources, roofs):
    lines = [
        "// Generated from Rangi42/polishedcrystal 3fa43192379df5c3e7b09a08e4d5d79af4f02f42.",
        "// Original source palette RGB555 values. Do not redraw, retint or edit by hand.",
        "// Generator: tools/polished_port/scripts/emit_johto_palette_c_tables.py",
        "// Family order: standard, violet_ecruteak, overcast.",
        "// Source period order: morn, day, nite, eve (NOT Emerald enum order).",
        "// 7 CGB banks, 4 RGB555 colors each, then original roof shade overrides.",
        "#ifndef GUARD_POLISHED_JOHTO_PALETTE_TABLES_H",
        "#define GUARD_POLISHED_JOHTO_PALETTE_TABLES_H",
        "",
        "static const u16 sPolishedJohtoPaletteColors[3][4][7][4] =",
        "{",
    ]
    for family in FAMILIES:
        lines.append(f"    // {family}")
        lines.append("    {")
        for time_index, time in enumerate(TIME_NAMES):
            lines.append(f"        // {time}")
            lines.append("        {")
            for bank in palette_for(sources, family, time_index):
                lines.append(emit_color_row(bank, 12))
            lines.append("        },")
        lines.append("    },")
    lines.extend(["};", "",
                  "// Group 0 is original 'N/A'; actual source map groups are 1..37.",
                  "// Each row: (morning/day pair, nighttime pair, evening pair).",
                  "static const u16 sPolishedJohtoRoofColors[38][3][2] =",
                  "{"])
    for group in range(38):
        pairs = [roof_colors(roofs, t, map_group=group)[0]
                 if group else roofs["groups"][0][i:i + 2]
                 for i, t in ((0, "day"), (2, "nite"), (4, "eve"))]
        lines.append(f"    // {group}: {roofs['group_labels'][group]}")
        lines.append("    {")
        for pair in pairs:
            lines.append(emit_color_row(pair, 8))
        lines.append("    },")
    lines.extend(["};", "",
                  "// Original GetOvercastIndex returns 1..3, so table index is result-1.",
                  "static const u16 sPolishedJohtoOvercastRoofColors[3][3][2] =",
                  "{"])
    for index in range(1, 4):
        lines.append(f"    // {index}: {roofs['overcast_labels'][index - 1]}")
        lines.append("    {")
        for time in ("day", "nite", "eve"):
            lines.append(emit_color_row(roof_colors(roofs, time, overcast_index=index)[0], 8))
        lines.append("    },")
    lines.extend(["};", "", "#endif // GUARD_POLISHED_JOHTO_PALETTE_TABLES_H", ""])
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    expected = generate(load_source_palettes(), load_roof_tables())
    if args.check:
        if not args.output.is_file() or args.output.read_text(encoding="utf-8") != expected:
            parser.error(f"Generated Johto palette table is missing or stale: {args.output}")
        print("PASS: C RGB555 palette tables match pinned Johto source byte-for-byte")
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(expected, encoding="utf-8")
        print(f"Generated authentic Johto C RGB555 palettes: {args.output}")


if __name__ == "__main__":
    main()
