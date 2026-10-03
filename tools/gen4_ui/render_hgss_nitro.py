#!/usr/bin/env python3
"""
Render authentic Nintendo DS Nitro UI graphics used by HGSS.

This tool is intentionally read-only with respect to source assets. It converts
decoded NCGR/NCLR/NSCR members into PNG previews so archive members can be
identified before they are adapted for pokeemerald.

Examples:
    # Reconstruct a tiled screen exactly from its NSCR map:
    python tools/gen4_ui/render_hgss_nitro.py \
        --ncgr 012_ncgr.ncgr --nclr 011_nclr.nclr --nscr 013_nscr.nscr \
        --output preview.png

    # Render a raw NCGR as a tile sheet when no NSCR is available:
    python tools/gen4_ui/render_hgss_nitro.py \
        --ncgr 012_ncgr.ncgr --nclr 011_nclr.nclr \
        --width-tiles 16 --palette-index 0 --output tiles.png

Only Python's standard library is required.
"""

from __future__ import annotations

import argparse
import math
import struct
import zlib
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Ncgr:
    bit_depth: int
    width_tiles: int | None
    height_tiles: int | None
    scanned: bool
    tiles: tuple[tuple[int, ...], ...]


@dataclass(frozen=True)
class Nclr:
    bit_depth: int
    colors: tuple[tuple[int, int, int], ...]


@dataclass(frozen=True)
class Nscr:
    bit_depth: int
    width_tiles: int
    height_tiles: int
    entries: tuple[int, ...]


def u16(data: bytes, off: int) -> int:
    return struct.unpack_from("<H", data, off)[0]


def s16(data: bytes, off: int) -> int:
    return struct.unpack_from("<h", data, off)[0]


def u32(data: bytes, off: int) -> int:
    return struct.unpack_from("<I", data, off)[0]


def find_block(data: bytes, magic: bytes) -> int:
    if len(data) < 0x10:
        raise ValueError("Nitro file is truncated")

    header_size = u16(data, 0x0C)
    block_count = u16(data, 0x0E)
    pos = header_size

    for _ in range(block_count):
        if pos + 8 > len(data):
            raise ValueError("Nitro block header is truncated")
        block_size = u32(data, pos + 4)
        if block_size < 8 or pos + block_size > len(data):
            raise ValueError("Nitro block has an invalid size")
        if data[pos : pos + 4] == magic:
            return pos
        pos += block_size

    raise ValueError(f"Nitro block {magic!r} was not found")


def expand_5bit(value: int) -> int:
    # Match the HGSS nitrogfx conversion closely: 0..31 -> 0..255.
    return value * 255 // 31


def decode_bgr555(value: int) -> tuple[int, int, int]:
    red = expand_5bit(value & 0x1F)
    green = expand_5bit((value >> 5) & 0x1F)
    blue = expand_5bit((value >> 10) & 0x1F)
    return red, green, blue


def parse_nclr(path: Path) -> Nclr:
    data = path.read_bytes()
    if data[:4] not in {b"RLCN", b"RPCN"}:
        raise ValueError(f"{path}: not an NCLR/NCPR file")

    off = find_block(data, b"TTLP")
    if off + 0x18 > len(data):
        raise ValueError(f"{path}: truncated TTLP block")

    bit_depth = 4 if data[off + 0x08] == 3 else 8
    palette_size = u32(data, off + 0x10)
    palette_off = off + 0x18
    if palette_size == 0:
        palette_size = u32(data, off + 4) - 0x18
    if palette_size % 2:
        raise ValueError(f"{path}: odd NCLR palette byte count")
    if palette_off + palette_size > len(data):
        raise ValueError(f"{path}: NCLR palette extends past end of file")

    colors = tuple(
        decode_bgr555(u16(data, palette_off + i))
        for i in range(0, palette_size, 2)
    )
    if not colors:
        raise ValueError(f"{path}: NCLR contains no colors")

    return Nclr(bit_depth=bit_depth, colors=colors)


def decode_4bpp_tile(raw: bytes) -> tuple[int, ...]:
    if len(raw) != 32:
        raise ValueError("4bpp tile must contain exactly 32 bytes")
    pixels: list[int] = []
    for byte in raw:
        pixels.append(byte & 0x0F)
        pixels.append(byte >> 4)
    return tuple(pixels)


def decode_8bpp_tile(raw: bytes) -> tuple[int, ...]:
    if len(raw) != 64:
        raise ValueError("8bpp tile must contain exactly 64 bytes")
    return tuple(raw)


