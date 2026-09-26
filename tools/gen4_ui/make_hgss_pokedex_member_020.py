#!/usr/bin/env python3
"""Rebuild an authentic 256x192 HGSS Pokédex layer from exact extracted pixels."""

import base64
import hashlib
import struct
import sys
import zlib

WIDTH = 256
HEIGHT = 192
EXPECTED_PIXEL_SHA256 = "17114a41f5c9b95f36e6a1d3b65830c9db538bbb3856e1526b2b9bb360a0d824"

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


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} OUTPUT.png")

    pixels = zlib.decompress(base64.b85decode(PIXELS_B85))
    if len(pixels) != WIDTH * HEIGHT:
        raise SystemExit(f"bad pixel payload size: {len(pixels)}")
    if hashlib.sha256(pixels).hexdigest() != EXPECTED_PIXEL_SHA256:
        raise SystemExit("HGSS pixel payload checksum mismatch")

    rows = []
    for y in range(HEIGHT):
        row = pixels[y * WIDTH:(y + 1) * WIDTH]
        rows.append(bytes([0]) + row)

    palette = b"".join(bytes(rgb) for rgb in PALETTE)
    palette += bytes(768 - len(palette))

    png = bytearray(b"\x89PNG\r\n\x1a\n")
    png += png_chunk(b"IHDR", struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 3, 0, 0, 0))
    png += png_chunk(b"PLTE", palette)
    png += png_chunk(b"IDAT", zlib.compress(b"".join(rows), 9))
    png += png_chunk(b"IEND", b"")

    with open(sys.argv[1], "wb") as f:
        f.write(png)

    print(
        f"{sys.argv[1]}: authentic HGSS Pokédex member 020, "
        f"{WIDTH}x{HEIGHT}, {len(PALETTE)} colors, {len(png)} bytes"
    )


if __name__ == "__main__":
    main()
