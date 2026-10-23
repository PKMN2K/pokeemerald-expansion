#!/usr/bin/env python3
"""Export a conservative HGSS Ribbons layout to GBA 4bpp BG assets.

Reads only authentic HGSS a/1/6/2 NARC bytes. No art is drawn or recolored.
Exports 32x32 GBA screen entries, 4bpp character tiles, and the original
256-color BGR555 palette. The BG6 tile is substituted only if it is fully
opaque; otherwise the BG3 tile remains, avoiding an incorrect flattened
transparent compositing approximation. No runtime wiring is performed.
"""
import argparse
import hashlib
import json
from pathlib import Path
from audit_hgss_ribbon_palette_slots import audit, SOURCE
from render_hgss_ribbon_backgrounds import narc_members, u16
from preview_hgss_ribbon_tile_layout import TRIALS
from audit_hgss_ribbon_gba_layout import REGIONS
from validate_hgss_ribbon_gba_bg import validate

MAX_TILES = {1: 512, 2: 256}
WIDTH, HEIGHT = 30, 20
EXPECTED_NARC_SHA256 = "003c80069fcd1f9c9eb842ae0294ecbdeffabbff40593cc471236e81239d9eed"


def verify_source_narc(data):
    actual = hashlib.sha256(data).hexdigest()
    if actual != EXPECTED_NARC_SHA256:
        raise ValueError(f"HGSS ribbon NARC SHA-256 mismatch: {actual}; expected {EXPECTED_NARC_SHA256}")


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


def parse_palette_map(spec):
    if not spec:
        return {}
    mapping = {}
    for pair in spec.split(","):
        fields = pair.split(":")
        if len(fields) != 2:
            raise ValueError("Expected SRC:DST palette mappings")
        source, target = map(int, fields)
        if not (0 <= source < 16 and 0 <= target < 16):
            raise ValueError("Palette banks must be in range 0..15")
        if source in mapping:
            raise ValueError("Duplicate source bank")
        mapping[source] = target
    if len(set(mapping.values())) != len(mapping):
        raise ValueError("Duplicate destination bank")
    return mapping


def remap_palette(original, mapping, used_banks):
    if set(mapping) != used_banks:
        raise ValueError("Mappings must cover precisely the used source banks")
    owners, _ = audit(SOURCE.read_text())
    collisions = set(mapping.values()) & set(owners)
    if collisions:
        raise ValueError(f"Destinations collide with live BG palette loads: {sorted(collisions)}")
    # Relocate complete original 16-color banks, never recolor retail pixels.
    result = bytearray(512)
    for source, target in mapping.items():
        result[32 * target:32 * (target + 1)] = original[32 * source:32 * (source + 1)]
    return bytes(result)


def export(members, layout, viewport_x, viewport_y, out, palette_map=None):
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
    skipped_bg6_cells = []
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
                    skipped_bg6_cells.append({"x": x, "y": y, "source_x": sx * 8, "source_y": sy * 8})
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
    palette_map = palette_map or {}
    if palette_map:
        palette_bytes = remap_palette(nclr[40:552], palette_map, banks)
        screen = [(value & 0x0fff) | (palette_map[value >> 12] << 12)
                  if index // 32 < HEIGHT and index % 32 < WIDTH else value
                  for index, value in enumerate(screen)]
    else:
        # A 512-byte GBA palette upload changes every BG bank, including
        # banks not referenced by the exported map. Require an explicit,
        # audited mapping rather than treating non-overlapping tile indices
        # as proof that a full palette upload is safe.
        raise ValueError(
            "HGSS BG export requires a complete --palette-map; "
            "unmapped full-palette uploads can overwrite live BG banks"
        )
    tile_bytes = b"".join(graphics)
    screen_bytes = b"".join(value.to_bytes(2, "little") for value in screen)
    # Fail before emitting any files if the export violates GBA structure.
    validate(tile_bytes, palette_bytes, screen_bytes)
    out.mkdir(parents=True, exist_ok=True)
    prefix = out / layout
    (out / (layout + ".4bpp")).write_bytes(tile_bytes)
    (out / (layout + ".gbapal")).write_bytes(palette_bytes)
    # Emit only explicitly allocated banks as standalone 32-byte resources.
    # The 512-byte palette is diagnostic; uploading it wholesale would erase
    # other PokéNav colours, including unreferenced but live palette banks.
    for bank in sorted(set(palette_map.values())):
        (out / f"{layout}.bank_{bank:02d}.gbapal").write_bytes(
            palette_bytes[bank * 32:(bank + 1) * 32]
        )
    (out / (layout + ".bin")).write_bytes(screen_bytes)
    # Machine-readable handoff for the future runtime loader. Each palette
    # bank is a separate BG_PLTT_ID(bank) upload of exactly 32 bytes.
    # This manifest does not certify the bank as free across PokéNav modules.
    manifest = {
        "source_narc_sha256": EXPECTED_NARC_SHA256,
        "layout": layout,
        "viewport": {"x": viewport_x, "y": viewport_y, "width": 240, "height": 160},
        "tile_count": len(graphics),
        "tilemap": layout + ".bin",
        "tiles": layout + ".4bpp",
        "palette_banks": [
            {"source_bank": src, "destination_bank": dst,
             "file": f"{layout}.bank_{dst:02d}.gbapal", "size": 32}
            for src, dst in sorted(palette_map.items())
        ],
        "runtime_verified": False,
        "bg6_opaque_substitutions": substitutions,
        "bg6_transparent_cells_not_composited": skipped_bg6_cells,
        "composition_verified": False,
        "requires": ["cross-module BG palette ownership audit",
                     "retail BG3/BG6 composition verification",
                     "live mGBA screenshot review"],
    }
    (out / (layout + ".manifest.json")).write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n"
    )
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
    p.add_argument("--palette-map", default="", help="Complete SRC:DST bank mapping, e.g. 0:7,2:8,4:9")
    args = p.parse_args()
    archive = args.narc.read_bytes()
    verify_source_narc(archive)
    members = narc_members(archive)
    if len(members) != 79:
        raise ValueError("Expected 79 NARC members")
    export(members, args.layout, args.viewport_x, args.viewport_y, args.out,
           parse_palette_map(args.palette_map))


if __name__ == "__main__":
    main()
