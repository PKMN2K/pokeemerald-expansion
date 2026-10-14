#!/usr/bin/env python3
"""Convert pinned *unaltered* Johto PNG pixels into source-faithful GBA 4bpp.

Original Polished Crystal data/tilesets.asm uses:
  gfx/tilesets/johto_common.2bpp.lz                    (128 8x8 tiles)
  gfx/tilesets/johto_traditional.johto_common.2bpp.vram0.lz (first 128)
  gfx/tilesets/johto_traditional.johto_common.2bpp.vram1.lz (last 32)

Game Boy Color attributes bit 3 chooses tile VRAM bank, NOT Emerald's
primary/secondary metatile banks. The two source tile graphic sets are:
  GBC VRAM bank 0: common tile numbers 0..127
  GBC VRAM bank 1: Traditional tile numbers 0..159

They fit together in Emerald's 512-tile primary graphics bank with a
single synthetic all-transparent tile at GBA index 0, followed by the
128 shared tiles and 160 Traditional tiles, preserving source order:
  GBA index 0                : transparent-only padding, no original artwork
  GBA index 1..128           : GBC bank0 common source tile 0..127
  GBA index 129..288         : GBC bank1 Traditional tile 0..159
  GBA index 289..511         : transparent padding (unused)
  GBA secondary graphics     : transparent padding (unused; tile refs point
                               to the shared primary graphics, not new art)

The original source art is 2-bit grayscale, using RGBDS -c dmg ordering
white=0, light gray=1, dark gray=2, black=3. GBA 4bpp palette color
index zero is *transparent*, unlike GBC BG index zero, which is opaque.
Therefore map CGB color0 to GBA index4, while colors1..3 map to 1..3.
The runtime composer/staged palettes must duplicate original CGB RGB555
color0 into GBA palette index4. No color is invented or altered.

Only standard-library Python is needed; PNG grayscale/2-bit, noninterlaced
is verified and unfiltered byte-for-byte. Source Git blob SHAs are pinned
and checked, rejecting unreviewed graphics changes.

Usage:
  python3 tools/polished_port/scripts/stage_johto_tile_graphics.py \
    --output /tmp/johto-graphics
"""
import argparse
import base64
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import zlib

FIXTURES = Path(__file__).resolve().parents[1] / "fixtures"
SOURCES = (
    ("johto_common.png.b64", 128, 64,
     "e193d6c5b63126cbd3477e2d94818dd535bbb60a"),
    ("johto_traditional.johto_common.png.b64", 128, 80,
     "c44dc6a67fa8598b60969e7b44b21431bd50fbe5"),
)
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
SOURCE_COMMON_TILES = 128
SOURCE_TRADITIONAL_TILES = 160
SOURCE_TOTAL_TILES = SOURCE_COMMON_TILES + SOURCE_TRADITIONAL_TILES
GBA_TRANSPARENT_TILE_ID = 0
GBA_TILE_BYTES = 32
GBA_PRIMARY_TILE_CAPACITY = 512
GBA_SECONDARY_TILE_CAPACITY = 512
SOURCE_TO_GBA_PIXEL = (3, 2, 1, 4)  # image samples 0,1,2,3 -> GBA indices
#   grayscale 0 (black) -> CGB 3 -> GBA 3
#   grayscale 1 (dark)  -> CGB 2 -> GBA 2
#   grayscale 2 (light) -> CGB 1 -> GBA 1
#   grayscale 3 (white) -> CGB 0 -> GBA 4 (opaque)
# Per GBA palette, index4 duplicates original source palette color0.


def load_pinned_png(filename, expected_sha, fixture_dir=FIXTURES):
    lines = (fixture_dir / filename).read_text(encoding="utf-8").splitlines()
    encoded = "".join(line.strip() for line in lines if line.strip() and
                      not line.lstrip().startswith("#"))
    data = base64.b64decode(encoded, validate=True)
    git_blob = b"blob " + str(len(data)).encode("ascii") + b"\x00" + data
    if hashlib.sha1(git_blob).hexdigest() != expected_sha:
        raise ValueError(f"{filename}: pinned Git source blob SHA does not match")
    return data


