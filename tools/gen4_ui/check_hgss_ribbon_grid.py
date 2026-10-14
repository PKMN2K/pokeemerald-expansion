#!/usr/bin/env python3
"""Validate the live 32x32 HGSS ribbon grid and render offline pixel-exact previews.

This does NOT run the game or simulate its PokeNav interface. Previews contain
unaltered 4bpp ribbon pixels with their retail NCLR bank colors, without
scaling, substitute graphics, or approximated screen backgrounds.
"""
import argparse
import csv
import re
import struct
import zlib
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "graphics/gen4_ui/hgss_ribbons"
SOURCE = ROOT / "src/pokenav_ribbons_summary.c"

# Frame order is the same for BG1 and the zoom OBJ sprite.
FRAME_MEMBERS = (72, 73, 74, 75, 76, 78, 77, 79, 80, 81, 82, 83)
FRAME_NAMES = (
    "CHAMPION", "CONTEST_NORMAL", "CONTEST_SUPER", "CONTEST_HYPER",
    "CONTEST_MASTER", "WINNING", "VICTORY", "ARTIST", "EFFORT",
    "GIFT_1", "GIFT_2", "GIFT_3",
)
GRID_COLUMNS = 4
GRID_ROWS = 2
PAGE_SIZE = GRID_COLUMNS * GRID_ROWS
NORMAL_CAPACITY = 25
GIFT_CAPACITY = 7
GIFT_START = 28
ICON_SIZE = 32


