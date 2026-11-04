#!/usr/bin/env python3
"""Check the real owner-list framebuffer against source art and font glyphs.

The 24-owner fixture covers party/PC data, paging, circular-buffer wrap,
the last-entry boundary, return from a boxed summary and main-menu re-entry.
"""
import re
import struct
import sys
from pathlib import Path

from PIL import Image, ImageChops
from make_hgss_ribbon_list import ASSETS, OUT, validate

ROOT = Path(__file__).resolve().parents[2]


def rgb555(color):
    return tuple(((color >> shift) & 31) * 255 // 31 for shift in (0, 5, 10))


def main(base):
    validate()
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

    def codes(text):
        # Decimal right alignment uses CHAR_SPACER, a full digit-width gap.
        return [0x77 if ch == " " else charmap[ch] for ch in text]

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

    def expected_frame(top, selected, total):
        frame = expected_bg.copy()
        draw_text(frame, codes("RIBBONS"), 8, 9)
        index = codes(f"{selected + 1:3d}/{total:3d}")
        draw_text(frame, index, 176 + (56 - sum(widths[c] for c in index)) // 2, 9)
        for slot in range(min(6, total - top)):
            y = 24 + slot * 16
            for yy in range(16):
                for xx in range(200):
                    tile = (yy // 8) * 25 + xx // 8
                    byte = row[tile * 32 + (yy % 8) * 4 + (xx % 8) // 2]
                    frame.putpixel((24 + xx, y + yy), palette[1][(byte >> (4 * (xx % 2))) & 15])
            owner = top + slot
            if owner < 4:
                name, level, count, gender = (
                    ("Pikachu", 50, 32, "♀"), ("Eevee", 35, 7, "♀"),
                    ("Ditto", 20, 2, None), ("Bulbasaur", 5, 1, "♀"),
                )[owner]
            else:
                name, level, count, gender = f"WWWWWWWWWW{owner - 3:02d}", 100, 1, None
            draw_text(frame, codes(name), 32, y + 1)
            # The genderless field uses the game's invisible spacer glyph.
            end = draw_text(frame, [charmap[gender]] if gender else [0x77], 128, y + 1, 6)
            draw_text(frame, codes("/") + [0x105] + codes(str(level)), end, y + 1)
            draw_text(frame, codes(f"{count:2d}"), 208, y + 1)
        return frame

    states = {
        "owners_first": (0, 0), "owners_page_02": (6, 6),
        "owners_page_04": (18, 18), "owners_last": (18, 23),
        "owners_last_after_down": (18, 23), "owners_wrap_up": (15, 15),
        "owners_return_from_box": (15, 15), "owners_back_at_start": (0, 0),
        "owners_reentered": (0, 0),
        "owners_short": (0, 0),
    }
    images = {}
    for name, (top, selected) in states.items():
        actual = Image.open(base / f"{name}.png").convert("RGB")
        assert actual.size == (240, 160), name
        total = 4 if name == "owners_short" else 24
        expected = expected_frame(top, selected, total)
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
        print(f"{name}: {checked} art/text pixels match; selected {selected + 1}/{total}")
    # Returning from the PC summary must restore exactly the selected list.
    assert not ImageChops.difference(images["owners_wrap_up"].crop((36, 29, 240, 115)), images["owners_return_from_box"].crop((36, 29, 240, 115))).getbbox()
    summary = Image.open(base / "owners_box_summary.png").convert("RGB")
    assert ImageChops.difference(summary, images["owners_wrap_up"]).getbbox()
    print("Verified party/PC names, levels, ribbon counts, paging, wrap and re-entry")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: verify_hgss_ribbon_list_screens.py SCREENSHOT_DIR")
    main(Path(sys.argv[1]))
