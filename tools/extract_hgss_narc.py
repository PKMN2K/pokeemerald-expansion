#!/usr/bin/env python3
"""Extract NARC members without altering source data.

Only use archives obtained from a lawful local HGSS dump. This tool does not
download ROMs or certify the artistic provenance of member contents.

Usage: python3 tools/extract_hgss_narc.py archive.narc output_directory
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

from inspect_hgss_narc import inspect


def extract(archive_path, output_dir):
    inventory = inspect(archive_path)  # structural verification first
    raw = archive_path.read_bytes()
    cursor = struct.unpack_from("<H", raw, 12)[0]
    blocks = {}
    for _ in range(struct.unpack_from("<H", raw, 14)[0]):
        size = struct.unpack_from("<I", raw, cursor + 4)[0]
        blocks[raw[cursor:cursor + 4]] = cursor + 8
        cursor += size
    payload = blocks[b"GMIF"]
    output_dir.mkdir(parents=True, exist_ok=True)
    if any(output_dir.iterdir()):
        raise ValueError("destination must be empty (will not overwrite files)")
    manifest = {
        "source_archive": archive_path.name,
        "archive_sha256": hashlib.sha256(raw).hexdigest(),
        "origin": "local source archive; external provenance not yet verified",
        "members": [],
    }
    for member in inventory["members"]:
        start = payload + member["offset"]
        data = raw[start:start + member["size"]]
        name = f"member_{member['index']:04d}.bin"
        (output_dir / name).write_bytes(data)
        manifest["members"].append({
            "file": name,
            "source_index": member["index"],
            "size": len(data),
            "format_hint": member["format_hint"],
            "sha256": hashlib.sha256(data).hexdigest(),
        })
    (output_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return len(manifest["members"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    try:
        count = extract(args.archive, args.destination)
        print(f"Extracted {count} NARC members with SHA-256 manifest")
    except (OSError, ValueError, struct.error) as error:
        print(f"Extraction failed: {error}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
