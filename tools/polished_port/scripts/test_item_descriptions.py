#!/usr/bin/env python3
"""Check source item descriptions, glyph conversion and profile isolation."""
from pathlib import Path
import re

from build_item_descriptions import build, convert_description

ROOT = Path(__file__).resolve().parents[3]
data = build()
assert (ROOT / "src/data/polished_item_descriptions.h").read_bytes() == data
generated = data.decode()
assert "static const u8 *const sPolishedItemDescriptions[ITEMS_COUNT]" in generated
assert len(re.findall(r'^    \[ITEM_\w+\] = COMPOUND_STRING\("', generated, re.M)) == 73
assert "static const u8 sPolishedDesc_" not in generated
assert convert_description("Cures #mon\n<" + "BOLDP><BOLDP>") == "Cures POKéMON\nPP"
assert "#mon." not in generated and "<BOLDP>" not in generated

source = (ROOT / "src/item.c").read_text()
assert '#include "data/polished_item_descriptions.h"' in source
body = source.split("const u8 *GetItemDescription(enum Item itemId)", 1)[1].split("u8 GetItemImportance", 1)[0]
assert body.count("#if POLISHED_JOHTO_DEMO") == 1
assert "sPolishedItemDescriptions[itemId]" in body
assert "return gItemsInfo[itemId].description;" in body
assert "itemId = SanitizeItemId(itemId);" in body
print("PASS: 73 source item descriptions, explicit GBA glyph conversion and native fallback")
