#!/usr/bin/env python3
"""Build the custom Stats page entirely from authentic HGSS Pokédex pixels."""

import base64
import hashlib
import struct
import sys
import zlib

WIDTH = 256
HEIGHT = 192
OUT_WIDTH = 240
OUT_HEIGHT = 160

PALETTE = [
    (255, 172, 172),
    (123, 90, 180),
    (213, 180, 255),
    (205, 222, 255),
    (115, 123, 123),
    (255, 255, 255),
    (90, 98, 98),
    (172, 115, 255),
    (255, 197, 148),
    (255, 148, 49),
    (255, 115, 115),
    (238, 238, 222),
    (197, 255, 255),
    (49, 115, 197),
    (197, 106, 32),
]

SOURCES = {
    57: (
        "7765c42ee555e158c34e45a8fc380abb0540cbfdbff5508538a986ca543e9698",
        "eNrt3dt2mzAQBVA0DgUn+f/vrd3iWwK20Vw0oznz0NUHi9UNspCPQB2GzSrDe9XL5+CHP52/JC9KXgdU00p+Xal8qFQJcgLoQ6ngh9/3uJfdT/79ireR5R7l2q95H11u0vCj/6ce/0alinL/y+q/nIY/SgU//BH8kw5/ChMAINfq8vxHqXm7pvm96uVzKf0+emFhuUr0cfL4cAL2+gsd446blxPAWNdrwi+yOT3LHzm3vfjHMbt/zO4fI/ilc/t4furPv2MaIJ1bivkZU5k98yDp3FrKz5nLefCfJ3Kt/B76P9d/MJrKS+f2Yv6guX00v3Rue/FPp6p9hK8Hf/brH80/yeb00fzi+YeGP8xawJJ/tfB7yj/F+/8bl99TXi3//X/d+/v2v54VR123Ko3H//YjZtT7n0z7I7Wd/xRe4s9tf14xWPfbLHcR8xc/CSQG05r/+YkV+63kwj+v+W0ei/Thn+39/Pw/up/685vmn/78tvk3/Jr+z+eNP3vv/1V+0/xfpH1D/yG1n5//kkR7+OFv6p94iT+3fWu/g/yjrd/BioGIP2z+Ocv4Yz/XDn/q/j/kHv+GIc79T7r9Vv5vNf/Zmd//DJ657Tfzf6/5L8m2387/E/lX83+b1+J9+Gd7f23+L++/PjFIO36YNsv/za5/jd8g/6S7nPn0p55/2v9avEX+TbenkUjTX7P+beO/2KvP36b/KDSo6fZ/Iu73Z8Nv+v5rbf7/c/xj72dz8w+W7z/X5v+a/lNV/562yn9Jtj38j/4hu38w9k91qb1U+9/+yv2/4+YfPP/XUh3k/yx/B/k/y99B/g1/wv5f8b5AT+NfxXgp69fap07ruA87DPH90vv/aB/3cYchAb/SPr1ax/33hml2/5zKT/SQa92/YcvwK+3/E8hP/fkb73/CWRd44f//j6Xn/sb737DWBV76l+Ddr5+3LvCq/1+/BW77P29d4C0/yY9/SvvUCx73dvFl/YdIfpK+/vznv2Vz4e3jXsf/818S+lfmv/CL+YX3/9E+rrQ/YP4h6w+4XrDqT5N/zuv+XPvf//ZPqeq2aeD3Ug72+7e7/l7y/+PTE6Dm95D/r6fx9v//d6P8f22//5B+7tv7d/v9J/WP2f1jRD83/4/vp/78hvmnR79l/g0/+n8P4x9395ro97+sfm5OCz/8PfgnXi4f3c/OP6L7ufmXF3+r/NOLv1VeD396/5Db7yP/bJ1Dw+/mc/DDDz/88MMPP/zww9/950ryQv+HH3744Ycffvjhhx9++OGHH3744Ycffvjhhx9++OGHv6fP/QUjYPtE"
    ),
    59: (
        "b6ab28ac5b84c7d6d21f584e78d52c602775ca17a5b0bb8c13267dd9008a079a",
        "eNrt2rENAEAIAkDdf+mfwORb9KhpLqGkakzXX7b0+PnP+ft47J+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn58/sef/Z//8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8eT3/P/vn5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fnz+v5/9k/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/f17P/8/++fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+ff0HuX1CWM="
    ),
    62: (
        "95fabe0a9c4555bcdb0e8972f54dd1b15e43ba60d4e19cf2042398ade8bf7d33",
        "eNrt2u1ugjAUBmDKZBQT7/925zI/olhwiYWmfc4fk6Wx54FOtG+7Llmhe69qGcffuD/0VdfL6xDu1VdfYV4PN3/6qrim1BL4q8r1lyuw9L9fRI+5lv7lzUPht7+fTllq6i8LIMaUv4zbf8pU1wUQExeAvxl/LNF//YRq19+X49/hYlyeUUX4b8/LrZ/7/Na/zz/Pvwb9rX//4d/b38rv39f+VvY/YvoHYAv7XzHpb2P/M+3vWtj/jgv++vOPGJf9XbzXMAxjfKM+Pe5cucd1/OsXoF7/cga64X3YZZz8m5+fn5+fn5+fv2V/aKSu3/+T5/+sf/6H7eBbHc91fT0+/+E/r79V6rzPbYRDlgp9mfPO+jhkqn4oct4N+xhKnJefn5+fn5//3MeQqdb8O83L/9THd6Za8+80Lz//Qx9jnjbGNf9O8y7tQ3y04kofe807q1znENb62GveeSOZ+ih1Xvuf/Pz8/Pz8/Pz8/K365f/yf/m//Kd2f+K4XL4+xnHpmN7m81r//Pz8/EXk0PJ/+b/8m1/+L/+X/8v/5f/2f/n5+fn5+fn5+fmz++X/8n/5v/yHn5+fn59f/i//r82fOoeXLYddOf+39byF5dDyf/m//F/+L/+X/9v/5efn5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn539rXGi8rH9+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn7+msb9ABljatc="
    ),
    65: (
        "b858f5d6fcee57297a8f2854b517caab327d86a0a493b49e5f2b81522f1c0349",
        "eNrt3N2OgjAQBlDaDbFcuO//uIs/xLjZxALC4vR8F3ozIqcMFoHQdauTgtTxN+5POVjSa396JAdMepWnjT98hcpwa4G6/g+nv49ApX/c/G/5xqWNutFyLg2Qdtz8eTgvypC3Wc44AOVU53/P5j8vTN5mOVd/4W/bX7b33z/fsD/H8S8YjPvcs2q9p/lrfP1X/1/zcdW8v84/TeC5Tf9kXz2OH9r/U++v34/8/pn/9va3fvzD/+n+uP9/a/xxz3/U+cOe/yqV/qjnP2v9Xczz36XaH/H6Rylz/F15pO/7U6lIbd1tPfZf3rWu4587AJH8cy7mLhrfA9f1/eLr36mxOn5+fn5+fn5+/gbqUuPR//z8/Pz8/Pz8/Pzt+r+Dh19E//Ob//j5+fn5+fn5+fn5+fn5+fn5+flj17n/T//z8/Pz81d9LuV0RNfv1drMny8PVb0eOnRHep9Wazf/9H0Heeffz3/Q8Ot/fvOf4x/Hv/z8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/E373f+n//n5+W/x/A/P/5B2Y/+3/5v/+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fn5P7HO/X/6n5+fn5+fn5+fn5+fn5+fn5+fn5+fn5+fnz9O3Q9X6Q2r"
    ),
}

