#!/usr/bin/env python3
"""Rebuild an authentic 256x192 HGSS Pokédex layer from exact extracted pixels."""

import argparse
import base64
import hashlib
import struct
import zlib

WIDTH = 256
HEIGHT = 192
EXPECTED_PIXEL_SHA256 = "17114a41f5c9b95f36e6a1d3b65830c9db538bbb3856e1526b2b9bb360a0d824"
EXPECTED_GBA_LAYOUT_SHA256 = "f17b1f8d5fa69df5d88a2beab52adf47119015057062d1e8723c5df9943c3d56"

PALETTE = [
    (148, 16, 32),
    (197, 32, 41),
    (238, 49, 49),
    (90, 90, 90),
    (255, 255, 255),
    (41, 41, 41),
    (172, 172, 172),
    (131, 131, 131),
    (205, 205, 205),
    (230, 230, 230),
    (197, 205, 213),
    (255, 156, 164),
    (156, 172, 189),
]

PIXELS_B85 = """c-rmV?T*4A5QSkYEUd`=U+?<a6k{!B>vWuXPvVb_lZ_7{QqrOBc9#sp-dBcUHw}LLPW@pLm;N{gzsvbQ@?HAJ0r13czW=WS;ELa_f9C=4#eZ1;eXV^@u7AAox&Fi7`rYLU{~@lE=95!jtkmJhkKdX9cRsJ)d%G?NRmp3)I_t8s{lz!{hxw#KUwZ1)ZLh*QuCMpU`J~vFo;rR0QpyUhul5)G`0?Y%j~_pN{P;iQKPn4;S$|X({P@rQ^qJJ!{Hf5NPdR@w=0AS?`0?Y%j~_q&_5D@Z0E8;*KMjDYT=<gy7)vgFk*mY++8;&z(Ys%J9scdsziQlH7j6Je2f(6#@%~r9Z`$8;KgzQIjr|yY{P^+X$B!RBe*F0H<HwJG<<BS6|Cg5iw|+e7F#qvu@(bN3uS)$=eG)wbu!Uc&O(fv=?H4}(uhC!d<CpO-t!wvwJgff2{)b}RAA9<r@Z-1ZPs@DU{&g-(@tgM7)6P`jg`fG4A3uKl_@8fCJO5S1&-}-aA3uKl`2SHScmG?bU#$P3DD_9lbJF<T`;)?Cp$PW>eM237{P^+X$B*BaU)kf?q#NMf@gD|Y{Q2{v`P?2i6GH3X-~G{a?*2Xg>HgoTzu?D@A3uKl`0?Y%j~_pN{P^+X$B!RBe*E|?`frctbLhWz05bpa<HwI5KYsl9@#DvjA3uKl_<i{O^}kR39?w5C2f(XKeschRyWe5ot`0x`*#WTcf4x?6{?kF-AKNE2PX"""


def png_chunk(chunk_type, data):
    crc = zlib.crc32(chunk_type + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + chunk_type + data + struct.pack(">I", crc)


def decode_source_pixels():
    pixels = zlib.decompress(base64.b85decode(PIXELS_B85))
    if len(pixels) != WIDTH * HEIGHT:
        raise SystemExit(f"bad pixel payload size: {len(pixels)}")
    if hashlib.sha256(pixels).hexdigest() != EXPECTED_PIXEL_SHA256:
        raise SystemExit("HGSS pixel payload checksum mismatch")
    return [bytearray(pixels[y * WIDTH:(y + 1) * WIDTH]) for y in range(HEIGHT)]


def remove_columns(rows, start, count):
    return [row[:start] + row[start + count:] for row in rows]


def remove_rows(rows, start, count):
    return rows[:start] + rows[start + count:]


def crop(rows, x0, y0, x1, y1):
    return [bytearray(row[x0:x1]) for row in rows[y0:y1]]


def paste(dst, src, x, y):
    for row_index, row in enumerate(src):
        dst[y + row_index][x:x + len(row)] = row


def build_gba_layout(source):
    # Start with the 240x160 DS viewport, then compact only empty interior
    # stretches. No source pixel is scaled or redrawn.
    out = [bytearray(row[:240]) for row in source[:160]]

    # Preserve both ends of the 250px top ribbon by deleting 10 flat center px.
    header = remove_columns(crop(source, 0, 0, 250, 16), 120, 10)
    paste(out, header, 0, 0)

    # Keep the authentic right-hand info frame at its original x position;
    # shorten only its empty center span so its right cap fits at x=239.
    info_box = remove_columns(crop(source, 103, 23, 250, 59), 60, 10)
    paste(out, info_box, 103, 23)

    # Same treatment for the height/weight box.
    measurement_box = remove_columns(crop(source, 143, 87, 250, 122), 48, 10)
    paste(out, measurement_box, 143, 87)

    # Remove the clipped DS description panel from the lower viewport using
    # an authentic repeating 40x40 blank-grid sample from this same screen.
    for y in range(122, 160):
        sample_y = 45 + ((y - 45) % 40)
        for x in range(240):
            sample_x = 28 + ((x - 28) % 40)
            out[y][x] = source[sample_y][sample_x]

    # The 244x54 description panel is compacted to 234x34 by deleting only
    # blank interior columns/rows. Its border pixels remain 1:1 HGSS pixels.
    description_box = crop(source, 6, 134, 250, 188)
    description_box = remove_columns(description_box, 115, 10)
    description_box = remove_rows(description_box, 22, 20)
    paste(out, description_box, 6, 126)

    flattened = b"".join(bytes(row) for row in out)
    if hashlib.sha256(flattened).hexdigest() != EXPECTED_GBA_LAYOUT_SHA256:
        raise SystemExit("HGSS 240x160 production layout checksum mismatch")
    return out


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
    parser.add_argument("--gba-layout", action="store_true",
                        help="emit the 240x160 production layout instead of the untouched 256x192 DS layer")
    parser.add_argument("output")
    args = parser.parse_args()

    source = decode_source_pixels()
    rows = build_gba_layout(source) if args.gba_layout else source
    png_size = write_indexed_png(args.output, rows)

    label = "240x160 production layout" if args.gba_layout else "authentic 256x192 source"
    print(
        f"{args.output}: HGSS Pokédex member 020 {label}, "
        f"{len(rows[0])}x{len(rows)}, {len(PALETTE)} colors, {png_size} bytes"
    )


if __name__ == "__main__":
    main()
