#!/usr/bin/env python3
"""Regression-test pinned Polished Crystal RGB palette extraction and staging."""
import struct
from pathlib import Path

from stage_johto_time_palettes import (
    BG_SOURCE_PALETTES,
    FAMILIES,
    OUTPUT_BYTES,
    TIME_NAMES,
    expand_seven_palettes,
    load_source_palettes,
    palette_for,
    parse_outdoor_colors,
    parse_rgb_palettes,
    stage,
)

ROOT = Path(__file__).resolve().parents[3]


def rejects(call):
    try:
        call()
    except ValueError:
        return
    raise AssertionError("malformed source palette data accepted")


def rgb555(r, g, b):
    return r | g << 5 | b << 10


def main():
    sources = load_source_palettes()
    assert len(sources["bg_tiles"]) == BG_SOURCE_PALETTES == 44
    assert len(sources["environment"]) == 4
    assert len(sources["violet_ecruteak"]) == 32
    assert len(sources["overcast"]) == 32

    # Original 0x28..0x2b water palette indices are not standard slot 3.
    for idx, row in enumerate(sources["environment"]):
        assert row[3] == 40 + idx
        assert row[7] == 7 + 8 * idx
        assert row[:3] == (idx * 8, idx * 8 + 1, idx * 8 + 2)
        assert palette_for(sources, "standard", idx)[3] == sources["bg_tiles"][40 + idx]
        assert palette_for(sources, "standard", idx)[3] != sources["bg_tiles"][idx * 8 + 3]

    outputs, report = stage(sources)
    assert len(outputs) == len(FAMILIES) * len(TIME_NAMES) == 12
    assert len(report["variants"]) == len(outputs)
    assert not report["ready_for_live_registration"]
    assert report["source_to_global_palette_slot"] == {str(i): i for i in range(7)}
    assert report["primary_slots"] == list(range(6))
    assert report["secondary_slots"] == list(range(6, 13))

    # Original palette values preserved byte-for-byte, even for night.
    assert sources["bg_tiles"][0][0] == rgb555(28, 31, 16)
    assert sources["bg_tiles"][8][0] == rgb555(27, 31, 27)
    assert sources["bg_tiles"][16][0] == rgb555(15, 14, 24)
    assert sources["bg_tiles"][24][0] == rgb555(31, 21, 14)
    assert sources["bg_tiles"][40][1] == rgb555(18, 19, 31)

    for family in FAMILIES:
        for time_idx, name in enumerate(TIME_NAMES):
            key = f"{family}.{name}"
            raw = outputs[key]
            assert len(raw) == OUTPUT_BYTES == 224
            expected_palettes = palette_for(sources, family, time_idx)
            record = report["variants"][key]
            assert record["map_specific_roof_override_pending"]
            assert record["secondary_loaded_global_slot"] == 6
            assert raw == expand_seven_palettes(expected_palettes)
            for pal_idx, color_values in enumerate(expected_palettes):
                vals = struct.unpack_from("<16H", raw, pal_idx * 32)
                assert vals[:4] == color_values
                assert vals[4:] == (0,) * 12
                assert record["original_rgb555"][pal_idx] == [
                    f"0x{color:04X}" for color in color_values
                ]

    # Check the source override actually changes the original palette.
    assert outputs["violet_ecruteak.day"] != outputs["standard.day"]
    assert outputs["overcast.day"] != outputs["standard.day"]
    # Validate fail-closed rejection instead of guessing colors.
    rejects(lambda: parse_rgb_palettes("RGB 1,2,3", "sample", 1))
    rejects(lambda: parse_rgb_palettes("RGB " + ",".join(["32", "0", "0"] * 4), "sample", 1))
    rejects(lambda: parse_rgb_palettes("RGB " + ",".join(["1", "1", "1"] * 4), "sample", 2))
    rejects(lambda: parse_outdoor_colors(".OutdoorColors:\\n db $00,$01\\n.IndoorColors:"))
    rejects(lambda: expand_seven_palettes([(0, 1, 2, 3)] * 6))
    rejects(lambda: expand_seven_palettes([(0, 1, 2, 0x8000)] * 7))
    rejects(lambda: palette_for(sources, "invented_palette", 0))

    # Keep the assertions grounded in the original source (not a redraw).
    manifest = (ROOT / "tools/polished_port/manifests/johto_traditional_outdoor.json").read_text()
    assert "3fa43192379df5c3e7b09a08e4d5d79af4f02f42" in manifest
    print("PASS: 12 authentic RGB555 time/family variants, 224 bytes each")
    print("PASS: 4-times-of-day water remapping and original RGB555 colors")
    print("PASS: special map/overcast palettes remain distinct")
    print("PASS: runtime roof overrides and tileset registration remain blocked")


if __name__ == "__main__":
    main()
