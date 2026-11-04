#!/usr/bin/env python3
"""Check actual detail frames against source pixels, metadata and chart data.

Only animated portraits/stars and the popup pointer are excluded from exact
chrome/text comparison. The live polygon is independently rasterized from
fixture condition values, with a two-pixel allowance at scanline boundaries.
"""
import re
import struct
import sys
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter
from make_hgss_condition_graph import OUT, ASSETS, STORAGE, validate

ROOT = Path(__file__).resolve().parents[2]


def rgb555(value):
    return tuple(((value >> shift) & 31) * 255 // 31 for shift in (0, 5, 10))


def tile_pixel(data, tile, x, y):
    return (data[tile * 32 + y * 4 + x // 2] >> (4 * (x % 2))) & 15


def png_palette(name):
    palette = Image.open(STORAGE / name).getpalette()
    return [rgb555(sum((palette[i * 3 + c] >> 3) << (c * 5) for c in range(3))) for i in range(16)]


class Reference:
    def __init__(self):
        self.pal = {bank: [rgb555(value) for value in struct.unpack("<16H", (ASSETS / f"summary_bg.bank_{bank}.gbapal").read_bytes())]
                    for bank in (1, 7)}
        self.pal[12] = png_palette("markings_menu.png")
        storage = (ROOT / "src/data/swsh_storage_system.h").read_text()
        selected = re.search(r"sHgssMarkingsMenuSelected_Pal\[16\]\s*=\s*\{(.*?)\};", storage, re.S)[1]
        self.pal[13] = [rgb555(int(value, 16)) for value in re.findall(r"0x[\dA-Fa-f]+", selected)]
        raw = (OUT / "graph_bg.4bpp").read_bytes()
        screen = struct.unpack("<1024H", (OUT / "graph_bg.bin").read_bytes())
        self.background = Image.new("RGB", (240, 160))
        for y in range(160):
            for x in range(240):
                attr = screen[y // 8 * 32 + x // 8]
                self.background.putpixel((x, y), self.pal[attr >> 12][tile_pixel(raw, attr & 1023, x % 8, y % 8)])
        self.font = Image.open(ROOT / "graphics/fonts/latin_normal.png")
        widths = re.search(r"gFontNormalLatinGlyphWidths\[\] = \{(.*?)\};", (ROOT / "src/fonts.c").read_text(), re.S)[1]
        self.widths = list(map(int, re.findall(r"\d+", widths)))
        self.charmap = {ch: int(code, 16) for ch, code in re.findall(r"^'(.)'\s*=\s*([0-9A-F]{2})\s*$", (ROOT / "charmap.txt").read_text(), re.M)}
        self.combo = (STORAGE / "marking_combo_hgss.4bpp").read_bytes()

    def codes(self, text):
        return [self.charmap[ch] for ch in text]

    def text(self, image, codes, x, y, fg=5):
        if isinstance(codes, str):
            codes = self.codes(codes)
        for code in codes:
            for yy in range(15):
                for xx in range(self.widths[code]):
                    value = self.font.getpixel((code % 16 * 16 + xx, code // 16 * 16 + yy))
                    if value in (1, 2):
                        image.putpixel((x + xx, y + yy), self.pal[1][fg if value == 1 else 2])
            x += self.widths[code]
        return x

    def frame(self, owner, index, location, marks=0, popup=None):
        frame = self.background.copy()
        self.text(frame, "CONDITION", 8, 9)
        if isinstance(index, int):
            self.text(frame, self.codes("No. ") + [0x77 if ch == " " else self.charmap[ch] for ch in f"{index:3d}"], 176, 9)
        else:
            self.text(frame, index, 176, 9)
        if owner is None:
            self.text(frame, "CANCEL", 8, 25)
        else:
            name, level, gender, _ = owner
            self.text(frame, name, 8, 25)
            end = self.text(frame, [self.charmap[gender]] if gender else [0x77], 104, 25, 9 if gender == "♂" else 6)
            self.text(frame, self.codes("/") + [0x105] + self.codes(str(level)), end, 25)
            self.text(frame, location, 160, 25)
            for slot, bit in enumerate((0, 2, 1, 3)):
                if marks & (1 << bit):
                    for yy in range(8):
                        for xx in range(8):
                            value = tile_pixel(self.combo, slot, xx, yy)
                            if value:
                                frame.putpixel((88 + slot * 8 + xx, 48 + yy), self.pal[12][value])
        if popup is not None:
            raw = (OUT / "markings_panel.4bpp").read_bytes()
            for y in range(120):
                for x in range(88):
                    bank = 12
                    for slot, bit in enumerate((0, 2, 1, 3)):
                        if 8 + slot % 2 * 40 <= x < 40 + slot % 2 * 40 and 8 + slot // 2 * 24 <= y < 32 + slot // 2 * 24:
                            if popup & (1 << bit):
                                bank = 13
                    value = tile_pixel(raw, y // 8 * 11 + x // 8, x % 8, y % 8)
                    if value:
                        frame.putpixel((152 + x, 8 + y), self.pal[bank][value])
        return frame


def polygon_mask(stats):
    source = (ROOT / "src/menu_specialized.c").read_text()
    values = re.search(r"sConditionToLineLength\[.*?\]\s*=\s*\{(.*?)\};", source, re.S)[1]
    lengths = list(map(int, re.findall(r"\d+", values)))
    sine = [int(float(value) * 256) for value in re.findall(r"Q_8_8\((-?[\d.]+)\)", (ROOT / "src/trig.c").read_text())[:320]]
    # Fixture stats are cool, beauty, cute, smart, tough. Native storage
    # traverses cool, tough, smart, cute, beauty around the five vertices.
    points = [(155, 91 - lengths[stats[0]])] + [(0, 0)] * 4
    angle = 64
    for pos, stat in zip((4, 3, 2, 1), (stats[4], stats[3], stats[2], stats[1])):
        angle = (angle + 51 + (pos == 2)) % 256
        radius = lengths[stat]
        x = 155 + ((radius * sine[64 + angle]) >> 8)
        y = 91 - ((radius * sine[angle]) >> 8)
        if pos <= 2 and (radius != 32 or pos != 2):
            x += 1
        points[pos] = (x, y)
    mask = Image.new("L", (240, 160))
    ImageDraw.Draw(mask).polygon(points, fill=255)
    return mask


def check_polygon(image, stats, name):
    expected = polygon_mask(stats) if stats is not None else Image.new("L", image.size)
    actual = Image.new("L", image.size)
    for y in range(40, 128):
        for x in range(80, 240):
            r, g, b = image.getpixel((x, y))
            if b > r + 50 and b > g + 30:
                actual.putpixel((x, y), 255)
    outside = ImageChops.subtract(actual, expected.filter(ImageFilter.MaxFilter(5)))
    missing = ImageChops.subtract(expected.filter(ImageFilter.MinFilter(5)), actual)
    assert not outside.getbbox(), f"{name}: polygon outside expected data vertices"
    assert not missing.getbbox(), f"{name}: missing condition polygon interior"
    if stats is not None:
        assert actual.getbbox(), f"{name}: no live condition polygon"


def main(base):
    validate()
    ref = Reference()
    pikachu = ("Pikachu", 50, "♀", [255] * 5)
    eevee = ("Eevee", 35, "♀", [0] * 5)
    ditto = ("Ditto", 20, None, [200, 255, 40, 100, 255])
    bulbasaur = ("Bulbasaur", 5, "♀", [180, 180, 255, 255, 120])
    last = ("WWWWWWWWWWWW", 100, "♂", [5] * 5)
    boxed = ("WWWWWWWWWW01", 100, "♂", [160, 0, 120, 200, 80])
    boxed_next = ("WWWWWWWWWW02", 100, None, [155, 10, 125, 195, 85])
    # owner, displayed position/rank, location, saved marks, popup bits, cursor
    states = {}

    def add(names, owner, index, location="IN PARTY", marks=0, popup=None, cursor=None):
        for name in names.split():
            states[name] = (owner, index, location, marks, popup, cursor)

    add("party_first party_a_ignored party_wrap_first party_reentered", pikachu, "1 / 5")
    add("party_zero", eevee, "2 / 5", marks=6)
    add("party_genderless", ditto, "3 / 5", marks=9)
    add("party_fourth", bulbasaur, "4 / 5")
    add("party_last party_reverse_last", last, "5 / 5")
    add("party_cancel party_wrap_cancel single_cancel empty_cancel empty_after_up empty_after_down", None, "CANCEL", "")
    add("single_first single_wrap", pikachu, "1 / 1")
    add("search_first search_first_clamped", pikachu, 1)
    for name, bits, cursor in (("marks_open", 0, 0), ("marks_circle", 1, 0), ("marks_triangle", 5, 1),
                               ("marks_heart", 13, 3), ("marks_all", 15, 2), ("marks_ok", 15, 4)):
        add(name, pikachu, 1, popup=bits, cursor=cursor)
    add("marks_saved_party marks_b_discarded marks_cancel_discarded", pikachu, 1, marks=15)
    add("marks_party_reopened", pikachu, 1, marks=15, popup=15, cursor=0)
    add("marks_party_pending", pikachu, 1, marks=15, popup=14, cursor=0)
    add("marks_cancel", pikachu, 1, marks=15, popup=14, cursor=5)
    add("search_ditto", ditto, 2, marks=9)
    add("search_bulbasaur", bulbasaur, 3)
    add("search_pc", boxed, 4, "WWWWWWWW")
    add("marks_saved_pc search_pc_reloaded search_pc_reentered", boxed, 4, "WWWWWWWW", marks=4)
    add("marks_pc_reopened", boxed, 4, "WWWWWWWW", marks=4, popup=4, cursor=0)
    add("search_pc_next", boxed_next, 5, "WWWWWWWW")
    add("party_saved_marks", pikachu, "1 / 5", marks=15)
    add("rank104 rank104_clamped", ("WWWWWWWWW100", 100, None, [152] * 5), 104, "BOX4")
    add("rank103", ("WWWWWWWWW099", 100, None, [153] * 5), 103, "BOX4")
    images = {}
    cursors = ((165, 28), (197, 28), (165, 52), (197, 52), (164, 80), (164, 112))
    for name, (owner, index, location, marks, popup, cursor) in states.items():
        actual = Image.open(base / f"{name}.png").convert("RGB")
        assert actual.size == (240, 160), name
        images[name] = actual
        expected = ref.frame(owner, index, location, marks, popup)
        bad = checked = 0
        for y in range(8, 128):
            for x in range(240):
                if x < 80 and y >= 40:  # Animated native portrait/sheens.
                    continue
                if popup is None and 120 <= x <= 191 and 54 <= y <= 122:
                    continue  # Independently validated below.
                if cursor is not None:
                    cx, cy = cursors[cursor]
                    if cx - 4 <= x < cx + 4 and cy - 8 <= y < cy + 8:
                        continue
                a, b = actual.getpixel((x, y)), expected.getpixel((x, y))
                bad += max(abs(a[c] - b[c]) for c in range(3)) > 8
                checked += 1
        if bad:
            expected.save(base / f"{name}_expected.png")
            raise SystemExit(f"{name}: {bad}/{checked} source/text pixels differ")
        if popup is None:
            check_polygon(actual, owner[3] if owner else None, name)
        print(f"{name}: {checked} original-art/text pixels and data state verified")

    # Cancel really clears the portrait; repeated input cannot resurrect it.
    for name in ("party_cancel", "single_cancel", "empty_cancel", "empty_after_up", "empty_after_down"):
        crop = (0, 40, 80, 128)
        a = images[name].crop(crop)
        b = ref.background.crop(crop)
        assert all(max(abs(x[c] - y[c]) for c in range(3)) <= 8 for x, y in zip(a.getdata(), b.getdata())), name

    # All ten sheen positions must show the original unscaled 8x8 star.
    star = Image.open(STORAGE / "shiny_star.png")
    palette = png_palette("shiny_star.png")
    shots = [Image.open(base / f"sheen_{i:02d}.png").convert("RGB") for i in range(32)]
    for dx, dy in ((0,-33),(20,-28),(30,-10),(30,10),(20,28),(0,33),(-20,28),(-30,10),(-30,-10),(-20,-28)):
        def matches(shot):
            for yy in range(8):
                for xx in range(8):
                    value = star.getpixel((xx, yy))
                    if value and max(abs(shot.getpixel((36 + dx + xx, 76 + dy + yy))[c] - palette[value][c]) for c in range(3)) > 8:
                        return False
            return True
        assert any(matches(shot) for shot in shots), f"Missing native sheen star at {dx},{dy}"

    # Successful exits must restore real menu/list screens, not stay on details.
    for name in ("party_cancel_return", "party_b_return", "single_return", "empty_return", "search_return_list", "rank103_return"):
        frame = Image.open(base / f"{name}.png").convert("RGB")
        assert ImageChops.difference(frame.crop((0, 8, 240, 128)), images["party_first"].crop((0, 8, 240, 128))).getbbox(), name
    for name in ("marks_saved_party", "marks_b_discarded", "marks_cancel_discarded", "search_pc", "search_pc_reentered"):
        assert not ImageChops.difference(images[name].crop((0, 128, 240, 160)), images["search_first"].crop((0, 128, 240, 160))).getbbox(), f"{name}: help bar not restored"
    print(f"Verified {len(states)} detail/markings states, ten native sheen stars and six return paths")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: verify_hgss_condition_graph_screens.py SCREENSHOT_DIR")
    main(Path(sys.argv[1]))