def parse_ncgr(path: Path) -> Ncgr:
    data = path.read_bytes()
    if data[:4] != b"RGCN":
        raise ValueError(f"{path}: not an NCGR file")

    off = find_block(data, b"RAHC")
    if off + 0x20 > len(data):
        raise ValueError(f"{path}: truncated RAHC block")

    height_raw = s16(data, off + 0x08)
    width_raw = s16(data, off + 0x0A)
    bit_depth = 4 if data[off + 0x0C] == 3 else 8
    scanned = bool(data[off + 0x14])
    data_size = u32(data, off + 0x18)
    char_off = off + 0x20

    # Some NCGRs use a clobbered size field. The block size remains authoritative.
    max_data_size = u32(data, off + 4) - 0x20
    if data_size <= 0 or data_size > max_data_size:
        data_size = max_data_size
    if char_off + data_size > len(data):
        raise ValueError(f"{path}: character data extends past end of file")

    tile_bytes = 32 if bit_depth == 4 else 64
    if data_size % tile_bytes:
        raise ValueError(
            f"{path}: character data size {data_size} is not aligned to {tile_bytes}-byte tiles"
        )

    raw = data[char_off : char_off + data_size]
    decoder = decode_4bpp_tile if bit_depth == 4 else decode_8bpp_tile
    tiles = tuple(
        decoder(raw[i : i + tile_bytes])
        for i in range(0, len(raw), tile_bytes)
    )

    return Ncgr(
        bit_depth=bit_depth,
        width_tiles=width_raw if width_raw > 0 else None,
        height_tiles=height_raw if height_raw > 0 else None,
        scanned=scanned,
        tiles=tiles,
    )


def parse_nscr(path: Path) -> Nscr:
    data = path.read_bytes()
    if data[:4] != b"RCSN":
        raise ValueError(f"{path}: not an NSCR file")

    off = find_block(data, b"NRCS")
    if off + 0x14 > len(data):
        raise ValueError(f"{path}: truncated NRCS block")

    width_px = u16(data, off + 0x08)
    height_px = u16(data, off + 0x0A)
    bit_depth = 4 if data[off + 0x0C] == 0 else 8
    data_size = u32(data, off + 0x10)
    screen_off = off + 0x14

    if width_px % 8 or height_px % 8:
        raise ValueError(f"{path}: NSCR dimensions are not tile-aligned")
    if data_size % 2:
        raise ValueError(f"{path}: NSCR tilemap has an odd byte count")
    if screen_off + data_size > len(data):
        raise ValueError(f"{path}: NSCR tilemap extends past end of file")

    width_tiles = width_px // 8
    height_tiles = height_px // 8
    expected_entries = width_tiles * height_tiles
    entries = tuple(
        u16(data, screen_off + i)
        for i in range(0, data_size, 2)
    )

    if len(entries) < expected_entries:
        raise ValueError(
            f"{path}: NSCR has {len(entries)} entries; expected at least {expected_entries}"
        )

    return Nscr(
        bit_depth=bit_depth,
        width_tiles=width_tiles,
        height_tiles=height_tiles,
        entries=entries[:expected_entries],
    )


def palette_color(nclr: Nclr, bit_depth: int, palette_bank: int, pixel: int) -> tuple[int, int, int]:
    if bit_depth == 4:
        color_index = palette_bank * 16 + pixel
    else:
        color_index = pixel

    if color_index >= len(nclr.colors):
        # Make an invalid palette reference immediately visible rather than silently
        # substituting black, which could hide a bad member pairing.
        return (255, 0, 255)
    return nclr.colors[color_index]


def draw_tile(
    rgb: bytearray,
    image_width: int,
    tile: tuple[int, ...],
    dst_x: int,
    dst_y: int,
    nclr: Nclr,
    bit_depth: int,
    palette_bank: int,
    hflip: bool = False,
    vflip: bool = False,
) -> None:
    for y in range(8):
        src_y = 7 - y if vflip else y
        for x in range(8):
            src_x = 7 - x if hflip else x
            pixel = tile[src_y * 8 + src_x]
            red, green, blue = palette_color(nclr, bit_depth, palette_bank, pixel)
            pos = ((dst_y + y) * image_width + dst_x + x) * 3
            rgb[pos : pos + 3] = bytes((red, green, blue))