EXPECTED_LAYOUT_SHA256 = "dd5759ea0df2948e8eb98bbabaa6edfe70b02841f01d0cf208a71625280ac7c9"


def png_chunk(chunk_type, data):
    crc = zlib.crc32(chunk_type + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + chunk_type + data + struct.pack(">I", crc)


def decode_member(member):
    expected_hash, payload = SOURCES[member]
    pixels = zlib.decompress(base64.b64decode(payload))
    if len(pixels) != WIDTH * HEIGHT:
        raise SystemExit(f"member {member:03d}: bad pixel payload size {len(pixels)}")
    if hashlib.sha256(pixels).hexdigest() != expected_hash:
        raise SystemExit(f"member {member:03d}: checksum mismatch")
    return [bytearray(pixels[y * WIDTH:(y + 1) * WIDTH]) for y in range(HEIGHT)]


def crop(rows, x0, y0, x1, y1):
    return [bytearray(row[x0:x1]) for row in rows[y0:y1]]


def paste(dst, src, x, y):
    for row_index, row in enumerate(src):
        dst[y + row_index][x:x + len(row)] = row


def build_layout():
    m57 = decode_member(57)
    m59 = decode_member(59)
    m62 = decode_member(62)
    m65 = decode_member(65)

    out = crop(m59, 0, 0, OUT_WIDTH, OUT_HEIGHT)
    paste(out, crop(m65, 8, 4, 229, 30), 8, 0)
    paste(out, crop(m62, 8, 38, 120, 136), 0, 36)
    paste(out, crop(m57, 48, 24, 192, 152), 96, 32)

    flattened = b"".join(bytes(row) for row in out)
    if hashlib.sha256(flattened).hexdigest() != EXPECTED_LAYOUT_SHA256:
        raise SystemExit("HGSS Stats production layout checksum mismatch")
    return out


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
        f"{path}: HGSS-derived Stats layout, {OUT_WIDTH}x{OUT_HEIGHT}, "
        f"{len(PALETTE)} colors, {len(png)} bytes"
    )


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} OUTPUT.png")
    write_png(sys.argv[1], build_layout())


if __name__ == "__main__":
    main()
