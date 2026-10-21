#!/usr/bin/env python3
"""Reconstruct authentic HGSS Pokémon Summary Ribbons BG tilemaps.

Input: original NARC a/1/6/2 extracted from a legitimately obtained HGSS ROM.
No source artwork is embedded, repainted, stretched, or recolored.
Outputs four independently rendered 256x256 PNG tilemaps for visual review.
Requires Pillow. This does not alter live Emerald UI resources.
"""
import argparse
from pathlib import Path

PAIRS = {"bg3_10": (2, 10), "bg3_11": (2, 11),
         "bg6_9": (1, 9), "bg6_78": (1, 78)}
MAX_TILE = {1: 512, 2: 256}


def u16(data, offset):
    return int.from_bytes(data[offset:offset + 2], "little")


def u32(data, offset):
    return int.from_bytes(data[offset:offset + 4], "little")


def narc_members(data):
    if data[:4] != b"NARC":
        raise ValueError("Expected original NARC archive")
    chunks = {}
    pos = u16(data, 12)
    while pos + 8 <= len(data):
        tag = data[pos:pos + 4]
        length = u32(data, pos + 4)
        if length < 8 or pos + length > len(data):
            raise ValueError("Damaged NARC chunk")
        chunks[tag] = (pos, length)
        pos += length
    if b"BTAF" not in chunks or b"GMIF" not in chunks:
        raise ValueError("NARC missing file index or contents")
    fat, _ = chunks[b"BTAF"]
    gmif, _ = chunks[b"GMIF"]
    count = u16(data, fat + 8)
    start = gmif + 8
    members = []
    for i in range(count):
        a = u32(data, fat + 12 + 8 * i)
        b = u32(data, fat + 16 + 8 * i)
        if a > b or start + b > len(data):
            raise ValueError("Out-of-bounds NARC member")
        members.append(data[start + a:start + b])
    return members


def palette_banks(nclr):
    if nclr[:4] != b"RLCN" or nclr[16:20] != b"TTLP":
        raise ValueError("Expected original NCLR palette")
    # Standard NCLR palette data begins at absolute offset 40.
    raw = nclr[40:552]
    if len(raw) != 512:
        raise ValueError("Expected sixteen 16-colour palette banks")
    result = []
    for bank in range(16):
        colors = []
        for i in range(16):
            v = u16(raw, (bank * 16 + i) * 2)
            colors.append(((v & 31) * 255 // 31,
                           ((v >> 5) & 31) * 255 // 31,
                           ((v >> 10) & 31) * 255 // 31, 255))
        result.append(colors)
    return result


def render(ncgr, nscr, palettes, max_tiles):
    from PIL import Image
    if ncgr[:4] != b"RGCN" or ncgr[16:20] != b"RAHC":
        raise ValueError("Expected original NCGR")
    if nscr[:4] != b"RCSN" or nscr[16:20] != b"NRCS":
        raise ValueError("Expected original NSCR")
    if len(nscr) != 2084:
        raise ValueError("Expected 32x32 tilemap")
    tiles = ncgr[64:]
    if len(tiles) != max_tiles * 32:
        raise ValueError("Unexpected NCGR tile count")
    out = Image.new("RGBA", (256, 256))
    pix = out.load()
    for index in range(1024):
        attr = u16(nscr, 36 + index * 2)
        tile_id = attr & 0x3FF
        hflip = bool(attr & 0x400)
        vflip = bool(attr & 0x800)
        bank = attr >> 12
        if tile_id >= max_tiles:
            raise ValueError(f"Tile {tile_id} exceeds {max_tiles} tiles")
        for py in range(8):
            for px in range(8):
                sx = 7 - px if hflip else px
                sy = 7 - py if vflip else py
                packed = tiles[tile_id * 32 + sy * 4 + sx // 2]
                color = packed >> 4 if sx & 1 else packed & 0xF
                rgba = palettes[bank][color]
                # Treat each bank's color zero as transparent; previews
                # are individual BG layers, not final DS compositions.
                if color:
                    pix[(index % 32) * 8 + px, (index // 32) * 8 + py] = rgba
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("narc", type=Path, help="Original a/1/6/2 NARC bytes")
    parser.add_argument("out", type=Path, help="Preview directory")
    parser.add_argument("--viewport-x", type=int, default=0, help="Review crop left, 0..16")
    parser.add_argument("--viewport-y", type=int, default=0, help="Review crop top, 0..96")
    args = parser.parse_args()
    if not (0 <= args.viewport_x <= 16 and 0 <= args.viewport_y <= 96):
        parser.error("240x160 viewport must fit the original 256x256 tilemap")
    members = narc_members(args.narc.read_bytes())
    if len(members) != 79:
        raise ValueError(f"Expected 79 archive members, got {len(members)}")
    palettes = palette_banks(members[0])
    args.out.mkdir(parents=True, exist_ok=True)
    for label, (char_id, screen_id) in PAIRS.items():
        image = render(members[char_id], members[screen_id],
                       palettes, MAX_TILE[char_id])
        destination = args.out / (label + ".png")
        image.save(destination)
        # Separate individual-layer crops for GBA layout review, not a
        # claim about original DS compositing or approved Emerald positions.
        image.crop((args.viewport_x, args.viewport_y,
                    args.viewport_x + 240, args.viewport_y + 160)).save(
                        args.out / (label + "_gba_review.png"))
        print(f"{destination}: authentic NCGR {char_id} / NSCR {screen_id}")


if __name__ == "__main__":
    main()