def render_screen(ncgr: Ncgr, nclr: Nclr, nscr: Nscr) -> tuple[int, int, bytes]:
    if ncgr.scanned:
        raise ValueError("Scanned NCGRs cannot be paired with an NSCR tilemap")
    if ncgr.bit_depth != nscr.bit_depth:
        raise ValueError(
            f"NCGR is {ncgr.bit_depth}bpp but NSCR declares {nscr.bit_depth}bpp"
        )

    width = nscr.width_tiles * 8
    height = nscr.height_tiles * 8
    rgb = bytearray(width * height * 3)

    for map_index, entry in enumerate(nscr.entries):
        tile_index = entry & 0x03FF
        hflip = bool(entry & 0x0400)
        vflip = bool(entry & 0x0800)
        palette_bank = (entry >> 12) & 0x0F if nscr.bit_depth == 4 else 0

        if tile_index >= len(ncgr.tiles):
            raise ValueError(
                f"NSCR entry {map_index} references tile {tile_index}, "
                f"but NCGR has only {len(ncgr.tiles)} tiles"
            )

        tile_x = map_index % nscr.width_tiles
        tile_y = map_index // nscr.width_tiles
        draw_tile(
            rgb,
            width,
            ncgr.tiles[tile_index],
            tile_x * 8,
            tile_y * 8,
            nclr,
            ncgr.bit_depth,
            palette_bank,
            hflip,
            vflip,
        )

    return width, height, bytes(rgb)


def render_tilesheet(
    ncgr: Ncgr,
    nclr: Nclr,
    width_tiles: int | None,
    palette_index: int,
) -> tuple[int, int, bytes]:
    if ncgr.scanned:
        raise ValueError(
            "Scanned NCGR layout is not supported by tile-sheet preview yet; "
            "use a matching NSCR or inspect the member separately"
        )

    if width_tiles is None:
        width_tiles = ncgr.width_tiles
    if width_tiles is None or width_tiles <= 0:
        width_tiles = max(1, math.ceil(math.sqrt(len(ncgr.tiles))))

    height_tiles = max(1, math.ceil(len(ncgr.tiles) / width_tiles))
    width = width_tiles * 8
    height = height_tiles * 8
    rgb = bytearray(width * height * 3)

    for tile_index, tile in enumerate(ncgr.tiles):
        tile_x = tile_index % width_tiles
        tile_y = tile_index // width_tiles
        draw_tile(
            rgb,
            width,
            tile,
            tile_x * 8,
            tile_y * 8,
            nclr,
            ncgr.bit_depth,
            palette_index if ncgr.bit_depth == 4 else 0,
        )

    return width, height, bytes(rgb)


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + kind
        + payload
        + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)
    )


def write_rgb_png(path: Path, width: int, height: int, rgb: bytes) -> None:
    if len(rgb) != width * height * 3:
        raise ValueError("RGB buffer length does not match PNG dimensions")

    rows = bytearray()
    stride = width * 3
    for y in range(height):
        rows.append(0)  # PNG filter type: None
        start = y * stride
        rows.extend(rgb[start : start + stride])

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    png = (
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", ihdr)
        + png_chunk(b"IDAT", zlib.compress(bytes(rows), 9))
        + png_chunk(b"IEND", b"")
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Render decoded HGSS Nitro NCGR/NCLR/NSCR assets to PNG"
    )
    parser.add_argument("--ncgr", type=Path, required=True, help="Decoded NCGR file")
    parser.add_argument("--nclr", type=Path, required=True, help="Decoded NCLR file")
    parser.add_argument("--nscr", type=Path, help="Optional NSCR tilemap for exact screen reconstruction")
    parser.add_argument("--output", type=Path, required=True, help="PNG output path")
    parser.add_argument(
        "--width-tiles",
        type=int,
        help="Tile-sheet width when no NSCR is supplied; defaults to NCGR metadata",
    )
    parser.add_argument(
        "--palette-index",
        type=int,
        default=0,
        help="4bpp palette bank for a raw NCGR tile-sheet preview (default: 0)",
    )
    args = parser.parse_args()

    if args.width_tiles is not None and args.width_tiles <= 0:
        parser.error("--width-tiles must be positive")
    if args.palette_index < 0 or args.palette_index > 15:
        parser.error("--palette-index must be in the range 0..15")

    ncgr = parse_ncgr(args.ncgr)
    nclr = parse_nclr(args.nclr)

    if args.nscr:
        nscr = parse_nscr(args.nscr)
        width, height, rgb = render_screen(ncgr, nclr, nscr)
        mode = f"screen {nscr.width_tiles}x{nscr.height_tiles} tiles"
    else:
        width, height, rgb = render_tilesheet(
            ncgr,
            nclr,
            args.width_tiles,
            args.palette_index,
        )
        mode = f"tile sheet ({len(ncgr.tiles)} tiles)"

    write_rgb_png(args.output, width, height, rgb)

    print(
        f"Rendered {args.output}: {width}x{height}, {ncgr.bit_depth}bpp, "
        f"{mode}, {len(nclr.colors)} palette colors"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
