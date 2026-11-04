#!/usr/bin/env python3
"""Check the shared retail HGSS panels and Condition-search VRAM layout."""
from pathlib import Path

from make_hgss_ribbon_list import validate, validate_list_layout

ROOT = Path(__file__).resolve().parents[2]


def check():
    # This proves the original source digest, exact tile/palette bytes and
    # lossless adaptation; Condition search uses those same output files.
    validate()
    source = (ROOT / "src/pokenav_conditions_search_results.c").read_text()
    validate_list_layout(source, "sConditionSearchResultBgTemplates", (
        "sSearchResultListMenuWindowTemplate", "sSearchTitleWindowTemplate"))
    for asset in ("summary_bg.4bpp", "summary_bg.bank_1.gbapal", "summary_bg.bank_7.gbapal",
                  "owner_list/background.bin", "owner_list/row.4bpp"):
        assert f'"graphics/gen4_ui/hgss_ribbons/{asset}"' in source, asset
    for legacy in ("search_results.png", "search_results.bin", "search_results_list.pal"):
        assert legacy not in source, legacy
        assert not (ROOT / "graphics/pokenav/condition" / legacy).exists(), legacy
    print("Verified authentic HGSS Condition-search panels, six-row mask and disjoint VRAM")


if __name__ == "__main__":
    check()