def parse_grayscale2_png(data, *, expected_width, expected_height):
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError("not a PNG")
    pos = len(PNG_SIGNATURE)
    width = height = None
    pieces = []
    seen = []
    while pos < len(data):
        if pos + 12 > len(data):
            raise ValueError("truncated PNG chunk header")
        length = struct.unpack_from(">I", data, pos)[0]
        kind = data[pos + 4:pos + 8]
        end = pos + 12 + length
        if end > len(data):
            raise ValueError("truncated PNG chunk")
        payload = data[pos + 8:pos + 8 + length]
        crc = struct.unpack_from(">I", data, pos + 8 + length)[0]
        if zlib.crc32(kind + payload) & 0xFFFFFFFF != crc:
            raise ValueError(f"corrupt PNG CRC on {kind!r}")
        pos = end
        seen.append(kind)
        if kind == b"IHDR":
            if width is not None or length != 13:
                raise ValueError("duplicate or malformed PNG IHDR")
            width, height, bits, color, comp, filt, interlace = struct.unpack(">IIBBBBB", payload)
            if (width, height) != (expected_width, expected_height):
                raise ValueError(f"unexpected authentic PNG dimensions: {width}x{height}")
            if (bits, color, comp, filt, interlace) != (2, 0, 0, 0, 0):
                raise ValueError("expected noninterlaced 2-bit grayscale PNG")
        elif kind == b"IDAT":
            if width is None:
                raise ValueError("IDAT before IHDR")
            pieces.append(payload)
        elif kind == b"IEND":
            if length or pos != len(data):
                raise ValueError("invalid PNG end")
            break
        elif kind[:1].isupper():
            raise ValueError(f"unexpected PNG critical chunk: {kind!r}")
    if not seen or seen[0] != b"IHDR" or seen[-1] != b"IEND" or not pieces:
        raise ValueError("missing PNG IHDR/IDAT/IEND")
    if width % 8 or height % 8 or width % 4:
        raise ValueError("source PNG not aligned to 8x8 tiles")
    row_bytes = width // 4
    raw = zlib.decompress(b"".join(pieces))
    if len(raw) != height * (row_bytes + 1):
        raise ValueError("PNG uncompressed scanline size mismatch")

    # PNG byte-wise unfilter. bpp=1 for a 2-bit grayscale image.
    prior = bytearray(row_bytes)
    rows = []
    ptr = 0
    for y in range(height):
        filt = raw[ptr]
        ptr += 1
        row = bytearray(raw[ptr:ptr + row_bytes])
        ptr += row_bytes
        if filt not in range(5):
            raise ValueError(f"unknown PNG scanline filter {filt}")
        for x in range(row_bytes):
            left = row[x - 1] if x else 0
            up = prior[x]
            upper_left = prior[x - 1] if x else 0
            if filt == 1:
                predictor = left
            elif filt == 2:
                predictor = up
            elif filt == 3:
                predictor = (left + up) // 2
            elif filt == 4:
                p = left + up - upper_left
                distances = (abs(p - left), abs(p - up), abs(p - upper_left))
                predictor = (left, up, upper_left)[distances.index(min(distances))]
            else:
                predictor = 0
            row[x] = (row[x] + predictor) & 0xFF
        prior = row
        rows.append(tuple((row[x // 4] >> (6 - 2 * (x % 4))) & 3
                          for x in range(width)))
    return rows


def gba_tile_id(gbc_tile, gbc_vram_bank):
    """Translate the full original 0..255 GBC ID (with VRAM bank bit)."""
    if gbc_vram_bank == 0 and 0 <= gbc_tile < SOURCE_COMMON_TILES:
        return gbc_tile + 1
    if gbc_vram_bank == 1 and 0 <= gbc_tile < SOURCE_TRADITIONAL_TILES:
        return SOURCE_COMMON_TILES + gbc_tile + 1
    raise ValueError(f"tile absent in authentic source: VRAM{gbc_vram_bank} tile {gbc_tile}")


def encode_gba_tile(rows):
    if len(rows) != 8 or any(len(row) != 8 for row in rows):
        raise ValueError("a GBA 4bpp tile needs exactly 8x8 source pixels")
    out = bytearray()
    for row in rows:
        for col in range(0, 8, 2):
            left, right = row[col:col + 2]
            if not 0 <= left <= 3 or not 0 <= right <= 3:
                raise ValueError("input pixels must be GBC grayscale2 samples")
            out.append(SOURCE_TO_GBA_PIXEL[left] | (SOURCE_TO_GBA_PIXEL[right] << 4))
    assert len(out) == GBA_TILE_BYTES
    return bytes(out)


def png_to_tiles(image_rows):
    if not image_rows or len(image_rows) % 8 or len(image_rows[0]) % 8:
        raise ValueError("source tile grid must be 8x8 aligned")
    width = len(image_rows[0])
    if any(len(row) != width for row in image_rows):
        raise ValueError("inconsistent PNG row widths")
    tiles = []
    for y in range(0, len(image_rows), 8):
        for x in range(0, width, 8):
            tiles.append(encode_gba_tile([row[x:x + 8] for row in image_rows[y:y + 8]]))
    return tiles


def build_graphics(fixture_dir=FIXTURES):
    source_tiles = []
    samples = Counter()
    for filename, width, height, sha in SOURCES:
        rows = parse_grayscale2_png(
            load_pinned_png(filename, sha, fixture_dir),
            expected_width=width, expected_height=height
        )
        samples.update(v for row in rows for v in row)
        source_tiles.append(png_to_tiles(rows))
    common, traditional = source_tiles
    if (len(common), len(traditional)) != (
        SOURCE_COMMON_TILES, SOURCE_TRADITIONAL_TILES
    ):
        raise ValueError("unexpected source graphics tile count")
    # Original source tile indices are 0..127 in VRAM0, 0..159 in VRAM1.
    # Blank tile0 is new *padding*, not a fabricated source graphic.
    source_converted = common + traditional
    primary = b"\x00" * GBA_TILE_BYTES + b"".join(source_converted)
    primary += b"\x00" * (GBA_PRIMARY_TILE_CAPACITY * GBA_TILE_BYTES - len(primary))
    secondary = b"\x00" * (GBA_SECONDARY_TILE_CAPACITY * GBA_TILE_BYTES)
    if len(primary) != 16384 or len(secondary) != 16384:
        raise ValueError("GBA primary/secondary graphics bank capacity mismatch")
    report = {
        "status": "original_png_pixels_converted_staging_not_registered",
        "source_png_blob_sha": {name: sha for name, _, _, sha in SOURCES},
        "source_png_tile_counts": {"common_vram0": len(common),
                                   "traditional_vram1": len(traditional)},
        "original_gbc_grayscale_sample_counts": {str(k): samples[k] for k in range(4)},
        "gba_4bpp_source_tiles": SOURCE_TOTAL_TILES,
        "gba_4bpp_reserved_transparent_tile_id": 0,
        "gba_source_common_tile_ids": [1, 128],
        "gba_source_traditional_tile_ids": [129, 288],
        "gba_primary_tile_count": GBA_PRIMARY_TILE_CAPACITY,
        "gba_secondary_tile_count": GBA_SECONDARY_TILE_CAPACITY,
        "gba_nonpadding_tile_count": SOURCE_TOTAL_TILES,
        "gba_padded_tile_count": 1024 - SOURCE_TOTAL_TILES,
        "gba_source_pixel_to_index": {"0": 3, "1": 2, "2": 1, "3": 4},
        "palette_gba_index_4_must_duplicate_source_color_0": True,
        "original_rgb555_values_modified": False,
        "gba_pixel_zero_present_in_source_tiles": False,
        "upper_emerald_layer_uses_reserved_transparent_tile_0": True,
        "gbc_priority_layers_implemented": False,
        "source_tile_animations_registered": False,
        "emerald_tileset_registered": False,
        "ready_for_live_registration": False,
    }
    return primary, secondary, report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    primary, secondary, report = build_graphics()
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "johto.graphics.primary.staging.4bpp").write_bytes(primary)
    (args.output / "johto.graphics.secondary.staging.4bpp").write_bytes(secondary)
    (args.output / "johto.graphics.review.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    print("Staged 128 authentic common + 160 authentic Traditional GBC tiles as GBA 4bpp.")
    print("GBA graphics tiles 1..288 authentic; tile 0 and unused slots transparent.")
    print("No registered Emerald tileset; source animations and priority still unresolved.")


if __name__ == "__main__":
    main()
