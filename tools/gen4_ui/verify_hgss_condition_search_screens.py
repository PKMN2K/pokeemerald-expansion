#!/usr/bin/env python3
"""Verify real Condition-search frames against original HGSS tiles/fonts.

Covers category-specific ordering, tied ranks, party/PC data, circular-buffer
wrap, short/single lists, and palette restoration on return from the graph.
"""
import re
import struct
import sys
from pathlib import Path

from PIL import Image, ImageChops
from make_hgss_ribbon_list import ASSETS, OUT
from check_hgss_condition_search import check
from verify_hgss_condition_graph_screens import check_polygon

ROOT = Path(__file__).resolve().parents[2]


def rgb555(color):
    return tuple(((color >> shift) & 31) * 255 // 31 for shift in (0, 5, 10))


def main(base):
    check()
    raw = (ASSETS / "summary_bg.4bpp").read_bytes()
    screen = struct.unpack("<1024H", (OUT / "background.bin").read_bytes())
    palette = {bank: [rgb555(c) for c in struct.unpack("<16H", (ASSETS / f"summary_bg.bank_{bank}.gbapal").read_bytes())]
               for bank in (1, 7)}
    row = (OUT / "row.4bpp").read_bytes()
    font = Image.open(ROOT / "graphics/fonts/latin_normal.png")
    widths = re.search(r"gFontNormalLatinGlyphWidths\[\] = \{(.*?)\};", (ROOT / "src/fonts.c").read_text(), re.S)
    widths = list(map(int, re.findall(r"\d+", widths[1])))
    charmap = {ch: int(code, 16) for ch, code in re.findall(r"^'(.)'\s*=\s*([0-9A-F]{2})\s*$", (ROOT / "charmap.txt").read_text(), re.M)}

    def draw_text(image, codes, x, y, foreground=5):
        for code in codes:
            for yy in range(15):
                for xx in range(widths[code]):
                    value = font.getpixel(((code % 16) * 16 + xx, (code // 16) * 16 + yy))
                    if value in (1, 2):
                        image.putpixel((x + xx, y + yy), palette[1][foreground if value == 1 else 2])
            x += widths[code]
        return x

    def codes(text, right_align=False):
        return [0x77 if ch == " " and right_align else charmap[ch] for ch in text]

    expected_bg = Image.new("RGB", (240, 160))
    for y in range(128):
        for x in range(240):
            attr = screen[(y // 8) * 32 + x // 8]
            if not attr:
                color = palette[1][1]
            else:
                xx = 7 - x % 8 if attr & 0x400 else x % 8
                yy = 7 - y % 8 if attr & 0x800 else y % 8
                byte = raw[(attr & 1023) * 32 + yy * 4 + xx // 2]
                color = palette[attr >> 12][(byte >> (4 * (xx % 2))) & 15]
            expected_bg.putpixel((x, y), color)

    party = [
        ("Pikachu", 50, "♀", [255, 40, 80, 120, 10]),
        ("Eevee", 35, "♀", [255, 200, 60, 20, 80]),
        ("Ditto", 20, None, [200, 255, 40, 100, 255]),
        ("Bulbasaur", 5, "♀", [180, 180, 255, 255, 120]),
    ]
    boxed = [(f"WWWWWWWWWW{i + 1:02d}", 100, "♂" if i == 0 else None,
              [160 - i * 5, i * 10, 120 + i * 5, 200 - i * 5, 80 + i * 5]) for i in range(20)]
    categories = ("COOL", "BEAUTY", "CUTE", "SMART", "TOUGH")

    def expected_frame(category, top, selected, total):
        if total == 104:
            owners = [(name, level, gender, [255 - i] * 5)
                      for i, (name, level, gender, _) in enumerate(party)]
            owners += [(f"WWWWWWWWW{i + 1:03d}", 100, "♂" if i == 0 else None, [251 - i] * 5)
                       for i in range(100)]
        else:
            owners = party + boxed if total == 24 else party[:total]
        owners = sorted(owners, key=lambda owner: -owner[3][category])
        value = owners[selected][3][category]
        rank = 1 + sum(owner[3][category] > value for owner in owners)
        frame = expected_bg.copy()
        draw_text(frame, codes(categories[category] + " SEARCH"), 8, 9)
        draw_text(frame, codes("No. ") + codes(f"{rank:3d}", True), 176, 9)
        for slot, owner in enumerate(owners[top:top + 6]):
            y = 24 + slot * 16
            for yy in range(16):
                for xx in range(200):
                    tile = (yy // 8) * 25 + xx // 8
                    byte = row[tile * 32 + (yy % 8) * 4 + (xx % 8) // 2]
                    frame.putpixel((24 + xx, y + yy), palette[1][(byte >> (4 * (xx % 2))) & 15])
            name, level, gender, _ = owner
            draw_text(frame, codes(name), 32, y + 1)
            end = draw_text(frame, [charmap[gender]] if gender else [0x77], 128, y + 1, 9 if gender == "♂" else 6)
            draw_text(frame, codes("/") + [0x105] + codes(str(level)), end, y + 1)
        return frame, rank

    # Values are (category, first visible item, selection, valid-result count).
    # The high-stat egg must never appear in any category or affect the rank.
    states = {
        "cool_first": (0, 0, 0, 24), "cool_first_after_up": (0, 0, 0, 24), "cool_tied": (0, 0, 1, 24),
        "cool_third": (0, 0, 2, 24), "cool_page_02": (0, 6, 6, 24),
        "cool_page_04": (0, 18, 18, 24), "cool_last": (0, 18, 23, 24),
        "cool_last_after_down": (0, 18, 23, 24), "cool_wrap_up": (0, 15, 15, 24),
        "cool_return_from_graph": (0, 15, 15, 24), "cool_back_at_start": (0, 0, 0, 24),
        "beauty": (1, 0, 0, 24), "cute": (2, 0, 0, 24),
        "smart": (3, 0, 0, 24), "tough": (4, 0, 0, 24),
        "cool_reentered": (0, 0, 0, 24), "short_list": (0, 0, 0, 4),
        "single_list": (0, 0, 0, 1), "single_after_down": (0, 0, 0, 1),
        "single_return": (0, 0, 0, 1), "single_after_up": (0, 0, 0, 1),
        "short_last": (0, 0, 3, 4), "short_last_after_down": (0, 0, 3, 4),
        "rank104": (0, 98, 103, 104), "rank104_after_down": (0, 98, 103, 104),
        "rank103": (0, 98, 102, 104),
    }
    for category, name in enumerate(categories[1:], 1):
        states[name.lower() + "_second"] = (category, 0, 1, 24)
        states[name.lower() + "_third"] = (category, 0, 2, 24)
    images = {}
    for name, (category, top, selected, total) in states.items():
        actual = Image.open(base / f"{name}.png").convert("RGB")
        assert actual.size == (240, 160), name
        expected, rank = expected_frame(category, top, selected, total)
        bad = checked = 0
        for y in range(8, 128):
            for x in range(240):
                # Animated pointers are checked visually; exclude only their
                # footprints, keeping all row text/counts and header pixels.
                if 20 <= x < 36 and 24 <= y < 120:
                    continue
                if 110 <= x < 134 and (12 <= y < 29 or 115 <= y < 128):
                    continue
                a, b = actual.getpixel((x, y)), expected.getpixel((x, y))
                bad += max(abs(a[c] - b[c]) for c in range(3)) > 8
                checked += 1
        if bad:
            expected.save(base / f"{name}_expected.png")
            raise SystemExit(f"{name}: {bad}/{checked} source/font pixels differ")
        images[name] = actual
        print(f"{name}: {checked} art/text pixels match; rank {rank}, selected {selected + 1}/{total}")
    footer = images["cool_first"].crop((0, 128, 240, 160))
    for name, frame in images.items():
        assert not ImageChops.difference(footer, frame.crop((0, 128, 240, 160))).getbbox(), f"{name}: stale help bar"
    for before, after in (("cool_wrap_up", "cool_return_from_graph"),
                          ("single_list", "single_return"),
                          ("single_list", "single_after_down")):
        area = (36, 29, 240, 115)
        assert not ImageChops.difference(images[before].crop(area), images[after].crop(area)).getbbox()
    for name, list_name, stats in (("condition_graph", "cool_wrap_up", [105, 110, 175, 145, 135]),
                                   ("single_graph", "single_list", [255, 40, 80, 120, 10])):
        graph = Image.open(base / f"{name}.png").convert("RGB")
        check_polygon(graph, stats, name)
        assert ImageChops.difference(graph, images[list_name]).getbbox()
    print(f"Verified {len(states)} states: all categories, ties, party/PC eggs, genders, three-digit ranks, wrap and returns")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: verify_hgss_condition_search_screens.py SCREENSHOT_DIR")
    main(Path(sys.argv[1]))
