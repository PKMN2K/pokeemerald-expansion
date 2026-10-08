#!/usr/bin/env python3
from pathlib import Path
import hashlib
import struct
import sys


PALETTE = Path("graphics/gen4_ui/hgss_pokegear/verified/pgphone_skin0_contact_palette.NCLR")
EXPECTED_PALETTE_BLOB = "85b29945c3b9e8250b119922d64111ded55b1563"
RETAIL_ROW_PALETTE_BANK = 2


def git_blob_sha(data):
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def load_nclr(path):
    data = path.read_bytes()
    actual = git_blob_sha(data)
    if actual != EXPECTED_PALETTE_BLOB:
        raise ValueError(
            f"{path}: Git blob {actual} != verified retail "
            f"{EXPECTED_PALETTE_BLOB}"
        )

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


def make_palette():
    colors = load_nclr(PALETTE)
    start = RETAIL_ROW_PALETTE_BANK * 16
    bank = colors[start:start + 16]
    if len(bank) != 16:
        raise ValueError(
            f"{PALETTE}: missing retail Phone row palette bank "
            f"{RETAIL_ROW_PALETTE_BANK}"
        )
    return struct.pack("<16H", *bank)


def main():
    if len(sys.argv) != 2:
        raise SystemExit(
            "usage: make_hgss_pokegear_contact_rows.py OUTPUT.gbapal"
        )

    output = Path(sys.argv[1])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(make_palette())


if __name__ == "__main__":
    main()
