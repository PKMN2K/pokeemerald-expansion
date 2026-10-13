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

    # Original raw 4bpp tiles from HG USA a/0/3/9 NCGR members 72-83.
    # Hashes are tied to retail ROM data, not merely expected file sizes.
    installed = {
        "champion.4bpp": (72, "59e2445ffb0104fe291b73931cd89839e30ccd8ac6bdb8b11cdd9dd7e869b25f"),
        "contest_normal.4bpp": (73, "982e40970474f3ee142f3fae75430472652774dd6748a3d8ae841a5e4fbebc99"),
        "contest_super.4bpp": (74, "4c733a21d75d2ac3005e81bddf6b1dd320eb4eed0f6aa1a6ad5aa073cc42e961"),
        "contest_hyper.4bpp": (75, "c67959b864234c997b87f795eca9dee8e732dcd9d6d28515f54b73ba9826066b"),
        "contest_master.4bpp": (76, "da4413ade2558e5a32b4e4e8bea587eaed1469bd9ae3d73e2ca0e4bfce85f53b"),
        "victory_77.4bpp": (77, "a478a73b7bd7bac7a4d1a858412c9bd981a5feeab0e99d7beec7a602d7946829"),
        "winning_78.4bpp": (78, "238ee96500435bbf7d985c0d68b4b3d872cc83ba61cbf78c8a9184c774602067"),
        "artist_79.4bpp": (79, "f675024fb5ea69ae14bc9f42aa91bba92b39748b401e8d2d92243c06ea68c03c"),
        "effort_80.4bpp": (80, "26dcf3affebb24151467b1c7dccad91712255a1f53f96c9f1cb7929956792042"),
        "marine_land_sky_81.4bpp": (81, "c3a7d55673bc680d66640681b569b8b94fbe97d2434cb3a02b5b6c9bf19ac6a2"),
        "country_national_82.4bpp": (82, "7be5f114e9e5bd92ea0261e57d51bae87038575d98acdd6d5cbfc2ef5ca9d18e"),
        "earth_world_83.4bpp": (83, "ead81584d4e756a094f552522c59659b8c4f1d4f108c0dbcfed2b80bc3f64811"),
    }
    imported = {p.name for p in RIBBONS.glob("*.4bpp")}
    assert imported == set(installed), (
        "Update the verified HGSS asset inventory for new or missing NCGR files",
        sorted(imported ^ set(installed)),
    )
    for name, (member, expected_digest) in installed.items():
        assert any(int(row["ncgr_member"]) == member for row in emerald), name
        gfx = (RIBBONS / name).read_bytes()
        assert len(gfx) == 32 * 32 // 2, (name, len(gfx))
        digest = hashlib.sha256(gfx).hexdigest()
        assert digest == expected_digest, (name, digest, expected_digest)
        print(f"{name}: 512 bytes, sha256={digest}")

    palette_hashes = (
        "4cc042e701db7bbeef8ccf6ef0ed52e55335eebc6bedc7b00acec7d67b079365",
        "12eb698ed4417f55640b7b50686958fc5ca05e5b8a7846529ebc6c41c2a667fc",
        "145207f9fa13957e0bccaae32b08656147f184450d1f6502f9a9e682d794d780",
        "7b88beecb7d30c73a98d2b0f42f768d1ab16b85a9a79047e551c15b783970b8b",
        "24125e0d9252ab3cfb4281468c7cf7955fc85074bd22129ce0987b8a70f19bde",
    )
    for bank, expected_digest in enumerate(palette_hashes):
        name = f"palette_{bank}.gbapal"
        pal = (RIBBONS / name).read_bytes()
        assert len(pal) == 16 * 2, (name, len(pal))
        assert any(int(row["nclr_bank"]) == bank for row in emerald), name
        digest = hashlib.sha256(pal).hexdigest()
        assert digest == expected_digest, (name, digest, expected_digest)
        print(f"{name}: 32 bytes, sha256={digest}")

    # Native retail NCGR tiles: 12 original 32x32 blocks in zoom-frame order.
    big_icon_order = (
        "champion.4bpp",
        "contest_normal.4bpp",
        "contest_super.4bpp",
        "contest_hyper.4bpp",
        "contest_master.4bpp",
        "winning_78.4bpp",
        "victory_77.4bpp",
        "artist_79.4bpp",
        "effort_80.4bpp",
        "marine_land_sky_81.4bpp",
        "country_national_82.4bpp",
        "earth_world_83.4bpp",
    )
    big_sheet = (RIBBONS / "big_icons_hgss.4bpp").read_bytes()
    expected_sheet = b"".join((RIBBONS / name).read_bytes() for name in big_icon_order)
    assert len(big_sheet) == 12 * 512, len(big_sheet)
    assert big_sheet == expected_sheet, "HGSS selected ribbon sheet differs from retail NCGRs"
    assert hashlib.sha256(big_sheet).hexdigest() == (
        "5b78564602edf75e2ceb4d93327310da286cc131fb9422081f07f002713eca4d"
    ), "HGSS ribbon sprite sheet does not match the source ROM"

    print("HGSS ribbon metadata consistent; imported binaries have expected 4bpp/palette sizes.")


if __name__ == "__main__":
    main()
