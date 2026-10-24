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



def compose_same_bank_tiles(base, overlay, base_attr, overlay_attr):
    """Composite two 8x8 4bpp tiles without changing palette-index colours.

    Both DS screen entries must select the same 16-colour palette bank.
    Resolve DS horizontal/vertical tile flips into pixel order; the output
    tile is unflipped and uses the original bank without recolouring.
    """
    if base_attr >> 12 != overlay_attr >> 12:
        raise ValueError("Cannot flatten BG layers with different palette banks")
    if len(base) != 32 or len(overlay) != 32:
        raise ValueError("Expected two 8x8 4bpp tiles")

    def pixels(tile_bytes, attr):
        values = [(byte >> shift) & 15 for byte in tile_bytes for shift in (0, 4)]
        return [values[(7 - y if attr & 0x800 else y) * 8 +
                       (7 - x if attr & 0x400 else x)]
                for y in range(8) for x in range(8)]

    bottom = pixels(base, base_attr)
    top = pixels(overlay, overlay_attr)
    result = [upper if upper else lower for lower, upper in zip(bottom, top)]
    return bytes(result[i] | (result[i + 1] << 4) for i in range(0, 64, 2))



def compose_matching_palette_tiles(base, overlay, base_attr, overlay_attr, palette):
    """Losslessly flatten cross-bank overlay only if all colours exist in BG3.

    Translate original colour indices, never RGB values. Palette entry zero is
    transparent on BG6; only nonzero destination entries represent ink.
    Return None if the authentic BG3 bank cannot represent an overlay colour.
    """
    base_bank, overlay_bank = base_attr >> 12, overlay_attr >> 12
    base_colors = [palette[base_bank * 32 + i * 2:
                           base_bank * 32 + i * 2 + 2] for i in range(16)]
    source_colors = [palette[overlay_bank * 32 + i * 2:
                             overlay_bank * 32 + i * 2 + 2] for i in range(16)]
    pixels = [(byte >> shift) & 15 for byte in overlay for shift in (0, 4)]
    translation = {0: 0}
    for index in set(pixels) - {0}:
        matching = next((i for i in range(1, 16)
                         if base_colors[i] == source_colors[index]), None)
        if matching is None:
            return None
        translation[index] = matching
    translated = [translation[index] for index in pixels]
    packed = bytes(translated[i] | translated[i + 1] << 4
                   for i in range(0, 64, 2))
    return compose_same_bank_tiles(
        base, packed, base_attr, (overlay_attr & 0x0fff) | (base_attr & 0xf000)
    )



def compose_in_original_palette(base, overlay, base_attr, overlay_attr, palette):
    """Find any retail HGSS palette bank that represents the exact final pixels.

    Prefer the original BG3 bank, then BG6, then the remaining retail banks.
    All flips are baked into the returned 4bpp tile. No new colours are made.
    Return (tile_bytes, original_bank), or None if no single bank fits.
    """
    if len(palette) != 512 or len(base) != 32 or len(overlay) != 32:
        raise ValueError("Expected original NCLR palette and 8x8 tiles")

    def unpack(raw, attr):
        values = [(v >> shift) & 15 for v in raw for shift in (0, 4)]
        return [values[(7 - y if attr & 0x800 else y) * 8
                       + (7 - x if attr & 0x400 else x)]
                for y in range(8) for x in range(8)]

    bottom = unpack(base, base_attr)
    top = unpack(overlay, overlay_attr)
    base_bank, overlay_bank = base_attr >> 12, overlay_attr >> 12
    pixels = [(overlay_bank, a) if a else (base_bank, b)
              for b, a in zip(bottom, top)]
    colors = [None if index == 0 else
              palette[bank * 32 + index * 2:bank * 32 + index * 2 + 2]
              for bank, index in pixels]
    candidates = dict.fromkeys((base_bank, overlay_bank, *range(16)))
    for bank in candidates:
        entries = [palette[bank * 32 + i * 2:bank * 32 + i * 2 + 2]
                   for i in range(16)]
        indices = {color: next((i for i in range(1, 16)
                                if entries[i] == color), None)
                   for color in set(colors) if color is not None}
        if any(index is None for index in indices.values()):
            continue
        flattened = [0 if color is None else indices[color] for color in colors]
        result = bytes(flattened[i] | (flattened[i + 1] << 4)
                       for i in range(0, 64, 2))
        return result, bank
    return None


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


