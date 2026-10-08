#!/usr/bin/env python3
"""
Pack a verified HGSS storage role into lossless GBA 4bpp BG assets.

Input comes from bind_hgss_storage_role.py. The original decoded NCGR/NCLR/NSCR
members are read again from the pipeline directory so the GBA data is derived
from Nintendo's indexed 4bpp source, not re-quantized from a preview PNG.

For 4bpp Nitro text backgrounds, the DS NSCR entry layout matches the useful
parts of a GBA text-background entry:
  bits 0..9   tile index
  bit 10      horizontal flip
  bit 11      vertical flip
  bits 12..15 palette bank

This tool therefore remaps only tile indices and palette-bank numbers while
preserving the source tile pixels, flip flags, and BGR555 palette values.

Outputs:
  <output>.tiles.4bpp   packed 32-byte GBA 4bpp tiles
  <output>.tilemap.bin  compact little-endian u16 tilemap rectangle
  <output>.palette.bin  packed BGR555 palette banks (32 bytes each)
  <output>.json         provenance and packing metadata
"""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

import render_hgss_nitro as nitro


def load_json(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise SystemExit(f"Required file is missing: {path}") from exc
    except json.JSONDecodeError as exc:
        raise SystemExit(f"Invalid JSON in {path}: {exc}") from exc


def member_path(root: Path, index: int, kind: str) -> Path:
    suffixes = {
        "NCGR": ".ncgr",
        "NCLR": ".nclr",
        "NSCR": ".nscr",
    }
    path = root / "members" / f"{index:03d}_{kind.lower()}{suffixes[kind]}"
    if not path.exists():
        raise SystemExit(
            f"Decoded {kind} member is missing: {path}\n"
            "Run preview_hgss_storage.py first."
        )
    return path


def read_nclr_bgr555(path: Path) -> bytes:
    data = path.read_bytes()
    if data[:4] not in {b"RLCN", b"RPCN"}:
        raise SystemExit(f"{path}: not an NCLR/NCPR file")

    off = nitro.find_block(data, b"TTLP")
    if off + 0x18 > len(data):
        raise SystemExit(f"{path}: truncated TTLP block")

    palette_size = nitro.u32(data, off + 0x10)
    if palette_size == 0:
        palette_size = nitro.u32(data, off + 4) - 0x18
    palette_off = off + 0x18

    if palette_size % 32:
        raise SystemExit(
            f"{path}: 4bpp palette data is not aligned to 16-color banks "
            f"({palette_size} bytes)"
        )
    if palette_off + palette_size > len(data):
        raise SystemExit(f"{path}: palette data extends past end of file")

    return data[palette_off : palette_off + palette_size]


def encode_4bpp_tile(tile: tuple[int, ...]) -> bytes:
    if len(tile) != 64:
        raise SystemExit("Decoded 4bpp tile does not contain 64 pixels")
    out = bytearray(32)
    for i in range(0, 64, 2):
        lo = tile[i]
        hi = tile[i + 1]
        if lo > 0x0F or hi > 0x0F:
            raise SystemExit("Decoded 4bpp tile contains an index above 15")
        out[i // 2] = lo | (hi << 4)
    return bytes(out)


def resolve_crop(binding: dict, nscr: nitro.Nscr) -> tuple[int, int, int, int]:
    source_width = nscr.width_tiles * 8
    source_height = nscr.height_tiles * 8

    declared_source = binding.get("rendered_source_size")
    if declared_source is not None and declared_source != [source_width, source_height]:
        raise SystemExit(
            "Verified binding source dimensions no longer match the decoded NSCR"
        )

    crop = binding.get("crop")
    if crop is None:
        return 0, 0, source_width, source_height

    if not isinstance(crop, list) or len(crop) != 4:
        raise SystemExit("Binding crop must be null or [x, y, width, height]")

    x, y, width, height = crop
    if any(not isinstance(v, int) for v in crop):
        raise SystemExit("Binding crop values must be integers")
    if x < 0 or y < 0 or width <= 0 or height <= 0:
        raise SystemExit("Binding crop has invalid dimensions")
    if any(v % 8 for v in crop):
        raise SystemExit(
            "Lossless GBA BG packing requires the verified crop to be 8-pixel aligned"
        )
    if x + width > source_width or y + height > source_height:
        raise SystemExit("Binding crop exceeds the decoded NSCR dimensions")

    declared_output = binding.get("output_size")
    if declared_output is not None and declared_output != [width, height]:
        raise SystemExit(
            "Verified binding output dimensions do not match its crop metadata"
        )

    return x, y, width, height


def pack_region(
    ncgr: nitro.Ncgr,
    nscr: nitro.Nscr,
    crop: tuple[int, int, int, int],
):
    if ncgr.bit_depth != 4 or nscr.bit_depth != 4:
        raise SystemExit(
            "Verified storage role is not a 4bpp NCGR/NSCR composition; "
            "this lossless GBA BG path only accepts 4bpp"
        )
    if ncgr.scanned:
        raise SystemExit("Scanned NCGR layout cannot be packed as a GBA text BG")

    x, y, width, height = crop
    left = x // 8
    top = y // 8
    width_tiles = width // 8
    height_tiles = height // 8

    source_entries: list[int] = []
    for row in range(height_tiles):
        start = (top + row) * nscr.width_tiles + left
        end = start + width_tiles
        source_entries.extend(nscr.entries[start:end])

    packed_tiles: list[bytes] = []
    tile_lookup: dict[bytes, int] = {}
    used_palette_banks = sorted({(entry >> 12) & 0x0F for entry in source_entries})
    palette_remap = {
        source_bank: packed_bank
        for packed_bank, source_bank in enumerate(used_palette_banks)
    }

    output_entries: list[int] = []
    for entry in source_entries:
        source_tile_index = entry & 0x03FF
        if source_tile_index >= len(ncgr.tiles):
            raise SystemExit(
                f"NSCR references tile {source_tile_index}, "
                f"but NCGR contains only {len(ncgr.tiles)} tiles"
            )

        packed = encode_4bpp_tile(ncgr.tiles[source_tile_index])
        new_tile_index = tile_lookup.get(packed)
        if new_tile_index is None:
            new_tile_index = len(packed_tiles)
            if new_tile_index >= 1024:
                raise SystemExit("Packed role exceeds the GBA 10-bit tile-index limit")
            tile_lookup[packed] = new_tile_index
            packed_tiles.append(packed)

        flip_bits = entry & 0x0C00
        source_bank = (entry >> 12) & 0x0F
        packed_bank = palette_remap[source_bank]
        output_entries.append(new_tile_index | flip_bits | (packed_bank << 12))

    return (
        width_tiles,
        height_tiles,
        packed_tiles,
        output_entries,
        used_palette_banks,
        palette_remap,
    )


def write_tiles(path: Path, tiles: list[bytes]) -> None:
    with path.open("wb") as f:
        for tile in tiles:
            f.write(tile)


def write_tilemap(path: Path, entries: list[int]) -> None:
    with path.open("wb") as f:
        for entry in entries:
            f.write(struct.pack("<H", entry))


def write_palette(
    path: Path,
    raw_palette: bytes,
    used_palette_banks: list[int],
) -> None:
    bank_count = len(raw_palette) // 32
    with path.open("wb") as f:
        for bank in used_palette_banks:
            if bank >= bank_count:
                raise SystemExit(
                    f"NSCR uses palette bank {bank}, but NCLR has only "
                    f"{bank_count} banks"
                )
            start = bank * 32
            f.write(raw_palette[start : start + 32])


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Pack a verified HGSS storage role into lossless GBA 4bpp BG assets"
    )
    parser.add_argument(
        "pipeline_dir",
        type=Path,
        help="Output directory created by preview_hgss_storage.py",
    )
    parser.add_argument(
        "binding",
        type=Path,
        help="Verified role JSON created by bind_hgss_storage_role.py",
    )
    parser.add_argument(
        "--output",
        required=True,
        type=Path,
        help="Output path prefix, e.g. graphics/gen4_ui/hgss_storage/packed/box_grid",
    )
    args = parser.parse_args()

    manifest = load_json(args.pipeline_dir / "manifest.json")
    binding = load_json(args.binding)

    if manifest.get("archive_sha256") != binding.get("archive_sha256"):
        raise SystemExit(
            "Binding provenance does not match the supplied HGSS pipeline archive"
        )

    members = binding.get("members", {})
    try:
        ncgr_index = int(members["ncgr"]["index"])
        nclr_index = int(members["nclr"]["index"])
        nscr_index = int(members["nscr"]["index"])
    except (KeyError, TypeError, ValueError) as exc:
        raise SystemExit("Binding does not contain valid NCGR/NCLR/NSCR member IDs") from exc

    ncgr_path = member_path(args.pipeline_dir, ncgr_index, "NCGR")
    nclr_path = member_path(args.pipeline_dir, nclr_index, "NCLR")
    nscr_path = member_path(args.pipeline_dir, nscr_index, "NSCR")

    ncgr = nitro.parse_ncgr(ncgr_path)
    nclr = nitro.parse_nclr(nclr_path)
    nscr = nitro.parse_nscr(nscr_path)

    if nclr.bit_depth != 4:
        raise SystemExit("Verified storage palette is not 4bpp")

    crop = resolve_crop(binding, nscr)
    (
        width_tiles,
        height_tiles,
        tiles,
        tilemap,
        used_banks,
        palette_remap,
    ) = pack_region(ncgr, nscr, crop)

    raw_palette = read_nclr_bgr555(nclr_path)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    tiles_path = Path(str(args.output) + ".tiles.4bpp")
    tilemap_path = Path(str(args.output) + ".tilemap.bin")
    palette_path = Path(str(args.output) + ".palette.bin")
    metadata_path = Path(str(args.output) + ".json")

    write_tiles(tiles_path, tiles)
    write_tilemap(tilemap_path, tilemap)
    write_palette(palette_path, raw_palette, used_banks)

    metadata = {
        "role": binding.get("role"),
        "archive_sha256": binding.get("archive_sha256"),
        "members": members,
        "crop": list(crop),
        "width_tiles": width_tiles,
        "height_tiles": height_tiles,
        "tile_count": len(tiles),
        "tile_bytes": len(tiles) * 32,
        "tilemap_entries": len(tilemap),
        "tilemap_bytes": len(tilemap) * 2,
        "source_palette_banks": used_banks,
        "palette_bank_remap": {
            str(source): packed
            for source, packed in palette_remap.items()
        },
        "packed_palette_banks": len(used_banks),
        "palette_bytes": len(used_banks) * 32,
        "tiles_file": tiles_path.name,
        "tilemap_file": tilemap_path.name,
        "palette_file": palette_path.name,
        "packing_rule": (
            "Lossless indexed 4bpp conversion: source NCGR pixels and BGR555 "
            "colors preserved; tile indices and palette banks compacted only."
        ),
    }
    metadata_path.write_text(
        json.dumps(metadata, indent=2) + "\n",
        encoding="utf-8",
    )

    print(
        f"Packed verified HGSS role {metadata['role']!r}: "
        f"{width_tiles}x{height_tiles} tiles, "
        f"{len(tiles)} unique tiles, {len(used_banks)} palette banks"
    )
    print(f"Tiles: {tiles_path}")
    print(f"Tilemap: {tilemap_path}")
    print(f"Palette: {palette_path}")
    print(f"Metadata: {metadata_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
