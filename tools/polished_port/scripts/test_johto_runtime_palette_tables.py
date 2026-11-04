#!/usr/bin/env python3
"""Check source parity and fail-closed Emerald runtime bridge registrations."""
from pathlib import Path
import re

from emit_johto_palette_c_tables import DEFAULT_OUTPUT, generate
from stage_johto_roof_palettes import load_roof_tables, roof_colors
from stage_johto_time_palettes import FAMILIES, TIME_NAMES, load_source_palettes, palette_for

ROOT = Path(__file__).resolve().parents[3]
RUNTIME_FILE = ROOT / "src/polished_johto_palette.c"
HEADER_FILE = ROOT / "include/polished_johto_palette.h"
FIELDMAP_FILE = ROOT / "src/fieldmap.c"
OVERWORLD_FILE = ROOT / "src/overworld.c"

# 3 families x 4 periods x 7 CGB banks x 4 colors; plus
# 38 source roof group rows x 3 time pairs x 2 shades; plus
# 3 source overcast regimes x 3 time pairs x 2 shades.
EXPECTED_SOURCE_WORDS = 3 * 4 * 7 * 4 + 38 * 3 * 2 + 3 * 3 * 2


def main():
    source = load_source_palettes()
    roofs = load_roof_tables()
    generated = generate(source, roofs)
    committed = DEFAULT_OUTPUT.read_text(encoding="utf-8")
    assert committed == generated, "C palette table differs from pinned source"
    assert len(re.findall(r"\b0x[0-9A-F]{4}\b", committed)) == EXPECTED_SOURCE_WORDS == 582

    assert len(FAMILIES) == 3 and TIME_NAMES == ("morn", "day", "nite", "eve")
    for family in FAMILIES:
        for index in range(4):
            palettes = palette_for(source, family, index)
            assert len(palettes) == 7
            assert all(len(colors) == 4 for colors in palettes)
            assert all(all(0 <= color < 0x8000 for color in row) for row in palettes)
    for group in range(1, 38):
        assert roof_colors(roofs, "morn", map_group=group)[0] == roof_colors(
            roofs, "day", map_group=group
        )[0]
    assert len(roofs["groups"]) == 38
    assert len(roofs["overcast"]) == 3

    runtime = RUNTIME_FILE.read_text(encoding="utf-8")
    header = HEADER_FILE.read_text(encoding="utf-8")
    fieldmap = FIELDMAP_FILE.read_text(encoding="utf-8")
    overworld = OVERWORLD_FILE.read_text(encoding="utf-8")

    # Only the live visual demo may opt in. Hoenn, Prism and the fixed-palette
    # indoor lab must never inherit an outdoor source map-group palette.
    assert re.search(
        r"static const struct PolishedJohtoMapPaletteRegistration "
        r"sJohtoMapRegistrations\[\]\s*=\s*\{\s*"
        r"\{LAYOUT_POLISHED_JOHTO_DEMO_OUTDOOR,\s*"
        r"&gTileset_PolishedJohtoDemoOutdoorPrimary,\s*"
        r"&gTileset_PolishedJohtoDemoOutdoorSecondary,\s*"
        r"24, JOHTO_PALETTE_STANDARD, 0\},\s*\};",
        runtime,
    ), "Unexpected or incomplete Johto palette registration"
    assert "registration->emeraldLayoutId == 0" in runtime
    assert "registration->primaryTileset == NULL" in runtime
    assert "registration->secondaryTileset == NULL" in runtime
    assert "registration->emeraldLayoutId == gMapHeader.mapLayoutId" in runtime
    assert "registration->primaryTileset == layout->primaryTileset" in runtime
    assert "registration->secondaryTileset == layout->secondaryTileset" in runtime

    # Emerald order is MORNING/DAY/EVENING/NIGHT, but source files are
    # MORNING/DAY/NIGHT/EVENING. Discrete original roof time pairs differ.
    for enum_label, source_index, roof_index in (
        ("TIME_MORNING", 0, 0), ("TIME_DAY", 1, 0),
        ("TIME_NIGHT", 2, 1), ("TIME_EVENING", 3, 2),
    ):
        pattern = (
            r"case " + enum_label + r":\s*"
            r"\*sourcePeriod = " + str(source_index) + r";\s*"
            r"\*roofPeriod = " + str(roof_index) + r";"
        )
        assert re.search(pattern, runtime), f"misrouted source time: {enum_label}"

    assert "PolishedJohtoComposePalette" in header
    assert "sPolishedJohtoPaletteColors[family][sourcePeriod][bank][color]" in runtime
    assert "output[6][1] = roofPairs[roofPeriod][0]" in runtime
    assert "output[6][2] = roofPairs[roofPeriod][1]" in runtime
    assert "PolishedJohtoTryApplyRegisteredMapPalette(mapLayout, skipFaded)" in fieldmap
    assert "PolishedJohtoTryApplyRegisteredMapPalette(gMapHeader.mapLayout, FALSE)" in overworld
    assert "PolishedJohtoIsRegisteredMapLayout(gMapHeader.mapLayout)" in overworld
    assert "#define JOHTO_GBA_PALETTE_COUNT 7" in header
    print("PASS: 582 C RGB555 words match pinned source palette and roof variants")
    print("PASS: source nighttime/evening map explicitly to Emerald enum")
    print("PASS: field palette-load, RTC refresh, and double-tint guards wired")
    print("PASS: only the isolated outdoor demo is registered; Hoenn/Prism are excluded")


if __name__ == "__main__":
    main()
