#!/usr/bin/env python3
"""Validate imported retail HGSS ribbons and Emerald compatibility mapping.

No asset generation or redraws. Run from repository root.
"""
import csv
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RIBBONS = ROOT / "graphics/gen4_ui/hgss_ribbons"


def main():
    with (RIBBONS / "retail_ribbon_map.csv").open(newline="") as fp:
        retail = list(csv.DictReader(fp))
    with (RIBBONS / "emerald_ribbon_compat.csv").open(newline="") as fp:
        emerald = list(csv.DictReader(line for line in fp if not line.startswith("#")))
    assert len(retail) == 80, "Expected all 80 retail HGSS ribbon records"
    assert len(emerald) == 32, "Expected all 32 Emerald ribbon records"
    assert [int(row["retail_ribbon_id"]) for row in retail] == list(range(80))
    assert [int(row["emerald_ribbon_id"]) for row in emerald] == list(range(32))
    for row in retail:
        assert 72 <= int(row["ncgr_member"]) <= 119
        assert 0 <= int(row["nclr_bank"]) <= 4
        assert int(row["retail_sub_obj_bank"]) == 7 + int(row["nclr_bank"])
    for row in emerald:
        source = retail[int(row["retail_ribbon_id"])]
        for field in ("ncgr_member", "nclr_bank", "retail_sub_obj_bank"):
            assert row[field] == source[field], (row["emerald_symbol"], field)
        assert row["retail_mon_data"] == source["mon_data"]

    # These are the ONLY retail ribbon binaries currently committed.
    # Expand this table as each original member is actually added, not from
    # the locally generated staging archive.
    installed = ((0, "champion.4bpp", "palette_0.gbapal", 72, 0),)
    for emerald_id, gfx_name, palette_name, member, bank in installed:
        entry = emerald[emerald_id]
        assert int(entry["ncgr_member"]) == member
        assert int(entry["nclr_bank"]) == bank
        gfx = (RIBBONS / gfx_name).read_bytes()
        pal = (RIBBONS / palette_name).read_bytes()
        assert len(gfx) == 32 * 32 // 2, (gfx_name, len(gfx))
        assert len(pal) == 16 * 2, (palette_name, len(pal))
        print(f"{gfx_name}: 512 bytes, sha256={hashlib.sha256(gfx).hexdigest()}")
        print(f"{palette_name}: 32 bytes, sha256={hashlib.sha256(pal).hexdigest()}")
    print("HGSS ribbon metadata consistent; imported binaries have expected 4bpp/palette sizes.")


if __name__ == "__main__":
    main()
