#!/usr/bin/env python3
"""Export a conservative HGSS Ribbons layout to GBA 4bpp BG assets.

Reads only authentic HGSS a/1/6/2 NARC bytes. No art is drawn or recolored.
Exports 32x32 GBA screen entries, 4bpp character tiles, and the original
256-color BGR555 palette. The BG6 tile is substituted only if it is fully
opaque; otherwise the BG3 tile remains, avoiding an incorrect flattened
transparent compositing approximation. No runtime wiring is performed.
"""
import argparse
from pathlib import Path
from render_hgss_ribbon_backgrounds import narc_members, u16
from preview_hgss_ribbon_tile_layout import TRIALS
from audit_hgss_ribbon_gba_layout import REGIONS

MAX_TILES = {1: 512, 2: 256}
WIDTH, HEIGHT = 30, 20


def entry(nscr, x, y):
    if nscr[:4] != b"RCSN" or nscr[16:20] != b"NRCS" or len(nscr) != 2084:
        raise ValueError("Expected original 32x32 NSCR")
    return u16(nscr, 36 + 2 * (y * 32 + x))


def tile(ncgr, tile_id, max_tiles):
    if ncgr[:4] != b"RGCN" or ncgr[16:20] != b"RAHC":
        raise ValueError("Expected original NCGR")
    data = ncgr[64:]
    if len(data) != max_tiles * 32 or tile_id >= max_tiles:
        raise ValueError("NCGR tile out of range")
    return data[tile_id * 32:(tile_id + 1) * 32]


def opaque(tile_bytes):
    return all((v & 15) != 0 and (v >> 4) != 0 for v in tile_bytes)


def protected(tx, ty):
    x, y = tx * 8, ty * 8
    return any(x < r.right and x + 8 > r.x and
               y < r.bottom and y + 8 > r.y
               for r in REGIONS if "frame" not in r.name)


def selected(name, x, y):
    return any(rx <= x < rx + rw and ry <= y < ry + rh
               for rx, ry, rw, rh in TRIALS[name])


def export(members, layout, viewport_x, viewport_y, out):
    if any(v % 8 for v in (viewport_x, viewport_y)):
        raise ValueError("Tilemap export requires 8-pixel-aligned viewport")
    if not (0 <= viewport_x <= 16 and 0 <= viewport_y <= 96):
        raise ValueError("GBA viewport outside 256x256 original maps")
    nclr = members[0]
    if nclr[:4] != b"RLCN" or nclr[16:20] != b"TTLP" or len(nclr[40:552]) != 512:
        raise ValueError("Expected original 16-bank NCLR palette")
    # Reserve tile 0 as blank. Each unique original source tile stays
    # identical (including its 4bpp pixel indexes); flips stay in the map.
    graphics = [bytes(32)]
    cache = {}
    screen = [0] * 1024
    kept_transparent = 0
    substitutions = 0
    banks = set()
    for ty in range(HEIGHT):
        for tx in range(WIDTH):
            x, y = tx * 8, ty * 8
            sx, sy = tx + viewport_x // 8, ty + viewport_y // 8
            char_id, screen_id = 2, 10
            if selected(layout, x, y) and not protected(tx, ty):
                candidate = entry(members[9], sx, sy)
                src_tile = tile(members[1], candidate & 0x3ff, MAX_TILES[1])
                if opaque(src_tile):
                    char_id, screen_id = 1, 9
                    substitutions += 1
                else:
                    kept_transparent += 1
            attr = entry(members[screen_id], sx, sy)
            index = attr & 0x3ff
            raw_tile = tile(members[char_id], index, MAX_TILES[char_id])
            key = (char_id, index)
            if key not in cache:
                cache[key] = len(graphics)
                graphics.append(raw_tile)
            dest_tile = cache[key]
            if dest_tile > 1023:
                raise ValueError("GBA BG tile index exceeds 10-bit capacity")
            bank = attr >> 12
            banks.add(bank)
            screen[ty * 32 + tx] = dest_tile | (attr & 0xfc00)
    out.mkdir(parents=True, exist_ok=True)
    prefix = out / layout
    (out / (layout + ".4bpp")).write_bytes(b"".join(graphics))
    (out / (layout + ".gbapal")).write_bytes(nclr[40:552])
    (out / (layout + ".bin")).write_bytes(
        b"".join(value.to_bytes(2, "little") for value in screen))
    print(f"{prefix}: {len(graphics)} GBA tiles, banks {sorted(banks)}, "
          f"{substitutions} opaque BG6 substitutions, "
          f"{kept_transparent} skipped transparent BG6 tiles")
    print("REVIEW ONLY: 32x32 map padded outside 240x160; palette-bank VRAM "
          "conflicts, DS scroll/priority, actual emulator display unverified.")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("narc", type=Path)
    p.add_argument("out", type=Path)
    p.add_argument("--layout", choices=tuple(TRIALS), default="header_only")
    p.add_argument("--viewport-x", type=int, default=0)
    p.add_argument("--viewport-y", type=int, default=0)
    args = p.parse_args()
    members = narc_members(args.narc.read_bytes())
    if len(members) != 79:
        raise ValueError("Expected 79 NARC members")
    export(members, args.layout, args.viewport_x, args.viewport_y, args.out)


if __name__ == "__main__":
    main()
