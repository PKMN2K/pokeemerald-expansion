#!/usr/bin/env python3
"""Validate an authentic HGSS PNG against a provenance manifest.

Usage:
  python3 tools/validate_hgss_ribbon_asset.py PATH/TO/manifest.json
The source and imported PNG are not modified.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys


def png_info(path):
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n" or len(data) < 33 or data[12:16] != b"IHDR":
        raise ValueError(f"{path}: invalid PNG header")
    width, height, bit_depth, color_type = struct.unpack(">IIBB", data[16:26])
    if width == 0 or height == 0:
        raise ValueError(f"{path}: invalid image size")
    return (width, height, bit_depth, color_type), hashlib.sha256(data).hexdigest()


def main(manifest_path):
    manifest_path = Path(manifest_path).resolve()
    root = manifest_path.parent
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    for required in ("source_game", "source_reference", "source_png", "source_sha256",
                     "imported_png", "imported_sha256", "source_dimensions", "imported_dimensions"):
        if not manifest.get(required):
            raise ValueError(f"missing provenance field: {required}")
    if manifest["source_game"] not in ("HeartGold", "SoulSilver"):
        raise ValueError("source_game must be HeartGold or SoulSilver")
    for prefix in ("source", "imported"):
        name = manifest[f"{prefix}_png"]
        file = (root / name).resolve()
        if not file.is_relative_to(root):
            raise ValueError(f"{prefix}_png escapes manifest directory")
        if not file.is_file():
            raise ValueError(f"missing file: {file}")
        info, digest = png_info(file)
        if digest.lower() != manifest[f"{prefix}_sha256"].lower():
            raise ValueError(f"{prefix} checksum mismatch")
        if list(info[:2]) != manifest[f"{prefix}_dimensions"]:
            raise ValueError(f"{prefix} dimensions mismatch")
        print(f"{prefix}: {info[0]}x{info[1]}, SHA256 verified")
    print("HGSS ribbon PNG provenance verified (visual authenticity still requires review)")


if __name__ == "__main__":
    try:
        if len(sys.argv) != 2:
            raise ValueError("expected exactly one manifest.json argument")
        main(sys.argv[1])
    except (ValueError, OSError, KeyError, json.JSONDecodeError) as error:
        print(f"validation failed: {error}", file=sys.stderr)
        sys.exit(1)
