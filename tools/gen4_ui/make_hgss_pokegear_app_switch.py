#!/usr/bin/env python3
from pathlib import Path
import struct
import sys

from PIL import Image


TILES = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_tiles.png")
PALETTE = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_palette.NCLR")
TILEMAP = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_tilemap.NSCR")


def bgr555_to_rgb(value):
    r5 = value & 0x1F
    g5 = (value >> 5) & 0x1F
    b5 = (value >> 10) & 0x1F
    return (
        (r5 << 3) | (r5 >> 2),
        (g5 << 3) | (g5 >> 2),
        (b5 << 3) | (b5 >> 2),
    )


def load_nclr(path):
    data = path.read_bytes()
    if data[:4] != b"RLCN":
        raise ValueError(f"{path}: not an NCLR")

    chunk = data.find(b"TTLP")
    if chunk < 0:
        raise ValueError(f"{path}: missing PLTT chunk")

    data_size = struct.unpack_from("<I", data, chunk + 0x10)[0]
    data_offset = struct.unpack_from("<I", data, chunk + 0x14)[0]
    start = chunk + 8 + data_offset
    raw = data[start:start + data_size]
    if len(raw) != data_size or data_size % 2:
        raise ValueError(f"{path}: malformed palette payload")

    return [
        struct.unpack_from("<H", raw, i)[0]
        for i in range(0, len(raw), 2)
    ]


def load_nscr(path):
    data = path.read_bytes()
    if data[:4] != b"RCSN":
        raise ValueError(f"{path}: not an NSCR")

    chunk = data.find(b"NRCS")
    if chunk < 0:
        raise ValueError(f"{path}: missing SCRN chunk")

    width = struct.unpack_from("<H", data, chunk + 8)[0]
    height = struct.unpack_from("<H", data, chunk + 10)[0]
    payload_size = struct.unpack_from("<I", data, chunk + 16)[0]
    start = chunk + 20
    raw = data[start:start + payload_size]

    expected = (width // 8) * (height // 8) * 2
    if len(raw) != expected:
        raise ValueError(
            f"{path}: expected {expected} tilemap bytes, got {len(raw)}"
        )

    entries = struct.unpack("<" + "H" * (len(raw) // 2), raw)
    return width, height, entries


def main():
    if len(sys.argv) != 2:
        raise SystemExit(
            "usage: make_hgss_pokegear_app_switch.py OUTPUT.png"
        )

    out_path = Path(sys.argv[1])
    source = Image.open(TILES)
    if source.mode != "P" or source.size != (256, 32):
        raise ValueError(
            f"{TILES}: expected indexed 256x32 source, "
            f"got {source.mode} {source.size}"
        )

    palettes = load_nclr(PALETTE)
    width, height, entries = load_nscr(TILEMAP)
    if (width, height) != (256, 96):
        raise ValueError(
            f"{TILEMAP}: expected 256x96, got {width}x{height}"
        )

    source_pixels = source.load()
    rgb = Image.new("RGB", (256, 32))
    pixels = rgb.load()

    # Member 54 stores three 32-pixel-high button states. Rows 0..3 are the
    # retail normal app-switch strip. Reconstruct those rows through the NSCR
    # so tile IDs, flips, and per-tile palette banks all remain authentic.
    for tile_y in range(4):
        for tile_x in range(32):
            entry = entries[tile_y * 32 + tile_x]
            tile = entry & 0x03FF
            hflip = bool(entry & 0x0400)
            vflip = bool(entry & 0x0800)
            bank = (entry >> 12) & 0xF
            src_tile_x = (tile % 32) * 8
            src_tile_y = (tile // 32) * 8

            for py in range(8):
                sy = 7 - py if vflip else py
                for px in range(8):
                    sx = 7 - px if hflip else px
                    color_index = source_pixels[
                        src_tile_x + sx,
                        src_tile_y + sy,
                    ]
                    value = palettes[bank * 16 + color_index]
                    pixels[tile_x * 8 + px, tile_y * 8 + py] = (
                        bgr555_to_rgb(value)
                    )

    # The DS strip is 256 pixels wide and has one empty 8-pixel margin on
    # each side. Remove only those margins to reach the 240-pixel GBA width.
    # No artwork is scaled, redrawn, recolored, or interpolated.
    rgb = rgb.crop((8, 0, 248, 32))

    # Emit an indexed 4bpp-compatible PNG while preserving only exact retail
    # colors. Palette-index reassignment is lossless and does not alter pixels.
    colors = []
    color_to_index = {}
    indexed = Image.new("P", rgb.size)
    out_pixels = indexed.load()
    rgb_pixels = rgb.load()

    for y in range(rgb.height):
        for x in range(rgb.width):
            color = rgb_pixels[x, y]
            if color not in color_to_index:
                if len(colors) >= 16:
                    raise ValueError("retail crop exceeds 16 colors")
                color_to_index[color] = len(colors)
                colors.append(color)
            out_pixels[x, y] = color_to_index[color]

    palette = []
    for color in colors:
        palette.extend(color)
    palette.extend([0] * (768 - len(palette)))
    indexed.putpalette(palette)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    indexed.save(out_path, optimize=False)


if __name__ == "__main__":
    main()
