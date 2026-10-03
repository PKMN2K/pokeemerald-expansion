#!/usr/bin/env python3
from pathlib import Path
import hashlib
import struct
import sys


PALETTE = Path("graphics/gen4_ui/hgss_pokegear/verified/trainer_card_sub_palette.NCLR")
FRONT_TILES = Path("graphics/gen4_ui/hgss_pokegear/verified/trainer_card_front_tiles.NCGR")
FRONT_SCREEN = Path("graphics/gen4_ui/hgss_pokegear/verified/trainer_card_front_screen.NSCR")
AVATAR_TILES = Path("graphics/gen4_ui/hgss_pokegear/verified/trainer_card_avatar_tiles.NCGR")
AVATAR_ETHAN = Path("graphics/gen4_ui/hgss_pokegear/verified/trainer_card_avatar_ethan.NSCR")
AVATAR_LYRA = Path("graphics/gen4_ui/hgss_pokegear/verified/trainer_card_avatar_lyra.NSCR")

EXPECTED_GIT_BLOBS = {
    PALETTE: "011286aa1e98ffe659d88ed6e99b7e6c14a65491",
    FRONT_TILES: "fa067c9657aae2f263d20306d00ad3961e043f18",
    FRONT_SCREEN: "8c2367f1b8c7ae0940f35e5173c195d7158cfef7",
    AVATAR_TILES: "5b7f2414b5240fb16134bd1ed6e9a25f40e3cd02",
    AVATAR_ETHAN: "29a3731d668684c9476dca5ed145fe08ff37a506",
    AVATAR_LYRA: "d7eb3d6803ee6d0ab12f30cfea6e5bf5c56d2309",
}

# Retail avatar overlay footprint from ov51_021E6C6C.
RETAIL_AVATAR_X = 21
RETAIL_AVATAR_Y = 5
RETAIL_AVATAR_W = 10
RETAIL_AVATAR_H = 11

# 88x64 GBA CHECK window crop. One tile of retail card chrome is retained
# left of the avatar footprint so the retail avatar center is x=43.5 here,
# matching the existing Match Call trainer sprite center at x=44.
CROP_X = 20
CROP_Y = 6
CROP_W = 11
CROP_H = 8

EXPECTED_SOURCE_INDICES = (16, 31, 32, 33, 34, 38, 39, 40, 42, 43)
EXPECTED_AVATAR_BBOXES = {
    AVATAR_ETHAN: (184, 48, 223, 95),
    AVATAR_LYRA: (184, 51, 223, 119),
}


def git_blob_sha(data):
    return hashlib.sha1(f"blob {len(data)}\0".encode("ascii") + data).hexdigest()


def verified(path):
    data = path.read_bytes()
    actual = git_blob_sha(data)
    expected = EXPECTED_GIT_BLOBS[path]
    if actual != expected:
        raise ValueError(f"{path}: Git blob {actual} != verified {expected}")
    return data


def u16(data, off):
    return struct.unpack_from("<H", data, off)[0]


def u32(data, off):
    return struct.unpack_from("<I", data, off)[0]


def parse_nclr(path):
    data = verified(path)
    if data[:4] != b"RLCN":
        raise ValueError(f"{path}: not NCLR")
    chunk = data.find(b"TTLP")
    if chunk < 0:
        raise ValueError(f"{path}: missing PLTT")
    size = u32(data, chunk + 0x10)
    offset = u32(data, chunk + 0x14)
    start = chunk + 8 + offset
    raw = data[start:start + size]
    if len(raw) != 512:
        raise ValueError(f"{path}: expected 256 BGR555 colors")
    return struct.unpack("<256H", raw)


def parse_ncgr(path):
    data = verified(path)
    if data[:4] != b"RGCN":
        raise ValueError(f"{path}: not NCGR")
    chunk = data.find(b"RAHC")
    if chunk < 0:
        raise ValueError(f"{path}: missing CHAR")
    bitdepth = u32(data, chunk + 0x0C)
    size = u32(data, chunk + 0x18)
    offset = u32(data, chunk + 0x1C)
    start = chunk + 8 + offset
    raw = data[start:start + size]
    if bitdepth != 4:
        raise ValueError(f"{path}: expected Nitro 8bpp format 4, got {bitdepth}")
    if len(raw) != size or size % 64:
        raise ValueError(f"{path}: malformed 8bpp tile payload")
    return raw


