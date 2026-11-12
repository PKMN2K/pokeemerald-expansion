#!/usr/bin/env python3
"""Generate authoritative Polished Crystal prices for source-mapped held items."""
import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "tools/polished_port/manifests/pokemon_held_items.json"
OUTPUT = ROOT / "src/data/polished_item_prices.h"
# Names in the pinned RGBDS item catalog which differ from their GBA IDs.
MART_ALIASES = {
    "PARALYZEHEAL": ("ITEM_PARALYZE_HEAL", 300),
    "AWAKENING": ("ITEM_AWAKENING", 100),
    "X_DEFEND": ("ITEM_X_DEFENSE", 2000),
}


def build():
    doc = json.loads(SOURCE.read_text())
    assert doc["mapped_target_count"] == 391
    rows = [row for row in doc["item_mapping"] if "source_attributes" in row]
    assert len(rows) == 70
    from build_pokemon_held_items import catalog, sources
    source_items = catalog(sources())
    for source_name, (target_item, expected_price) in MART_ALIASES.items():
        assert target_item not in {r["target_item"] for r in rows}, target_item
        attributes = source_items[source_name]["source_attributes"]
        assert int(attributes[0]) == expected_price, (source_name, attributes[0])
        rows.append({"source_item": source_name, "target_item": target_item,
                     "source_attributes": attributes})
    assert len(rows) == 73
    names = set(re.findall(r"\bITEM_\w+", (ROOT / "include/constants/items.h").read_text()))
    assert len({r["target_item"] for r in rows}) == len(rows)
    out = ["/* Generated from pinned default Polished Crystal item attributes. */",
           "/* Zero denotes native fallback for items absent from the source mapping. */",
           "static const u32 sPolishedMappedItemPrices[ITEMS_COUNT] = {"]
    for row in sorted(rows, key=lambda row: row["target_item"]):
        symbol = row["target_item"]
        assert symbol in names, symbol
        value = row["source_attributes"][0]
        assert value.isdecimal() and 0 < int(value) <= 65535, (symbol, value)
        out.append(f"    [{symbol}] = {int(value)}, /* {row['source_item']} */")
    out += ["};", ""]
    return "\n".join(out).encode()


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--check", action="store_true")
    args = p.parse_args()
    data = build()
    if args.check:
        assert OUTPUT.read_bytes() == data, "Regenerate source mapped item prices"
    else:
        OUTPUT.write_bytes(data)
    print("PASS: 73 pinned source item prices (70 mapped held items plus 3 Violet Mart corrections)")


if __name__ == "__main__":
    main()
