#!/usr/bin/env python3
"""Guard the first source shop-economy migration against profile regressions."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
item = (ROOT / "src/item.c").read_text()
price = item.split("u32 GetItemPrice(enum Item itemId)", 1)[1].split("static bool32 DoesItemHavePluralName", 1)[0]
sell = item.split("u32 GetItemSellPrice(enum Item itemId)", 1)[1].split("bool32 IsHoldEffectChoice", 1)[0]

# Verify the source price adapter is restricted to the Johto configuration.
assert price.count("#if POLISHED_JOHTO_DEMO") == 1
assert price.count("#endif") == 1
for symbol, expected in {
    "ITEM_PARALYZE_HEAL": 300,
    "ITEM_AWAKENING": 100,
    "ITEM_X_DEFENSE": 2000,
}.items():
    assert re.search(r"case\s+" + symbol + r":\s*return\s+" + str(expected) + r";", price), symbol
assert "return gItemsInfo[SanitizeItemId(itemId)].price;" in price
assert "#if POLISHED_JOHTO_DEMO" in sell
assert "return GetItemPrice(itemId) / 2;" in sell
assert "#else" in sell and "return GetItemPrice(itemId) / ITEM_SELL_FACTOR;" in sell
print("Polished Crystal Violet shop economy source-profile guards passed")
