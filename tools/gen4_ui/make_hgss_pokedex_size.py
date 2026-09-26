#!/usr/bin/env python3
"""Build the Size comparison page from authentic HGSS Pokédex member 097."""

import base64
import hashlib
import struct
import sys
import zlib

SOURCE_WIDTH = 256
SOURCE_HEIGHT = 192
OUT_WIDTH = 240
OUT_HEIGHT = 160
EXPECTED_SOURCE_SHA256 = "a3592f0d08c489b0ea1a399e199bf70a9c363407b8bf3e8b46fc304cfe9f3b56"
EXPECTED_LAYOUT_SHA256 = "680d17b844b4e5198ccdd2df5cacc9462a4446facdf06a806cee369fe80693cd"

PALETTE = [
    (106, 16, 189),
    (148, 65, 222),
    (189, 115, 246),
    (90, 90, 90),
    (255, 255, 255),
    (41, 41, 41),
    (172, 172, 172),
    (131, 131, 131),
    (205, 205, 205),
    (230, 230, 230),
    (222, 189, 255),
    (197, 205, 213),
    (156, 172, 189),
]

PIXELS_ZLIB_B64 = """eNrt3dtugkAUQFERChX8/+9trYlBKjJNOsrMWedVkuVG4gW5HA4/0zTHWNM0h/no1x+0v51NyP7uNvpDbv+3/JD9txUQKf/u/f+6AkLl33/+XVZArPzF53+szz7ff/TrX/SHm2v4hwk9/foMfdrUspx+/fqD9X+uz7PHalwuZP9pffpT2tSyXMj+bn36Lm1qWU6/fv369evXr19/wP6+rXr62PmbK2DxP+g4doXPOM572rbd6j/W3H/UH71/mqa7N7z7qb9/+J5ZcMTXfwq+/evXr1+/fv369Yf6/fu8v/79H8/7q9//NW30X2a6zjClTXnL6de/vQKq7ff/h379+vXr169fv/75nPP89jynPt83++dcP77PZfj59j6U4///CcdF+TnOuG5/HXuwX3/MMJv7Xnfk59gTzefz+fw9+Q9Oic7nL469W5mX+rY/Pp/P5/P5fD6fz+fz+Xw+n8/n8/l8Pp/P5/P5fD6fz+eX6Dv+z/bH5/P5fD6fz+fz+Xw+n8/n8/l8Pp/P5/P5fD6fz6/Nj379y3zXHy3Dz3T92XL8PLND/8Hxd9muPz2kHf/3Uv/RC5DpCQyJN9/p3+xfJt99iBL6u9stqDLc/0i//rQVkKW/S+vvcvX/4Quj+3/o169fv379+vXr169fv379+vXr169ff8HLfQG62zWf"""


def png_chunk(chunk_type, data):
    crc = zlib.crc32(chunk_type + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + chunk_type + data + struct.pack(">I", crc)


def build_layout():
    pixels = zlib.decompress(base64.b64decode(PIXELS_ZLIB_B64))
    if len(pixels) != SOURCE_WIDTH * SOURCE_HEIGHT:
        raise SystemExit(f"bad member 097 payload size: {len(pixels)}")
    if hashlib.sha256(pixels).hexdigest() != EXPECTED_SOURCE_SHA256:
        raise SystemExit("HGSS member 097 source checksum mismatch")

    rows = [
        bytearray(pixels[y * SOURCE_WIDTH:(y + 1) * SOURCE_WIDTH])
        for y in range(SOURCE_HEIGHT)
    ]

    # Member 097 already provides two authentic comparison frames. Remove one
    # empty 8px grid strip immediately above them so both lower borders fit
    # inside the GBA's 160px height. No artwork is scaled or redrawn.
    rows = rows[:64] + rows[72:]
    rows = [row[:OUT_WIDTH] for row in rows[:OUT_HEIGHT]]

    flattened = b"".join(bytes(row) for row in rows)
    if hashlib.sha256(flattened).hexdigest() != EXPECTED_LAYOUT_SHA256:
        raise SystemExit("HGSS Size 240x160 layout checksum mismatch")
    return rows


def write_png(path, rows):
    scanlines = [bytes([0]) + bytes(row) for row in rows]
    palette = b"".join(bytes(rgb) for rgb in PALETTE)
    palette += bytes(768 - len(palette))

    png = bytearray(b"\x89PNG\r\n\x1a\n")
    png += png_chunk(b"IHDR", struct.pack(">IIBBBBB", OUT_WIDTH, OUT_HEIGHT, 8, 3, 0, 0, 0))
    png += png_chunk(b"PLTE", palette)
    png += png_chunk(b"IDAT", zlib.compress(b"".join(scanlines), 9))
    png += png_chunk(b"IEND", b"")

    with open(path, "wb") as f:
        f.write(png)

    print(
        f"{path}: authentic HGSS member 097 Size layout, "
        f"{OUT_WIDTH}x{OUT_HEIGHT}, {len(PALETTE)} colors, {len(png)} bytes"
    )


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} OUTPUT.png")
    write_png(sys.argv[1], build_layout())


if __name__ == "__main__":
    main()
