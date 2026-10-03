#!/usr/bin/env python3
"""Rebuild the authentic HGSS Pokédex list screen and its 240x160 GBA layout."""

import argparse
import base64
import hashlib
import struct
import zlib

SOURCE_WIDTH = 256
SOURCE_HEIGHT = 192
EXPECTED_SOURCE_SHA256 = "3bc5705e9b3511ee7a84dc1f65663cf185401c78cd2393b352f803b5fbecc475"
EXPECTED_GBA_LAYOUT_SHA256 = "9dbd8b9cd1ae7cbdb0933d4bf63fc274a4d39e65d078e7c43e8e770763c851b7"

PALETTE = [
    (238, 49, 49),
    (197, 32, 41),
    (41, 41, 41),
    (246, 172, 180),
    (255, 255, 255),
    (0, 0, 0),
    (246, 189, 41),
    (139, 98, 65),
    (205, 148, 74),
    (148, 16, 32),
    (156, 172, 189),
    (98, 98, 98),
    (115, 197, 131),
    (205, 213, 222),
    (139, 131, 131),
    (90, 90, 90),
]

PIXELS_ZLIB_B64 = """eNrt3It2oyAUheFAMYxnmun7v+3IRdRoNVeNnn+vVZNJqPAhoFmxczo9FWM3jjltGvy79X855/T6G31VqfVHvVp/1iv1F71Sf9Ernv+c//Djx48fP/77wOcQGx6ssT78w8eWxFfTQ3rN56dtaduVzmVzfv1N29t5r2R5O/hNalHaGLOS32f/OftGit7TtrTtSs/5+10xLDdVMvh9LupXGA2FfI///KC/HH4/HDe9kqEHctE1Dn/x+9bv/Q1+37Jz6Sn/ebrnbOm3sx2/HdFpXq1y+Is/tcenRvkl/3Xpkf88mCf9mdMuG5NvJ3+7JpnV/dcHZd4/+pVrv532x0f/y4638Pu7/P5G/3iCD/Y83nGZ/2uPf9sTXbVyqpnj0pPrn53y+8H4mFz/11z/mr7+88F5+xBQ79/8gnfhcvjdM8BcZXvwMHz+wY8f/5Z9ElO/Ullvsri94jxRvwS+K/r06bJ+iL1b+Ox1Q71gPg77pmsoDWZCCCGEEEIOmqXbql9w2zX1U/8H1z9Xg3PUf+z6CSGEELK3mENnmV/bA6c2qvnLHTB3P0O4ck4bm3+cbV/Jm1xSnEvlxt9r3pW6q9g5Sc+7em1pSVfzs/dLzPplWGvbDul6opSVV/ulbcSG/kY19EtoV/dyZ0498MstarenDDzp71baetuWSL/A+/xP38Px4OFf834hYz+nA96yFG/hb3rggWxyv5ixBw9+/Pjx48ePHz9+/Pjx48ePHz9+/Pjx48ePH//R/X8JIYQQQgghhBBCCCGEEEIIIYQQsvv0/tuLwd+efisJfufcl2p/VTU9oNqfekCzP/aAav+gB1T6mx7Q5w+rXyXN9hJ7QOJGlPkD+9INg0qdv4p+EQkbtf44DvAr9Es3/lXOfyfZr2/9z7lcLiJ1LTG6P//qwU/6rf2n3P+NHz9+/Pjx48ePHz9+/Pjx48ePHz9+/Pjx48ePf9f+8P2nar+16v3x+29R608Rxce/0XP88Ws9/2le/07q/aJ7/AvrH+c/5dc/uq9/HZ9/dfvz/c9qx384Bf4onv/a1/90BaDP/x9vQ7D7"""


def png_chunk(chunk_type, data):
    crc = zlib.crc32(chunk_type + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + chunk_type + data + struct.pack(">I", crc)


def decode_source():
    pixels = zlib.decompress(base64.b64decode(PIXELS_ZLIB_B64))
    if len(pixels) != SOURCE_WIDTH * SOURCE_HEIGHT:
        raise SystemExit(f"bad list pixel payload size: {len(pixels)}")
    if hashlib.sha256(pixels).hexdigest() != EXPECTED_SOURCE_SHA256:
        raise SystemExit("HGSS list source checksum mismatch")
    return [bytearray(pixels[y * SOURCE_WIDTH:(y + 1) * SOURCE_WIDTH]) for y in range(SOURCE_HEIGHT)]


def build_gba_layout(source):
    # Preserve the DS list geometry at 1:1. The 16 px horizontal reduction is
    # taken from a flat interior strip between the selected-mon area and the
    # right-side counters; the bottom 32 px are list continuation, not a frame.
    rows = [row[:208] + row[224:] for row in source[:160]]

    flattened = b"".join(bytes(row) for row in rows)
    if hashlib.sha256(flattened).hexdigest() != EXPECTED_GBA_LAYOUT_SHA256:
        raise SystemExit("HGSS list 240x160 layout checksum mismatch")
    return rows


def write_indexed_png(path, rows):
    height = len(rows)
    width = len(rows[0])
    scanlines = [bytes([0]) + bytes(row) for row in rows]

    palette = b"".join(bytes(rgb) for rgb in PALETTE)
    palette += bytes(768 - len(palette))

    png = bytearray(b"\x89PNG\r\n\x1a\n")
    png += png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 3, 0, 0, 0))
    png += png_chunk(b"PLTE", palette)
    png += png_chunk(b"IDAT", zlib.compress(b"".join(scanlines), 9))
    png += png_chunk(b"IEND", b"")

    with open(path, "wb") as f:
        f.write(png)

    return len(png)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", action="store_true",
                        help="emit the untouched 256x192 left DS list screen")
    parser.add_argument("output")
    args = parser.parse_args()

    source = decode_source()
    rows = source if args.source else build_gba_layout(source)
    png_size = write_indexed_png(args.output, rows)

    label = "authentic 256x192 source" if args.source else "240x160 production layout"
    print(
        f"{args.output}: HGSS Pokédex list member 000 {label}, "
        f"{len(rows[0])}x{len(rows)}, {len(PALETTE)} colors, {png_size} bytes"
    )


if __name__ == "__main__":
    main()
