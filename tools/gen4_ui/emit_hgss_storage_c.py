#!/usr/bin/env python3
"""
Emit a pokeemerald C include from a packed, verified HGSS storage role.

This consumes the metadata and binary outputs from
pack_verified_hgss_storage.py. It validates every reported byte count before
embedding the authentic 4bpp tiles, tilemap entries, palette values, and the
HgssStorageBgAsset descriptor.

The generated include is intentionally self-contained so the pokeemerald build
does not depend on a special raw-binary include macro.

Example:
    python tools/gen4_ui/emit_hgss_storage_c.py \
        graphics/gen4_ui/hgss_storage/packed/box_grid.json \
        --symbol HgssStorageBoxGrid \
        --output src/data/hgss_storage_box_grid.inc.h
"""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path


def load_json(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise SystemExit(f"Packed metadata is missing: {path}") from exc
    except json.JSONDecodeError as exc:
        raise SystemExit(f"Invalid JSON in {path}: {exc}") from exc


def require_int(meta: dict, key: str, minimum: int = 0, maximum: int | None = None) -> int:
    value = meta.get(key)
    if not isinstance(value, int):
        raise SystemExit(f"{key!r} must be an integer in packed metadata")
    if value < minimum or (maximum is not None and value > maximum):
        limit = f"{minimum}..{maximum}" if maximum is not None else f">={minimum}"
        raise SystemExit(f"{key!r} must be {limit}, got {value}")
    return value


def read_exact(path: Path, expected: int, label: str) -> bytes:
    try:
        data = path.read_bytes()
    except FileNotFoundError as exc:
        raise SystemExit(f"{label} file is missing: {path}") from exc
    if len(data) != expected:
        raise SystemExit(
            f"{label} size mismatch: metadata says {expected} bytes, "
            f"but {path} contains {len(data)}"
        )
    return data


def c_u8_array(name: str, data: bytes) -> str:
    lines = [f"static const ALIGNED(4) u8 {name}[] = {{"]
    for offset in range(0, len(data), 16):
        chunk = data[offset : offset + 16]
        lines.append("    " + ", ".join(f"0x{value:02X}" for value in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)


def c_u16_array(name: str, data: bytes) -> str:
    if len(data) % 2:
        raise SystemExit(f"{name}: u16 data has odd byte length")
    values = struct.unpack(f"<{len(data) // 2}H", data)
    lines = [f"static const ALIGNED(4) u16 {name}[] = {{"]
    for offset in range(0, len(values), 8):
        chunk = values[offset : offset + 8]
        lines.append("    " + ", ".join(f"0x{value:04X}" for value in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)


def validate_role(meta: dict, expected_role: str | None) -> str:
    role = meta.get("role")
    if not isinstance(role, str) or not role:
        raise SystemExit("Packed metadata does not contain a valid role")
    if expected_role is not None and role != expected_role:
        raise SystemExit(
            f"Packed role is {role!r}, but --require-role requested {expected_role!r}"
        )
    return role


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Emit a C include from packed verified HGSS storage assets"
    )
    parser.add_argument("metadata", type=Path, help="Packed role JSON")
    parser.add_argument(
        "--symbol",
        default="HgssStorageBoxGrid",
        help="C symbol stem; default: HgssStorageBoxGrid",
    )
    parser.add_argument(
        "--require-role",
        default="box_grid",
        help="Reject metadata for a different semantic role; default: box_grid",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("src/data/hgss_storage_box_grid.inc.h"),
        help="Generated C include path",
    )
    args = parser.parse_args()

    meta = load_json(args.metadata)
    role = validate_role(meta, args.require_role)

    width_tiles = require_int(meta, "width_tiles", 1, 32)
    height_tiles = require_int(meta, "height_tiles", 1, 32)
    tile_count = require_int(meta, "tile_count", 1, 512)
    palette_bank_count = require_int(meta, "packed_palette_banks", 1, 13)

    tile_bytes = require_int(meta, "tile_bytes", 32)
    tilemap_bytes = require_int(meta, "tilemap_bytes", 2)
    palette_bytes = require_int(meta, "palette_bytes", 32)

    if tile_bytes != tile_count * 32:
        raise SystemExit(
            f"tile_bytes {tile_bytes} does not equal tile_count*32 "
            f"({tile_count * 32})"
        )
    if tilemap_bytes != width_tiles * height_tiles * 2:
        raise SystemExit(
            f"tilemap_bytes {tilemap_bytes} does not equal "
            f"width_tiles*height_tiles*2 ({width_tiles * height_tiles * 2})"
        )
    if palette_bytes != palette_bank_count * 32:
        raise SystemExit(
            f"palette_bytes {palette_bytes} does not equal "
            f"palette banks*32 ({palette_bank_count * 32})"
        )

    root = args.metadata.parent
    try:
        tiles_name = meta["tiles_file"]
        tilemap_name = meta["tilemap_file"]
        palette_name = meta["palette_file"]
    except KeyError as exc:
        raise SystemExit(f"Packed metadata is missing file field: {exc}") from exc

    if not all(isinstance(value, str) and value for value in (tiles_name, tilemap_name, palette_name)):
        raise SystemExit("Packed output filenames must be non-empty strings")

    tiles = read_exact(root / tiles_name, tile_bytes, "Tile")
    tilemap = read_exact(root / tilemap_name, tilemap_bytes, "Tilemap")
    palette = read_exact(root / palette_name, palette_bytes, "Palette")

    stem = args.symbol
    tiles_symbol = f"s{stem}Tiles"
    tilemap_symbol = f"s{stem}Tilemap"
    palette_symbol = f"s{stem}Palette"
    asset_symbol = f"s{stem}Asset"

    archive_sha = meta.get("archive_sha256", "unknown")
    members = meta.get("members", {})
    member_comment = []
    for kind in ("ncgr", "nclr", "nscr"):
        try:
            member_comment.append(f"{kind.upper()} {members[kind]['index']}")
        except (KeyError, TypeError):
            member_comment.append(f"{kind.upper()} ?")

    output = [
        "/*",
        " * AUTO-GENERATED by tools/gen4_ui/emit_hgss_storage_c.py.",
        " * Do not hand-edit counts, dimensions, or embedded bytes.",
        f" * Verified HGSS role: {role}",
        f" * Source archive SHA-256: {archive_sha}",
        " * Members: " + ", ".join(member_comment),
        " */",
        "",
        c_u8_array(tiles_symbol, tiles),
        "",
        c_u16_array(tilemap_symbol, tilemap),
        "",
        c_u16_array(palette_symbol, palette),
        "",
        f"static const struct HgssStorageBgAsset {asset_symbol} =",
        "{",
        f"    .tiles = {tiles_symbol},",
        f"    .tilemap = {tilemap_symbol},",
        f"    .palette = {palette_symbol},",
        f"    .tileCount = {tile_count},",
        f"    .widthTiles = {width_tiles},",
        f"    .heightTiles = {height_tiles},",
        f"    .paletteBankCount = {palette_bank_count},",
        "};",
        "",
    ]

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(output), encoding="utf-8")

    print(
        f"Emitted {asset_symbol}: {tile_count} tiles, "
        f"{width_tiles}x{height_tiles} tilemap, "
        f"{palette_bank_count} palette banks"
    )
    print(f"Output: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
