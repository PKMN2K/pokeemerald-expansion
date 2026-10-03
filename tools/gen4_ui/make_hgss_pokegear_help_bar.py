#!/usr/bin/env python3
from pathlib import Path
import struct
import sys

from make_hgss_pokegear_match_call_contact import (
    PALETTE,
    TILEMAP,
    TILES,
    git_blob_sha,
    load_indexed_png4,
    load_nclr,
    load_nscr,
)


# Retail PokegearPhone_PrintContextMenuTooltip copies a 32x4 tile rectangle
# from member-34 source rows 24..27. At the locked source revision that entire
# rectangle is tile 64, palette bank 1, with no flips (NSCR entry 0x1040).
SOURCE_WIDTH_TILES = 32
TOOLTIP_SOURCE_Y = 24
TOOLTIP_HEIGHT_TILES = 4
TOOLTIP_TILE = 64
TOOLTIP_PALETTE_BANK = 1
EXPECTED_TOOLTIP_ENTRY = 0x1040

# The shared PokéNav BG0 header uses palette bank 0. At this validated blob,
# indices 7..10 are unused by the header artwork, so the tooltip can relocate
# its needed indices there while retaining exact retail BGR555 colors.
HEADER = Path("graphics/pokenav/header.png")
EXPECTED_HEADER_GIT_BLOB = "7fe891e22f0fd440d3ee7d41ae0a2b35d008ac5d"
GBA_INDEX_REMAP = {
    1: 7,   # retail tooltip strip
    2: 8,   # retail text shadow
    3: 9,   # retail text foreground
    5: 10,  # retail text background/fill
}


def verify_tooltip_binding():
    width, height, entries = load_nscr(TILEMAP)
    if (width, height) != (256, 296):
        raise ValueError(f"{TILEMAP}: expected 256x296, got {width}x{height}")

    cols = width // 8
    region = []
    for y in range(TOOLTIP_SOURCE_Y, TOOLTIP_SOURCE_Y + TOOLTIP_HEIGHT_TILES):
        region.extend(entries[y * cols:(y + 1) * cols])

    if len(region) != SOURCE_WIDTH_TILES * TOOLTIP_HEIGHT_TILES:
        raise ValueError("unexpected HGSS Phone tooltip rectangle size")

    changed = sorted(set(region))
    if changed != [EXPECTED_TOOLTIP_ENTRY]:
        raise ValueError(
            f"{TILEMAP}: tooltip rows changed: expected only "
            f"0x{EXPECTED_TOOLTIP_ENTRY:04X}, got "
            + ", ".join(f"0x{x:04X}" for x in changed)
        )


def make_tiles():
    verify_tooltip_binding()
    pixels = load_indexed_png4(TILES)

    tx = (TOOLTIP_TILE % 32) * 8
    ty = (TOOLTIP_TILE // 32) * 8
    out = bytearray()
    for py in range(8):
        for px in range(0, 8, 2):
            lo = pixels[ty + py][tx + px]
            hi = pixels[ty + py][tx + px + 1]
            out.append(lo | (hi << 4))

    if len(out) != 32:
        raise ValueError(f"expected one 4bpp tile (32 bytes), got {len(out)}")
    return bytes(out)


def verify_gba_header_palette_slots():
    data = HEADER.read_bytes()
    actual = git_blob_sha(data)
    if actual != EXPECTED_HEADER_GIT_BLOB:
        raise ValueError(
            f"{HEADER}: Git blob {actual} != validated "
            f"{EXPECTED_HEADER_GIT_BLOB}; re-audit free palette slots 7..10"
        )


def make_gba_tiles():
    verify_gba_header_palette_slots()
    source = make_tiles()
    out = bytearray()
    for value in source:
        lo = value & 0x0F
        hi = value >> 4
        if lo not in GBA_INDEX_REMAP or hi not in GBA_INDEX_REMAP:
            raise ValueError("retail tooltip tile now uses an unverified palette index")
        out.append(GBA_INDEX_REMAP[lo] | (GBA_INDEX_REMAP[hi] << 4))
    return bytes(out)


def make_palette():
    verify_tooltip_binding()
    colors = load_nclr(PALETTE)
    start = TOOLTIP_PALETTE_BANK * 16
    bank = colors[start:start + 16]
    if len(bank) != 16:
        raise ValueError(f"{PALETTE}: missing retail palette bank 1")
    return struct.pack("<16H", *bank)


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("--tiles", "--gba-tiles", "--palette"):
        raise SystemExit(
            "usage: make_hgss_pokegear_help_bar.py "
            "(--tiles|--gba-tiles|--palette) OUTPUT"
        )

    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)
    if sys.argv[1] == "--tiles":
        output.write_bytes(make_tiles())
    elif sys.argv[1] == "--gba-tiles":
        output.write_bytes(make_gba_tiles())
    else:
        output.write_bytes(make_palette())


if __name__ == "__main__":
    main()