def export(members, layout, viewport_x, viewport_y, out, palette_map=None, audit_only=False):
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
    empty_bg6_cells = 0
    skipped_bg6_cells = []
    composed_transparent = 0
    composed_cross_bank = 0
    substitutions = 0
    banks = set()
    for ty in range(HEIGHT):
        for tx in range(WIDTH):
            x, y = tx * 8, ty * 8
            sx, sy = tx + viewport_x // 8, ty + viewport_y // 8
            base_attr = entry(members[10], sx, sy)
            attr = base_attr
            raw_tile = tile(members[2], base_attr & 0x3ff, MAX_TILES[2])
            key = (2, base_attr & 0x3ff)
            if selected(layout, x, y) and not protected(tx, ty):
                candidate = entry(members[9], sx, sy)
                src_tile = tile(members[1], candidate & 0x3ff, MAX_TILES[1])
                if opaque(src_tile):
                    attr = candidate
                    raw_tile = src_tile
                    key = (1, candidate & 0x3ff)
                    substitutions += 1
                elif not any(src_tile):
                    # Fully transparent BG6 contributes no visible pixels,
                    # irrespective of the two palette-bank identifiers.
                    empty_bg6_cells += 1
                elif (candidate >> 12) == (base_attr >> 12):
                    raw_tile = compose_same_bank_tiles(raw_tile, src_tile, base_attr, candidate)
                    # The pixel order already incorporates both DS flip flags.
                    attr = base_attr & ~0x0c00
                    key = ("composite", raw_tile)
                    composed_transparent += 1
                else:
                    composite = compose_in_original_palette(
                        raw_tile, src_tile, base_attr, candidate, nclr[40:552]
                    )
                    if composite is not None:
                        raw_tile, output_bank = composite
                        attr = (output_bank << 12) | (base_attr & 0x03ff)
                        key = ("composite", raw_tile)
                        composed_cross_bank += 1
                    else:
                        kept_transparent += 1
                        visible_pixels = sum((byte & 15 != 0) + (byte >> 4 != 0) for byte in src_tile)
                        skipped_bg6_cells.append({
                            "x": x, "y": y, "source_x": sx * 8, "source_y": sy * 8,
                            "bg3_bank": base_attr >> 12, "bg6_bank": candidate >> 12,
                            "bg6_visible_pixels": visible_pixels,
                        })
            if key not in cache:
                cache[key] = len(graphics)
                graphics.append(raw_tile)
            dest_tile = cache[key]
            if dest_tile > 1023:
                raise ValueError("GBA BG tile index exceeds 10-bit capacity")
            bank = attr >> 12
            banks.add(bank)
            screen[ty * 32 + tx] = dest_tile | (attr & 0xfc00)
    # An audit of the retail source does not require guessing which GBA
    # palette banks are free. Preserve exact diagnostics even when no
    # collision-safe runtime mapping has yet been established.
    if audit_only:
        unresolved_pairs = {}
        for cell in skipped_bg6_cells:
            pair = (cell["bg3_bank"], cell["bg6_bank"])
            item = unresolved_pairs.setdefault(pair, {"cells": 0, "visible_pixels": 0})
            item["cells"] += 1
            item["visible_pixels"] += cell["bg6_visible_pixels"]
        report = {
            "source_narc_sha256": EXPECTED_NARC_SHA256,
            "layout": layout,
            "viewport": {"x": viewport_x, "y": viewport_y, "width": 240, "height": 160},
            "source_palette_banks_needed": sorted(banks),
            "bg6_opaque_substitutions": substitutions,
            "bg6_same_bank_composited_cells": composed_transparent,
            "bg6_cross_bank_exact_color_composited_cells": composed_cross_bank,
            "bg6_fully_transparent_cells_ignored": empty_bg6_cells,
            "bg6_transparent_cells_not_composited": skipped_bg6_cells,
            "bg6_unresolved_palette_bank_pairs": [
                {"bg3_bank": bg3, "bg6_bank": bg6, **values}
                for (bg3, bg6), values in sorted(unresolved_pairs.items())
            ],
            "composition_verified": False,
            "runtime_verified": False,
        }
        out.mkdir(parents=True, exist_ok=True)
        target = out / (layout + ".audit.json")
        target.write_text(json.dumps(report, indent=2, sort_keys=True) + "\\n")
        print(f"{target}: {kept_transparent} unresolved BG6 cells; review only")
        return report
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
    # Group genuine unrepresentable overlays by their original source banks.
    # This reports where a second hardware BG or palette allocation is still
    # needed without pretending the provisional flattened export is complete.
    unresolved_pairs = {}
    for cell in skipped_bg6_cells:
        pair = (cell["bg3_bank"], cell["bg6_bank"])
        report = unresolved_pairs.setdefault(pair, {"cells": 0, "visible_pixels": 0})
        report["cells"] += 1
        report["visible_pixels"] += cell["bg6_visible_pixels"]
    unresolved_summary = [
        {"bg3_bank": bg3, "bg6_bank": bg6, **unresolved_pairs[(bg3, bg6)]}
        for bg3, bg6 in sorted(unresolved_pairs)
    ]
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
        "bg6_same_bank_composited_cells": composed_transparent,
        "bg6_cross_bank_exact_color_composited_cells": composed_cross_bank,
        "bg6_fully_transparent_cells_ignored": empty_bg6_cells,
        "bg6_transparent_cells_not_composited": skipped_bg6_cells,
        "bg6_unresolved_palette_bank_pairs": unresolved_summary,
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
          f"{composed_transparent} same-bank composites, "
          f"{composed_cross_bank} exact-colour cross-bank composites, "
          f"{empty_bg6_cells} empty BG6 tiles, "
          f"{kept_transparent} skipped cross-bank BG6 tiles")
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
    p.add_argument("--audit-only", action="store_true", help="Inspect original composition without assigning GBA palette banks")
    args = p.parse_args()
    archive = args.narc.read_bytes()
    verify_source_narc(archive)
    members = narc_members(archive)
    if len(members) != 79:
        raise ValueError("Expected 79 NARC members")
    export(members, args.layout, args.viewport_x, args.viewport_y, args.out,
           parse_palette_map(args.palette_map), audit_only=args.audit_only)


if __name__ == "__main__":
    main()
