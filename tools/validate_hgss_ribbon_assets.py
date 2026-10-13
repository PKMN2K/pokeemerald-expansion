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

    # Verify every imported authentic NCGR tile file, including shared contest
    # shapes. Files are never synthesized: only resources present on disk count.
    installed = {
        "champion.4bpp": 72,
        "contest_normal.4bpp": 73,
        "contest_super.4bpp": 74,
        "contest_hyper.4bpp": 75,
        "contest_master.4bpp": 76,
        "victory_77.4bpp": 77,
        "winning_78.4bpp": 78,
        "artist_79.4bpp": 79,
        "effort_80.4bpp": 80,
    }
    imported = {p.name for p in RIBBONS.glob("*.4bpp")}
    assert imported == set(installed), (
        "Update the verified HGSS asset inventory for new or missing NCGR files",
        sorted(imported ^ set(installed)),
    )
    for name, member in installed.items():
        assert any(int(row["ncgr_member"]) == member for row in emerald), name
        gfx = (RIBBONS / name).read_bytes()
        assert len(gfx) == 32 * 32 // 2, (name, len(gfx))
        print(f"{name}: 512 bytes, sha256={hashlib.sha256(gfx).hexdigest()}")

    for bank in range(5):
        name = f"palette_{bank}.gbapal"
        pal = (RIBBONS / name).read_bytes()
        assert len(pal) == 16 * 2, (name, len(pal))
        assert any(int(row["nclr_bank"]) == bank for row in emerald), name
        print(f"{name}: 32 bytes, sha256={hashlib.sha256(pal).hexdigest()}")
    print("HGSS ribbon metadata consistent; imported binaries have expected 4bpp/palette sizes.")


if __name__ == "__main__":
    main()
