#!/usr/bin/env python3
"""Stage native GBC RGB555 background palettes for the Emerald graphics pipeline.

Reads *raw, uncompressed* 8-byte GBC palettes (4 little-endian RGB555
colors each), not an RGBDS .pal assembly source or PNG palette.
The GBC and GBA RGB555 bit ordering is identical. Preserve the 16-bit
words exactly; expand each 4-color GBC bank to a 16-color GBA bank with
12 unused zero entries. The output is a staging binary, NOT a registered
tileset or a daylight/nighttime palette mapping.

Usage:
  python3 tools/polished_port/scripts/convert_gbc_palette.py SOURCE.bin OUTPUT.bin
  python3 tools/polished_port/scripts/convert_gbc_palette.py --self-test
"""
import argparse
from pathlib import Path
import struct


def expand_palettes(data: bytes) -> bytes:
    if not data or len(data) % 8:
        raise ValueError("input must contain one or more complete 8-byte GBC palettes")
    if len(data) // 8 > 16:
        raise ValueError("GBA 4bpp background tiles support at most 16 palette banks")
    result = bytearray()
    for offset in range(0, len(data), 8):
        words = struct.unpack_from("<4H", data, offset)
        if any(color & 0x8000 for color in words):
            raise ValueError("unexpected high bit in RGB555 color")
        result.extend(struct.pack("<16H", *words, *([0] * 12)))
    return bytes(result)


def self_test():
    example = struct.pack("<4H", 0, 0x001F, 0x03E0, 0x7C00)
    result = expand_palettes(example)
    assert len(result) == 32
    assert result[:8] == example
    assert result[8:] == b"\x00" * 24
    try:
        expand_palettes(example[:-1])
    except ValueError:
        pass
    else:
        raise AssertionError("truncated source palette accepted")
    print("PASS: GBC RGB555 bank expansion")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, nargs="?")
    parser.add_argument("output", type=Path, nargs="?")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    if args.source is None or args.output is None:
        parser.error("source and output are required unless --self-test is used")
    result = expand_palettes(args.source.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(result)
    print(f"Staged {len(result) // 32} GBA RGB555 palette banks: {args.output}")
    print("Palette source, time-of-day variants and VRAM tile banks must be verified before registration.")


if __name__ == "__main__":
    main()
