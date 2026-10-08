#!/usr/bin/env python3
"""Read-only inventory of Nintendo DS NARC files for HGSS asset provenance.

Usage: python3 tools/inspect_hgss_narc.py path/to/archive.narc
Prints member offsets, sizes and probable Nintendo graphics format signatures.
Never writes extracted contents or changes the input file.
"""
import argparse
import json
from pathlib import Path
import struct
import sys

SIGNATURES = {
    b"RGCN": "NCGR graphics",
    b"RLCN": "NCLR palette",
    b"RCSN": "NSCR screen",
    b"RECN": "NCER cell",
    b"RNAN": "NANR animation",
    b"RTFN": "NFTR font",
}


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def inspect(path):
    data = path.read_bytes()
    if len(data) < 16 or data[:4] != b"NARC":
        raise ValueError("not a NARC archive")
    if u16(data, 4) != 0xFFFE:
        raise ValueError("unsupported NARC byte order")
    header_size = u16(data, 12)
    block_count = u16(data, 14)
    if u32(data, 8) != len(data) or header_size < 16 or header_size > len(data):
        raise ValueError("corrupt NARC header/size")
    cursor = header_size
    blocks = {}
    for _ in range(block_count):
        if cursor + 8 > len(data):
            raise ValueError("truncated block header")
        name = data[cursor:cursor + 4]
        length = u32(data, cursor + 4)
        if length < 8 or cursor + length > len(data):
            raise ValueError("invalid block length")
        if name in blocks:
            raise ValueError("duplicate block")
        blocks[name] = (cursor + 8, cursor + length)
        cursor += length
    if cursor != len(data):
        raise ValueError("unexpected trailing bytes")
    if b"BTAF" not in blocks or b"GMIF" not in blocks:
        raise ValueError("missing allocation table or image payload")
    start, end = blocks[b"BTAF"]
    if end - start < 4:
        raise ValueError("truncated allocation table")
    count = u16(data, start)
    if start + 4 + 8 * count > end:
        raise ValueError("truncated member table")
    payload_start, payload_end = blocks[b"GMIF"]
    payload_length = payload_end - payload_start
    entries = []
    for i in range(count):
        offset, finish = struct.unpack_from("<II", data, start + 4 + i * 8)
        if finish < offset or finish > payload_length:
            raise ValueError(f"member {i}: invalid range")
        begin = payload_start + offset
        signature = data[begin:begin + 4]
        entries.append({
            "index": i,
            "offset": offset,
            "size": finish - offset,
            "signature": signature.hex(),
            "format_hint": SIGNATURES.get(signature, "unknown"),
        })
    return {"archive": str(path), "members": entries, "member_count": len(entries)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("--json", action="store_true", help="print machine-readable inventory")
    args = parser.parse_args()
    result = inspect(args.archive)
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print(f"{result['archive']}: {result['member_count']} members")
        for entry in result["members"]:
            print(f"  {entry['index']:04d}  {entry['size']:>8} bytes  {entry['format_hint']}")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, struct.error) as exc:
        print(f"NARC inspection failed: {exc}", file=sys.stderr)
        sys.exit(1)