def parse_nscr(path):
    data = verified(path)
    if data[:4] != b"RCSN":
        raise ValueError(f"{path}: not NSCR")
    chunk = data.find(b"NRCS")
    if chunk < 0:
        raise ValueError(f"{path}: missing SCRN")
    width = u16(data, chunk + 8)
    height = u16(data, chunk + 10)
    size = u32(data, chunk + 16)
    start = chunk + 20
    raw = data[start:start + size]
    if len(raw) != (width // 8) * (height // 8) * 2:
        raise ValueError(f"{path}: malformed tilemap payload")
    entries = struct.unpack("<" + "H" * (len(raw) // 2), raw)
    return width, height, entries


def source_pixel(tiles, entry, px, py):
    tile = entry & 0x03FF
    if entry & 0x0400:
        px = 7 - px
    if entry & 0x0800:
        py = 7 - py
    off = tile * 64 + py * 8 + px
    if off >= len(tiles):
        raise ValueError(f"tile {tile} exceeds character data")
    # 8bpp text BG: palette-bank bits in the NSCR entry do not alter pixels.
    return tiles[off]


def verify_avatar_placement():
    tiles = parse_ncgr(AVATAR_TILES)
    for path, expected in EXPECTED_AVATAR_BBOXES.items():
        width, height, entries = parse_nscr(path)
        if (width, height) != (256, 256):
            raise ValueError(f"{path}: expected 256x256 avatar screen")
        cols = width // 8
        xs = []
        ys = []
        for ty in range(RETAIL_AVATAR_Y, RETAIL_AVATAR_Y + RETAIL_AVATAR_H):
            for tx in range(RETAIL_AVATAR_X, RETAIL_AVATAR_X + RETAIL_AVATAR_W):
                entry = entries[ty * cols + tx]
                for py in range(8):
                    for px in range(8):
                        if source_pixel(tiles, entry, px, py) != 0:
                            xs.append(tx * 8 + px)
                            ys.append(ty * 8 + py)
        actual = (min(xs), min(ys), max(xs), max(ys))
        if actual != expected:
            raise ValueError(f"{path}: avatar bbox {actual} != verified {expected}")


def crop_pixels():
    tiles = parse_ncgr(FRONT_TILES)
    width, height, entries = parse_nscr(FRONT_SCREEN)
    if (width, height) != (256, 256):
        raise ValueError(f"{FRONT_SCREEN}: expected 256x256 Trainer Card front")
    cols = width // 8
    pixels = []
    used = set()
    for out_ty in range(CROP_H):
        tile_row = []
        source_ty = CROP_Y + out_ty
        for out_tx in range(CROP_W):
            source_tx = CROP_X + out_tx
            entry = entries[source_ty * cols + source_tx]
            tile = []
            for py in range(8):
                row = []
                for px in range(8):
                    idx = source_pixel(tiles, entry, px, py)
                    row.append(idx)
                    used.add(idx)
                tile.append(row)
            tile_row.append(tile)
        pixels.append(tile_row)
    actual = tuple(sorted(used))
    if actual != EXPECTED_SOURCE_INDICES:
        raise ValueError(
            f"Trainer Card crop palette indices {actual} != verified "
            f"{EXPECTED_SOURCE_INDICES}"
        )
    return pixels


def make_tiles():
    verify_avatar_placement()
    pixels = crop_pixels()
    mapping = {source: dest for dest, source in enumerate(EXPECTED_SOURCE_INDICES)}
    out = bytearray()
    for ty in range(CROP_H):
        for tx in range(CROP_W):
            tile = pixels[ty][tx]
            for py in range(8):
                for px in range(0, 8, 2):
                    lo = mapping[tile[py][px]]
                    hi = mapping[tile[py][px + 1]]
                    out.append(lo | (hi << 4))
    expected = CROP_W * CROP_H * 32
    if len(out) != expected:
        raise ValueError(f"expected {expected} output bytes, got {len(out)}")
    return bytes(out)


def make_palette():
    # The 8bpp source uses only ten colors in the selected crop. GBA 4bpp
    # therefore needs only an index remap; every BGR555 value stays exact.
    colors = parse_nclr(PALETTE)
    out = [colors[i] for i in EXPECTED_SOURCE_INDICES]
    out.extend([0] * (16 - len(out)))
    return struct.pack("<16H", *out)


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("--tiles", "--palette"):
        raise SystemExit(
            "usage: make_hgss_trainer_card_check_portrait.py "
            "(--tiles|--palette) OUTPUT"
        )
    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)
    if sys.argv[1] == "--tiles":
        output.write_bytes(make_tiles())
    else:
        # Validate avatar placement in both modes so every generated artifact
        # is gated by the same retail geometry evidence.
        verify_avatar_placement()
        crop_pixels()
        output.write_bytes(make_palette())


if __name__ == "__main__":
    main()
