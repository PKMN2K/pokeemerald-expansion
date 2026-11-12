#!/usr/bin/env python3
"""Generate GBA-visible descriptions from pinned Polished Crystal item text."""
import argparse
import json
import re
from pathlib import Path

from build_pokemon_held_items import catalog, sources
from build_item_prices import MART_ALIASES, SOURCE

ROOT = Path(__file__).resolve().parents[3]
OUTPUT = ROOT / "src/data/polished_item_descriptions.h"


def convert_description(text):
    # GBC's #mon abbreviation and bold PP glyphs have no identical GBA
    # encoding; preserve meaning with explicit readable replacements.
    text = text.replace("#mon", "POKéMON")
    text = text.replace("<BOLDP><BOLDP>", "PP")
    assert "<" not in text and ">" not in text and "#mon" not in text
    assert '"' not in text and "\r" not in text
    assert text.count("\\n") <= 2
    return text


def build():
    manifest = json.loads(SOURCE.read_text())
    pinned = catalog(sources())  # SHA-verified source fixtures
    entries = {}
    for row in manifest["item_mapping"]:
        if "source_attributes" not in row:
            continue
        token, target = row["source_item"], row["target_item"]
        assert row["source_description"] == pinned[token]["source_description"], token
        entries[target] = (token, convert_description(row["source_description"]))
    assert len(entries) == 70
    for token, (target, _) in MART_ALIASES.items():
        assert target not in entries
        entries[target] = (token, convert_description(pinned[token]["source_description"]))
    assert len(entries) == 81
    names = set(re.findall(r"\bITEM_\w+", (ROOT / "include/constants/items.h").read_text()))
    assert set(entries) <= names
    output = ["/* Generated from SHA-pinned default Polished Crystal item descriptions. */",
              "/* Source #mon -> POKéMON and bold PP glyphs -> PP (GBA text). */",
              "/* COMPOUND_STRING produces a pointer, not a u8 array initializer. */",
              "static const u8 *const sPolishedItemDescriptions[ITEMS_COUNT] = {"]
    for target, (token, value) in sorted(entries.items()):
        output.append(f'    [{target}] = COMPOUND_STRING("{value}"),')
    output += ["};", ""]
    return "\n".join(output).encode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    data = build()
    if args.check:
        assert OUTPUT.read_bytes() == data, "Regenerate Polished Crystal item descriptions"
    else:
        OUTPUT.write_bytes(data)
    print("PASS: 81 source item descriptions and explicit GBA glyph substitutions")


if __name__ == "__main__":
    main()
