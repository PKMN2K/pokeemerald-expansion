#!/usr/bin/env python3
"""Verify source X-item stat-raise magnitude and Johto-only runtime selection."""
from pathlib import Path

from build_pokemon_held_items import catalog, sources

ROOT = Path(__file__).resolve().parents[3]
items = catalog(sources())
expected = {
    "X_ATTACK": "ATTACK",
    "X_DEFEND": "DEFENSE",
    "X_SPEED": "SPEED",
    "X_SP_ATK": "SP_ATTACK",
    "X_SP_DEF": "SP_DEFENSE",
    "X_ACCURACY": "ACCURACY",
}
for source_item, stat in expected.items():
    price, effect, parameter, pocket, field, battle = items[source_item]["source_attributes"]
    assert effect == "0", (source_item, effect)
    assert parameter == "$10 | " + stat, (source_item, parameter)
    assert pocket == "ITEM" and field == "ITEMMENU_NOUSE" and battle == "ITEMMENU_CLOSE"

text = (ROOT / "src/data/items.h").read_text()
start = text.index("// Pinned Polished Crystal X-item attributes")
end = text.index("#define TREASURE_FACTOR", start)
block = text[start:end]
assert "#if POLISHED_JOHTO_DEMO" in block
assert "#define X_ITEM_STAGES 1" in block
assert "#else" in block
assert "#define X_ITEM_STAGES ((B_X_ITEMS_BUFF >= GEN_7) ? 2 : 1)" in block
assert block.count("#endif") == 1
for symbol in ("ITEM_X_ATTACK", "ITEM_X_DEFENSE", "ITEM_X_SP_ATK", "ITEM_X_SP_DEF", "ITEM_X_SPEED", "ITEM_X_ACCURACY"):
    at = text.index("    [" + symbol + "] =")
    assert ".holdEffectParam = X_ITEM_STAGES" in text[at:at+290], symbol
print("PASS: six pinned one-stage X-item parameters and profile-isolated GBA adapter")
