#!/usr/bin/env python3
"""Build authentic HGSS Pokédex scroll-control OBJ graphics from member 000.

The source pixels are copied 1:1 from HGSS member 000's native upper-right
control arrow. Source palette index 0 is the red screen background and becomes
OBJ transparency; the pink/white/black edge colors are preserved.

Tile layout:
  tiles 0-1: 16x8 arrow
  tile 2:    8x8 scrollbar marker cropped from the same authentic arrow
"""

import argparse
import hashlib
import struct

from make_hgss_pokedex_list_member_000 import PALETTE, decode_source

ARROW_X = 240
ARROW_Y = 0
ARROW_W = 16
ARROW_H = 8
BAR_X = 246
BAR_Y = 0
BAR_W = 8
BAR_H = 8

EXPECTED_ARROW_SHA256 = "ea5b9b2ac4141931be38eb7ac8e9ae1866ec26b4b2e252fe64f8aaeded5d6a36"
EXPECTED_BAR_SHA256 = "00c554e81ac7204af5897257270598f8d58784ba756565ee1039851e53d60f76"

# Source index 0 is background/transparency. The authentic arrow itself uses
# member-000 colors 3 (pink edge), 4 (white), and 5 (black).
SOURCE_TO_OBJ = {
    0: 0,
    3: 1,
    4: 2,
    5: 3,
}


def crop_remap(source, x, y, width, height):
    out = []
    for py in range(y, y + height):
        for px in range(x, x + width):
            source_index = source[py][px]
            if source_index not in SOURCE_TO_OBJ:
                raise SystemExit(
                    f"unexpected member-000 color {source_index} at {px},{py}"
                )
            out.append(SOURCE_TO_OBJ[source_index])
    return out


def encode_tile_4bpp(pixels):
    if len(pixels) != 64:
        raise SystemExit(f"tile has {len(pixels)} pixels, expected 64")
    out = bytearray()
    for i in range(0, 64, 2):
        out.append((pixels[i] & 0xF) | ((pixels[i + 1] & 0xF) << 4))
    return bytes(out)


def rgb555(rgb):
    r, g, b = rgb
    return (
        min(31, (r + 4) // 8)
        | (min(31, (g + 4) // 8) << 5)
        | (min(31, (b + 4) // 8) << 10)
    )


def build_tiles():
    source = decode_source()
    arrow = crop_remap(source, ARROW_X, ARROW_Y, ARROW_W, ARROW_H)
    bar = crop_remap(source, BAR_X, BAR_Y, BAR_W, BAR_H)

    if hashlib.sha256(bytes(arrow)).hexdigest() != EXPECTED_ARROW_SHA256:
        raise SystemExit("HGSS scroll-arrow pixel checksum mismatch")
    if hashlib.sha256(bytes(bar)).hexdigest() != EXPECTED_BAR_SHA256:
        raise SystemExit("HGSS scrollbar-marker pixel checksum mismatch")

    left = []
    right = []
    for y in range(8):
        left.extend(arrow[y * 16:y * 16 + 8])
        right.extend(arrow[y * 16 + 8:y * 16 + 16])

    return encode_tile_4bpp(left) + encode_tile_4bpp(right) + encode_tile_4bpp(bar)


def build_palette():
    colors = [
        (0, 0, 0),   # transparent
        PALETTE[3],  # authentic pink edge
        PALETTE[4],  # authentic white
        PALETTE[5],  # authentic black
    ]
    colors += [(0, 0, 0)] * (16 - len(colors))
    return b"".join(struct.pack("<H", rgb555(color)) for color in colors)


def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--tiles", action="store_true")
    group.add_argument("--palette", action="store_true")
    parser.add_argument("output")
    args = parser.parse_args()

    if args.tiles:
        data = build_tiles()
        label = "3 OBJ tiles"
    else:
        data = build_palette()
        label = "16-color OBJ palette"

    with open(args.output, "wb") as f:
        f.write(data)

    print(f"{args.output}: authentic HGSS member-000 scroll controls, {label}")


if __name__ == "__main__":
    main()