def parse_live_mapping():
    source = SOURCE.read_text()
    expected = (
        ("RIBBONS_PER_ROW", "4"),
        ("RIBBON_GRID_ROWS", "2"),
        ("RIBBON_ICON_TILES", "4"),
        ("RIBBON_GRID_LEFT_TILE", "11"),
        ("RIBBON_GRID_TOP_TILE", "4"),
    )
    for name, value in expected:
        assert re.search(r"^#define " + name + r" " + value + r"$", source, re.M), name
    assert "#define RIBBONS_PER_PAGE (RIBBONS_PER_ROW * RIBBON_GRID_ROWS)" in source
    assert "LoadBgTiles(1, sRibbonIconsBig_Gfx, sizeof(sRibbonIconsBig_Gfx), 1)" in source
    assert "CopyToBgTilemapBufferRect(1, bgData, destX, destY, RIBBON_ICON_TILES, RIBBON_ICON_TILES)" in source
    assert "DrawAllRibbonsHgss(menu);" in source
    assert "sRibbonGfxData[ribbonId].tileNumOffset * 16" in source
    assert "for (i = 0; i < RIBBON_ICON_TILES * RIBBON_ICON_TILES; i++)" in source
    assert "list->selectedPos - GIFT_RIBBON_START_POS >= RIBBONS_PER_ROW" in source
    # BG1 icons, selection corners and the zoomed OBJ must agree on
    # the selected slot inside the 4x2 page (including pages 2-5).
    origin_x = "s32 x = (RIBBON_GRID_LEFT_TILE + RIBBON_ICON_TILES * (pageSlot % RIBBONS_PER_ROW)) * 8 + 16;"
    origin_y = "s32 y = (RIBBON_GRID_TOP_TILE + RIBBON_ICON_TILES * (pageSlot / RIBBONS_PER_ROW)) * 8 + 16;"
    assert source.count("u32 pageSlot = position % RIBBONS_PER_PAGE;") == 2
    assert source.count(origin_x) == 2
    assert source.count(origin_y) == 2
    assert "corner->x = x + ((i & 2) ? 16 : -16);" in source
    assert "corner->y = y + ((i & 1) ? 16 : -16);" in source
    assert ".shape = SPRITE_SHAPE(32x32)," in source
    assert ".size = SPRITE_SIZE(32x32)," in source
    assert "sRibbonIconsSmall_Gfx" not in source
    assert "BufferSmallRibbonGfxData" not in source
    for bank in range(5):
        assert "CopyPaletteIntoBufferUnfaded(sHgssRibbonBigPal" + str(bank) in source
    matches = re.findall(
        r"^\s*\[(\w+)\]\s*=\s*\{\s*RIBBONGFX_(\w+),\s*"
        r"TO_PAL_OFFSET\(PALTAG_RIBBON_ICONS_(\d)\)\s*\}", source, re.M
    )
    assert len(matches) == 32, ("live ribbon table count", len(matches))
    mapping = {symbol: (FRAME_MEMBERS[FRAME_NAMES.index(frame)], int(pal) - 1)
               for symbol, frame, pal in matches}
    assert len(mapping) == 32
    with (ASSETS / "emerald_ribbon_compat.csv").open(newline="") as handle:
        rows = list(csv.DictReader(line for line in handle if not line.startswith("#")))
    assert len(rows) == 32
    for row in rows:
        member, bank = mapping[row["emerald_symbol"]]
        assert member == int(row["ncgr_member"]), row
        assert bank == int(row["nclr_bank"]), row
    # Check the mGBA-accessible debug preview mirrors the same 32 entries.
    debug_screen = (ROOT / "src/gen4_ui_test.c").read_text()
    assert "void CB2_HgssRibbonGridTest(void)" in debug_screen
    assert "LoadBgTiles(0, sHgssRibbonPreviewTiles" in debug_screen
    assert "HGSS_RIBBON_PREVIEW_PAGE_COUNT 5" in debug_screen
    # A permanent 2 KB .bss map consumes scarce IWRAM even in headless
    # battle tests which never open the debug viewer. Guard this repair.
    assert "static u16 *sHgssRibbonPreviewMap;" in debug_screen
    assert "sHgssRibbonPreviewMap = AllocZeroed(BG_SCREEN_SIZE);" in debug_screen
    assert "Free(sHgssRibbonPreviewMap);" in debug_screen
    assert "static u16 sHgssRibbonPreviewMap[" not in debug_screen
    icon_table = debug_screen.split("static const struct HgssRibbonPreviewIcon sHgssRibbonPreviewIcons[] =", 1)[1].split("};", 1)[0]
    preview_entries = re.findall(r"\{(\d+),\s*(\d+)\}", icon_table)
    assert len(preview_entries) == 32, len(preview_entries)
    for row in rows:
        rid = int(row["emerald_ribbon_id"])
        frame, bank = (int(x) for x in preview_entries[rid])
        assert FRAME_MEMBERS[frame] == int(row["ncgr_member"]), row
        assert bank == int(row["nclr_bank"]), row
    assert (1 + (NORMAL_CAPACITY // GRID_COLUMNS)) * GRID_COLUMNS == GIFT_START
    # The 0x1800 tiles + blank tile fit BG1's 16 KB character block.
    assert (12 * 16 + 1) * 32 <= 0x4000
    return rows


def adjacent(pos, normal_count, gift_count):
    """Model C DPAD targets for exhaustive page/navigation geometry checks."""
    normal_last_row = ((normal_count - 1) // GRID_COLUMNS) * GRID_COLUMNS if normal_count else 0
    results = []
    if pos < NORMAL_CAPACITY:
        if pos >= GRID_COLUMNS:
            results.append(pos - GRID_COLUMNS)
    elif pos >= GIFT_START:
        if pos - GIFT_START >= GRID_COLUMNS:
            results.append(pos - GRID_COLUMNS)
        elif normal_count:
            results.append(min(pos - GIFT_START + normal_last_row, normal_count - 1))
    if pos >= GIFT_START:
        if pos - GIFT_START + GRID_COLUMNS < gift_count:
            results.append(pos + GRID_COLUMNS)
    elif pos < normal_last_row:
        results.append(min(pos + GRID_COLUMNS, normal_count - 1))
    elif gift_count:
        results.append(GIFT_START + min(pos - normal_last_row, gift_count - 1))
    if pos % GRID_COLUMNS:
        results.append(pos - 1)
    if pos % GRID_COLUMNS < GRID_COLUMNS - 1:
        if pos < GIFT_START and pos < normal_count - 1:
            results.append(pos + 1)
        if pos >= GIFT_START and pos - GIFT_START + 1 < gift_count:
            results.append(pos + 1)
    return results


def check_navigation():
    checked = 0
    for normal_count in range(NORMAL_CAPACITY + 1):
        for gift_count in range(GIFT_CAPACITY + 1):
            if not normal_count + gift_count:
                continue
            slots = set(range(normal_count)) | set(range(GIFT_START, GIFT_START + gift_count))
            start = 0 if normal_count else GIFT_START
            seen, pending = {start}, deque([start])
            while pending:
                current = pending.popleft()
                page_start = current // PAGE_SIZE * PAGE_SIZE
                for pos in slots:
                    if page_start <= pos < page_start + PAGE_SIZE:
                        slot = pos - page_start
                        x = 88 + (slot % GRID_COLUMNS) * ICON_SIZE
                        y = 32 + (slot // GRID_COLUMNS) * ICON_SIZE
                        assert 88 <= x and x + ICON_SIZE <= 216
                        assert 32 <= y and y + ICON_SIZE <= 96
                for nxt in adjacent(current, normal_count, gift_count):
                    assert nxt in slots, (normal_count, gift_count, current, nxt)
                    if nxt not in seen:
                        seen.add(nxt)
                        pending.append(nxt)
            assert seen == slots, (normal_count, gift_count, slots - seen)
            checked += 1
    return checked


def png_chunk(tag, payload):
    return struct.pack(">I", len(payload)) + tag + payload + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)


def encode_png_rgba(width, height, raw):
    rows = b"".join(b"\0" + raw[y * width * 4:(y + 1) * width * 4] for y in range(height))
    return (b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
            + png_chunk(b"IDAT", zlib.compress(rows, 9)) + png_chunk(b"IEND", b""))


def icon_pixels(tiles, palette):
    assert len(tiles) == 512 and len(palette) == 32
    colors = []
    for i in range(16):
        val = int.from_bytes(palette[i * 2:i * 2 + 2], "little")
        colors.append(((val & 31) * 255 // 31, ((val >> 5) & 31) * 255 // 31,
                       ((val >> 10) & 31) * 255 // 31, 0 if i == 0 else 255))
    pixels = bytearray(ICON_SIZE * ICON_SIZE * 4)
    for y in range(ICON_SIZE):
        for x in range(ICON_SIZE):
            tile = (y // 8) * 4 + x // 8
            b = tiles[tile * 32 + (y % 8) * 4 + (x % 8) // 2]
            color = colors[(b >> (4 * (x % 2))) & 15]
            idx = (y * ICON_SIZE + x) * 4
            pixels[idx:idx + 4] = bytes(color)
    return pixels


def write_previews(rows, outdir):
    outdir.mkdir(parents=True, exist_ok=True)
    sheet = (ASSETS / "big_icons_hgss.4bpp").read_bytes()
    members = {member: sheet[i * 512:(i + 1) * 512]
               for i, member in enumerate(FRAME_MEMBERS)}
    assert len(members) == 12 and all(len(raw) == 512 for raw in members.values())
    palettes = [(ASSETS / f"palette_{i}.gbapal").read_bytes() for i in range(5)]
    positions = {i: r for i, r in enumerate(rows[:NORMAL_CAPACITY])}
    positions.update({GIFT_START + i: r for i, r in enumerate(rows[NORMAL_CAPACITY:])})
    notes = ["OFFLINE sprite-only proof; NOT an in-game screenshot.", ""]
    for page in range(5):
        img = bytearray(128 * 64 * 4)
        notes.append(f"Page {page + 1} — slots {page * PAGE_SIZE}..{page * PAGE_SIZE + 7}:")
        for pos in range(page * PAGE_SIZE, (page + 1) * PAGE_SIZE):
            row = positions.get(pos)
            if row is None:
                continue
            slot = pos % PAGE_SIZE
            sx, sy = (slot % GRID_COLUMNS) * ICON_SIZE, (slot // GRID_COLUMNS) * ICON_SIZE
            icon = icon_pixels(members[int(row["ncgr_member"])], palettes[int(row["nclr_bank"])])
            for y in range(ICON_SIZE):
                dst = ((sy + y) * 128 + sx) * 4
                img[dst:dst + ICON_SIZE * 4] = icon[y * ICON_SIZE * 4:(y + 1) * ICON_SIZE * 4]
            notes.append(f"  slot {pos}: {row['emerald_symbol']}")
        (outdir / f"page_{page + 1:02}.png").write_bytes(encode_png_rgba(128, 64, img))
    (outdir / "README.txt").write_text("\n".join(notes) + "\n")
    print("Wrote five unscaled pixel-exact offline ribbon grid previews:", outdir)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview-dir", type=Path)
    args = parser.parse_args()
    rows = parse_live_mapping()
    cases = check_navigation()
    print(f"Verified 32 live ribbon member/palette combinations and {cases} nonempty ribbon layouts")
    if args.preview_dir:
        write_previews(rows, args.preview_dir)


if __name__ == "__main__":
    main()
