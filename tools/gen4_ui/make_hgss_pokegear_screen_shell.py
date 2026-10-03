#!/usr/bin/env python3
from pathlib import Path
import struct
import sys


PALETTE = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_palette.NCLR")
TILEMAP = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_tilemap.NSCR")

TILE_BASE = 0x40
PALETTE_BANK = 13
SOURCE_WIDTH_TILES = 32
SOURCE_HEIGHT_TILES = 24
OUTPUT_WIDTH_TILES = 30
OUTPUT_HEIGHT_TILES = 20
SOURCE_ROWS = tuple(range(0, 12)) + tuple(range(16, 24))


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


def make_tilemap():
    width, height, entries = load_nscr(TILEMAP)
    if (width, height) != (256, 192):
        raise ValueError(
            f"{TILEMAP}: expected 256x192, got {width}x{height}"
        )

    if len(entries) != SOURCE_WIDTH_TILES * SOURCE_HEIGHT_TILES:
        raise ValueError(f"{TILEMAP}: unexpected entry count {len(entries)}")

    # Retail rows 12..15 are one uniform 32-pixel spacer. Refuse to adapt if
    # that invariant changes, so this never discards artwork accidentally.
    spacer = entries[12 * SOURCE_WIDTH_TILES]
    for y in range(12, 16):
        for x in range(SOURCE_WIDTH_TILES):
            if entries[y * SOURCE_WIDTH_TILES + x] != spacer:
                raise ValueError(
                    f"{TILEMAP}: rows 12..15 are no longer a uniform spacer"
                )

    out = []
    for source_y in SOURCE_ROWS:
        row = entries[
            source_y * SOURCE_WIDTH_TILES:
            (source_y + 1) * SOURCE_WIDTH_TILES
        ]

        # Keep the centered 240-pixel viewport: source columns 1..30.
        for entry in row[1:31]:
            palette_bank = (entry >> 12) & 0xF
            if palette_bank != PALETTE_BANK:
                raise ValueError(
                    f"{TILEMAP}: expected palette bank {PALETTE_BANK}, "
                    f"got {palette_bank}"
                )

            tile = entry & 0x03FF
            mapped_tile = tile + TILE_BASE
            if mapped_tile > 0x03FF:
                raise ValueError("mapped tile exceeds GBA text-BG tile range")

            # DS and GBA text-screen entries use the same tile/flip/palette
            # bit layout. Keep bits 10..15 exactly and offset only tile ID.
            out.append((entry & 0xFC00) | mapped_tile)

        # GBA screen blocks are 32 tiles wide; the last two columns are outside
        # the 240-pixel display and stay blank.
        out.extend((0, 0))

    if len(out) != 32 * OUTPUT_HEIGHT_TILES:
        raise ValueError(f"unexpected output entry count {len(out)}")

    return struct.pack("<" + "H" * len(out), *out)


def make_palette():
    colors = load_nclr(PALETTE)
    start = PALETTE_BANK * 16
    bank = colors[start:start + 16]
    if len(bank) != 16:
        raise ValueError(
            f"{PALETTE}: missing palette bank {PALETTE_BANK}"
        )
    return struct.pack("<16H", *bank)


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("--tilemap", "--palette"):
        raise SystemExit(
            "usage: make_hgss_pokegear_screen_shell.py "
            "(--tilemap|--palette) OUTPUT"
        )

    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)

    if sys.argv[1] == "--tilemap":
        output.write_bytes(make_tilemap())
    else:
        output.write_bytes(make_palette())


if __name__ == "__main__":
    main()
