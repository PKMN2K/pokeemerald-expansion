#!/usr/bin/env python3
"""Guard the pinned Figy held-effect specification and its healing amount."""
import build_pokemon_learnsets as source
from build_pokemon_held_items import catalog, sources
from build_item_healing import build, OUTPUT

files = source.sources()  # Verifies every fixture's Git blob hash and pin.
attrs = catalog(sources())["FIGY_BERRY"]["source_attributes"]
assert attrs[1:] == ["HELD_BERRY", "0", "BERRIES", "ITEMMENU_PARTY", "ITEMMENU_PARTY"]
core = files["engine/battle/core.asm"]
natural = core.split("HandleHPHealingItem:", 1)[1].split("UseBattleItem:", 1)[0]
assert "call GetHalfMaxHP\n\tcall CompareHP\n\tjr z, .ok\n\tret nc" in natural
assert "cp FIGY_BERRY\n\tjr nz, .figy_ok\n\n\tcall QuarterPinchOrGluttony\n\tret nz" in natural
assert natural.count("predef GetUserItemAfterUnnerve") == 2
pinch = core.split("QuarterPinchOrGluttony::", 1)[1].split("HandleStatBoostBerry:", 1)[0]
assert "call GetQuarterMaxHP\n\tcall GetTrueUserAbility\n\tcp GLUTTONY\n\tcall z, GetHalfMaxHP" in pinch
assert "call CompareHP\n\tret nc\n\txor a\n\tret" in pinch
held = core.split("_HeldHPHealingItem:", 1)[1].split("CurItemRecoveryAnim::", 1)[0]
assert "cp HELD_BERRY\n\tret nz" in held
assert "cp FIGY_BERRY\n\tcall z, GetThirdMaxHP" in held
assert "call CurItemRecoveryAnim\n\tcall RestoreHP\n\txor a\n\tret" in held
assert not any(word in held.lower() for word in ("confus", "flavor", "nature", "ripen"))
for routine in ("StealHPHealingItem:", "ReconsumeHPHealingItem:"):
    assert core.split(routine, 1)[1].lstrip().split("call CheckFullHP", 1)[1].startswith("\n\tret z")
half = files["home/battle.asm"].split("HalveBC::", 1)[1].split("GetMaxHP::", 1)[0]
assert "srl b\n\trr c\nFloorBC::\n\tld a, c\n\tor b\n\tret nz\n\tinc c" in half
assert OUTPUT.read_bytes() == build()
print("PASS: pinned Figy HELD_BERRY, one-third healing, no confusion, inclusive pinch/Gluttony and forced-use rules")
