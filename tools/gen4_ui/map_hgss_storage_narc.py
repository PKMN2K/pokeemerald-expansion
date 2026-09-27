#!/usr/bin/env python3
"""
Map the authentic HGSS PC Box graphics archive (/a/0/1/9).

Usage:
    python tools/gen4_ui/map_hgss_storage_narc.py path/to/a_0_1_9.narc
    python tools/gen4_ui/map_hgss_storage_narc.py path/to/a_0_1_9.narc --extract out_dir

The script does not contain or download Nintendo assets. It operates only on a
NARC extracted from a user-provided HeartGold/SoulSilver ROM.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from dataclasses import dataclass
from pathlib import Path


NITRO_MAGICS = {
    b"RGCN": "NCGR",  # character/tile graphics
    b"RLCN": "NCLR",  # palette
    b"RCSN": "NSCR",  # screen/tilemap
    b"RECN": "NCER",  # cell/OAM layout
    b"RNAN": "NANR",  # animation
    b"RTFN": "NFTR",  # font
}


@dataclass(frozen=True)
class NarcMember:
    index: int
    start: int
    end: int
    data: bytes


def u16(data: bytes, off: int) -> int:
    return struct.unpack_from("<H", data, off)[0]


def u32(data: bytes, off: int) -> int:
    return struct.unpack_from("<I", data, off)[0]


def parse_narc(blob: bytes) -> list[NarcMember]:
    if len(blob) < 0x10 or blob[:4] != b"NARC":
        raise ValueError("Input is not a Nintendo NARC archive")

    header_size = u16(blob, 0x0C)
    block_count = u16(blob, 0x0E)
    pos = header_size

    fat_entries: list[tuple[int, int]] | None = None
    gmif_payload: bytes | None = None

    for _ in range(block_count):
        if pos + 8 > len(blob):
            raise ValueError("Truncated NARC block header")
        magic = blob[pos : pos + 4]
        block_size = u32(blob, pos + 4)
        if block_size < 8 or pos + block_size > len(blob):
            raise ValueError("Invalid NARC block size")

        payload = blob[pos + 8 : pos + block_size]

        if magic == b"BTAF":
            if len(payload) < 4:
                raise ValueError("Truncated BTAF block")
            file_count = u16(payload, 0)
            entries_off = 4
            if entries_off + file_count * 8 > len(payload):
                raise ValueError("Truncated BTAF file table")
            fat_entries = [
                (u32(payload, entries_off + i * 8), u32(payload, entries_off + i * 8 + 4))
                for i in range(file_count)
            ]
        elif magic == b"GMIF":
            gmif_payload = payload

        pos += block_size

    if fat_entries is None:
        raise ValueError("NARC has no BTAF file allocation table")
    if gmif_payload is None:
        raise ValueError("NARC has no GMIF file image block")

    members: list[NarcMember] = []
    for index, (start, end) in enumerate(fat_entries):
        if start > end or end > len(gmif_payload):
            raise ValueError(f"Invalid member {index} range {start:#x}..{end:#x}")
        members.append(NarcMember(index, start, end, gmif_payload[start:end]))
    return members


def lz10_decompress(src: bytes) -> bytes:
    if len(src) < 4 or src[0] != 0x10:
        raise ValueError("Not LZ10 data")

    out_size = src[1] | (src[2] << 8) | (src[3] << 16)
    src_pos = 4
    out = bytearray()

    while len(out) < out_size:
        if src_pos >= len(src):
            raise ValueError("Truncated LZ10 flag byte")
        flags = src[src_pos]
        src_pos += 1

        for bit in range(7, -1, -1):
            if len(out) >= out_size:
                break

            if not (flags & (1 << bit)):
                if src_pos >= len(src):
                    raise ValueError("Truncated LZ10 literal")
                out.append(src[src_pos])
                src_pos += 1
                continue

            if src_pos + 1 >= len(src):
                raise ValueError("Truncated LZ10 back-reference")
            first = src[src_pos]
            second = src[src_pos + 1]
            src_pos += 2

            length = (first >> 4) + 3
            disp = ((first & 0x0F) << 8) | second
            copy_pos = len(out) - disp - 1
            if copy_pos < 0:
                raise ValueError("Invalid LZ10 back-reference")

            for _ in range(length):
                if len(out) >= out_size:
                    break
                out.append(out[copy_pos])
                copy_pos += 1

    return bytes(out)


def maybe_decompress(data: bytes) -> tuple[bytes, str]:
    if not data:
        return data, "none"
    if data[0] == 0x10:
        try:
            return lz10_decompress(data), "LZ10"
        except ValueError:
            return data, "unknown-0x10"
    if data[0] == 0x11:
        # LZ11 is used by some DS titles, but this archive is documented as LZ
        # compressed and HGSS UI resources are commonly LZ10. Preserve an LZ11
        # member losslessly and flag it for a later decoder instead of guessing.
        return data, "LZ11"
    return data, "none"


def classify(data: bytes) -> str:
    if len(data) >= 4 and data[:4] in NITRO_MAGICS:
        return NITRO_MAGICS[data[:4]]
    if len(data) >= 4 and data[:4] == b"NARC":
        return "NARC"
    if not data:
        return "EMPTY"
    return "BIN"


def member_record(member: NarcMember) -> tuple[dict[str, object], bytes]:
    decoded, compression = maybe_decompress(member.data)
    kind = classify(decoded)

    rec: dict[str, object] = {
        "index": member.index,
        "archive_offset_start": member.start,
        "archive_offset_end": member.end,
        "compressed_size": len(member.data),
        "compression": compression,
        "decoded_size": len(decoded),
        "type": kind,
        "sha256_compressed": hashlib.sha256(member.data).hexdigest(),
        "sha256_decoded": hashlib.sha256(decoded).hexdigest(),
    }

    if kind in {"NCGR", "NCLR", "NSCR", "NCER", "NANR", "NFTR"} and len(decoded) >= 0x10:
        rec["nitro_bom"] = decoded[4:6].hex()
        rec["nitro_version"] = u16(decoded, 6)
        rec["nitro_file_size"] = u32(decoded, 8)
        rec["nitro_header_size"] = u16(decoded, 0x0C)
        rec["nitro_block_count"] = u16(decoded, 0x0E)

    return rec, decoded


def main() -> int:
    parser = argparse.ArgumentParser(description="Map HGSS /a/0/1/9 PC Box graphics members")
    parser.add_argument("narc", type=Path, help="Extracted HGSS /a/0/1/9 NARC")
    parser.add_argument("--extract", type=Path, help="Optional directory for decoded member files")
    parser.add_argument("--json", type=Path, help="Manifest output path (default: beside input NARC)")
    args = parser.parse_args()

    blob = args.narc.read_bytes()
    members = parse_narc(blob)

    records: list[dict[str, object]] = []
    decoded_members: list[tuple[int, str, bytes]] = []

    for member in members:
        rec, decoded = member_record(member)
        records.append(rec)
        decoded_members.append((member.index, str(rec["type"]), decoded))

    manifest = {
        "source_archive": "/a/0/1/9",
        "source_file": str(args.narc),
        "source_sha256": hashlib.sha256(blob).hexdigest(),
        "member_count": len(members),
        "expected_hgss_member_count": 87,
        "count_matches_expected": len(members) == 87,
        "members": records,
    }

    json_path = args.json or args.narc.with_suffix(args.narc.suffix + ".manifest.json")
    json_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    if args.extract:
        args.extract.mkdir(parents=True, exist_ok=True)
        ext_map = {
            "NCGR": ".ncgr",
            "NCLR": ".nclr",
            "NSCR": ".nscr",
            "NCER": ".ncer",
            "NANR": ".nanr",
            "NFTR": ".nftr",
            "NARC": ".narc",
            "BIN": ".bin",
            "EMPTY": ".bin",
        }
        for index, kind, decoded in decoded_members:
            suffix = ext_map.get(kind, ".bin")
            (args.extract / f"{index:03d}_{kind.lower()}{suffix}").write_bytes(decoded)

    print(f"Mapped {len(members)} members from {args.narc}")
    print(f"Manifest: {json_path}")
    if len(members) != 87:
        print("WARNING: HGSS /a/0/1/9 is expected to contain 87 members")
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
