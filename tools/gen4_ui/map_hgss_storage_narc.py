#!/usr/bin/env python3
"""
Map the authentic HGSS PC Box graphics archive (/a/0/1/9).

Usage:
    python tools/gen4_ui/map_hgss_storage_narc.py path/to/heartgold.nds
    python tools/gen4_ui/map_hgss_storage_narc.py path/to/soulsilver.nds --extract out_dir
    python tools/gen4_ui/map_hgss_storage_narc.py path/to/a_0_1_9.narc

The script does not contain or download Nintendo assets. It operates only on a
user-provided HeartGold/SoulSilver ROM or an already extracted /a/0/1/9 NARC.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from dataclasses import dataclass
from pathlib import Path


HGSS_STORAGE_PATH = "a/0/1/9"

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


@dataclass(frozen=True)
class InputArchive:
    data: bytes
    source_kind: str
    source_path: str
    rom_title: str | None = None
    rom_game_code: str | None = None
    rom_file_id: int | None = None
    rom_offset_start: int | None = None
    rom_offset_end: int | None = None


def u16(data: bytes, off: int) -> int:
    return struct.unpack_from("<H", data, off)[0]


def u32(data: bytes, off: int) -> int:
    return struct.unpack_from("<I", data, off)[0]


def parse_nds_fnt(rom: bytes) -> dict[str, int]:
    """Return Nintendo DS ROM filesystem paths mapped to FAT file IDs."""
    if len(rom) < 0x200:
        raise ValueError("Input is too small to be a Nintendo DS ROM")

    fnt_off = u32(rom, 0x40)
    fnt_size = u32(rom, 0x44)
    fat_off = u32(rom, 0x48)
    fat_size = u32(rom, 0x4C)

    if not fnt_size or fnt_off + fnt_size > len(rom):
        raise ValueError("ROM has an invalid Nintendo DS file-name table")
    if not fat_size or fat_size % 8 or fat_off + fat_size > len(rom):
        raise ValueError("ROM has an invalid Nintendo DS file-allocation table")

    fnt = rom[fnt_off : fnt_off + fnt_size]
    if len(fnt) < 8:
        raise ValueError("Nintendo DS file-name table is truncated")

    dir_count = u16(fnt, 6)
    if dir_count == 0 or dir_count * 8 > len(fnt):
        raise ValueError("Nintendo DS file-name table has an invalid directory count")

    directories: dict[int, tuple[int, int]] = {}
    for index in range(dir_count):
        off = index * 8
        subtable_off = u32(fnt, off)
        first_file_id = u16(fnt, off + 4)
        directory_id = 0xF000 + index
        if subtable_off >= len(fnt):
            raise ValueError(f"Directory {directory_id:#06x} has an invalid FNT subtable offset")
        directories[directory_id] = (subtable_off, first_file_id)

    paths: dict[str, int] = {}
    visited: set[int] = set()

    def walk(directory_id: int, prefix: str) -> None:
        if directory_id in visited:
            raise ValueError("Nintendo DS file-name table contains a directory cycle")
        if directory_id not in directories:
            raise ValueError(f"FNT references unknown directory {directory_id:#06x}")

        visited.add(directory_id)
        pos, file_id = directories[directory_id]

        while True:
            if pos >= len(fnt):
                raise ValueError("Nintendo DS FNT directory subtable is truncated")

            length = fnt[pos]
            pos += 1
            if length == 0:
                break

            is_directory = bool(length & 0x80)
            name_len = length & 0x7F
            if name_len == 0 or pos + name_len > len(fnt):
                raise ValueError("Nintendo DS FNT contains an invalid name entry")

            raw_name = fnt[pos : pos + name_len]
            pos += name_len
            try:
                name = raw_name.decode("ascii")
            except UnicodeDecodeError as exc:
                raise ValueError("Nintendo DS FNT contains a non-ASCII path component") from exc

            child_path = f"{prefix}/{name}" if prefix else name

            if is_directory:
                if pos + 2 > len(fnt):
                    raise ValueError("Nintendo DS FNT directory entry is truncated")
                child_id = u16(fnt, pos)
                pos += 2
                walk(child_id, child_path)
            else:
                paths[child_path] = file_id
                file_id += 1

        visited.remove(directory_id)

    walk(0xF000, "")
    return paths


def extract_nds_file(rom: bytes, path: str) -> tuple[bytes, int, int, int]:
    paths = parse_nds_fnt(rom)
    if path not in paths:
        raise ValueError(f"Nintendo DS ROM does not contain /{path}")

    file_id = paths[path]
    fat_off = u32(rom, 0x48)
    fat_size = u32(rom, 0x4C)
    file_count = fat_size // 8

    if file_id >= file_count:
        raise ValueError(f"FNT file id {file_id} is outside the FAT")

    start = u32(rom, fat_off + file_id * 8)
    end = u32(rom, fat_off + file_id * 8 + 4)
    if start > end or end > len(rom):
        raise ValueError(f"FAT entry for /{path} points outside the ROM")

    return rom[start:end], file_id, start, end


def read_input_archive(path: Path) -> InputArchive:
    blob = path.read_bytes()

    if blob[:4] == b"NARC":
        return InputArchive(
            data=blob,
            source_kind="extracted_narc",
            source_path=str(path),
        )

    archive, file_id, start, end = extract_nds_file(blob, HGSS_STORAGE_PATH)
    if archive[:4] != b"NARC":
        raise ValueError(
            f"/{HGSS_STORAGE_PATH} was found, but it is not a NARC archive "
            f"(magic={archive[:4]!r})"
        )

    title = blob[:12].rstrip(b"\0 ").decode("ascii", errors="replace")
    game_code = blob[0x0C:0x10].decode("ascii", errors="replace")

    return InputArchive(
        data=archive,
        source_kind="nds_rom",
        source_path=str(path),
        rom_title=title,
        rom_game_code=game_code,
        rom_file_id=file_id,
        rom_offset_start=start,
        rom_offset_end=end,
    )


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
        # Preserve LZ11 losslessly and flag it rather than guessing at decoding.
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
    parser = argparse.ArgumentParser(
        description="Map authentic HGSS /a/0/1/9 PC Box graphics from an NDS ROM or NARC"
    )
    parser.add_argument("input", type=Path, help="HeartGold/SoulSilver .nds or extracted /a/0/1/9 NARC")
    parser.add_argument("--extract", type=Path, help="Optional directory for decoded member files")
    parser.add_argument("--json", type=Path, help="Manifest output path (default: beside input)")
    parser.add_argument("--save-narc", type=Path, help="Optional path to save /a/0/1/9 when input is a ROM")
    args = parser.parse_args()

    source = read_input_archive(args.input)
    blob = source.data

    if args.save_narc:
        args.save_narc.parent.mkdir(parents=True, exist_ok=True)
        args.save_narc.write_bytes(blob)

    members = parse_narc(blob)

    records: list[dict[str, object]] = []
    decoded_members: list[tuple[int, str, bytes]] = []

    for member in members:
        rec, decoded = member_record(member)
        records.append(rec)
        decoded_members.append((member.index, str(rec["type"]), decoded))

    manifest: dict[str, object] = {
        "source_archive": f"/{HGSS_STORAGE_PATH}",
        "source_kind": source.source_kind,
        "source_file": source.source_path,
        "archive_sha256": hashlib.sha256(blob).hexdigest(),
        "member_count": len(members),
        "expected_hgss_member_count": 87,
        "count_matches_expected": len(members) == 87,
        "members": records,
    }

    if source.source_kind == "nds_rom":
        manifest["rom_title"] = source.rom_title
        manifest["rom_game_code"] = source.rom_game_code
        manifest["rom_file_id"] = source.rom_file_id
        manifest["rom_archive_offset_start"] = source.rom_offset_start
        manifest["rom_archive_offset_end"] = source.rom_offset_end

    json_path = args.json or args.input.with_suffix(args.input.suffix + ".manifest.json")
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

    if source.source_kind == "nds_rom":
        print(
            f"Extracted /{HGSS_STORAGE_PATH} from {args.input} "
            f"(file id {source.rom_file_id}, game code {source.rom_game_code})"
        )
    print(f"Mapped {len(members)} members from /{HGSS_STORAGE_PATH}")
    print(f"Manifest: {json_path}")

    if len(members) != 87:
        print("WARNING: HGSS /a/0/1/9 is expected to contain 87 members")
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
