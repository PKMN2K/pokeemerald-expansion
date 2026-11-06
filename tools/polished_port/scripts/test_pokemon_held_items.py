#!/usr/bin/env python3
"""Audit source item identities, exact byte probabilities and icon pixels."""
import json
import re
import struct
import tempfile
import zlib
from pathlib import Path

import build_pokemon_learnsets as source
from build_pokemon_held_items import ROOT, MANIFEST, build, sources
from convert_player_walking import _read_png_2bit_grayscale


def indexed_pixels(data):
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    offset, image = 8, b""
    while offset < len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        kind, payload = data[offset + 4:offset + 8], data[offset + 8:offset + 8 + length]
        if kind == b"IHDR":
            assert struct.unpack(">IIBBBBB", payload) == (24, 24, 4, 3, 0, 0, 0)
        elif kind == b"IDAT":
            image += payload
        elif kind == b"tRNS":
            assert payload == bytes((0, 255, 255, 255))
        offset += 12 + length
    raw = zlib.decompress(image)
    assert len(raw) == 24 * 13
    rows = []
    for y in range(24):
        assert raw[y * 13] == 0
        rows.append([v for b in raw[y * 13 + 1:y * 13 + 13] for v in (b >> 4, b & 15)])
    return rows


def main():
    for path, expected in build().items():
        assert path.read_bytes() == expected, path
    manifest = json.loads(MANIFEST.read_text())
    rows = {r["target_species"]: r for r in manifest["records"]}
    assert len(rows) == 391
    assert rows["SPECIES_ODDISH"]["common"] == "ITEM_POLISHED_SILVER_LEAF"
    assert rows["SPECIES_BELLSPROUT"]["common"] == "ITEM_POLISHED_GOLD_LEAF"
    assert rows["SPECIES_SYLVEON"]["rare"] == "ITEM_POLISHED_PINK_BOW"
    assert rows["SPECIES_VULPIX_ALOLA"]["always_rare"]
    assert rows["SPECIES_MEWTWO_ARMORED"]["rare"] == "ITEM_PETAYA_BERRY"
    assert rows["SPECIES_URSALUNA"]["rare"] == "ITEM_SITRUS_BERRY"
    assert rows["SPECIES_URSALUNA_BLOODMOON"]["rare"] == "ITEM_LUM_BERRY"
    assert rows["SPECIES_JIGGLYPUFF"]["common"] == rows["SPECIES_JIGGLYPUFF"]["rare"] == "ITEM_ORAN_BERRY"
    assert not rows["SPECIES_JIGGLYPUFF"]["always_rare"]
    assert rows["SPECIES_MAGIKARP_RAINDROP"]["common"] == rows["SPECIES_MAGIKARP_RAINDROP"]["rare"] == "ITEM_NONE"
    mapping = {r["source_item"]: r for r in manifest["item_mapping"]}
    assert len(mapping) == 71 and "ALWAYS_ITEM_2" not in mapping
    assert mapping["NO_ITEM"]["source_id"] == 0
    # Independent complete enumeration: these are not rounded 50/5/45 or
    # 60/20/20 percentages. Each pair of bytes is equally likely.
    limits = manifest["byte_thresholds"]
    for prefix, expected in (("", (32512, 3225, 29799)), ("boost_", (39168, 13081, 13287))):
        counts = [0, 0, 0]
        for first in range(256):
            for second in range(256):
                outcome = 0 if first < limits[prefix + "common"] else 1 if second < limits[prefix + "rare"] else 2
                counts[outcome] += 1
        assert tuple(counts) == expected
    pinned = source.sources()
    core = "\n".join(source.default_lines(pinned["engine/battle/core.asm"].split("; Let's get the item:", 1)[1].split(".UpdateItem:", 1)[0]))
    assert core.index("inc a") < core.index("BATTLETYPE_LEGENDARY") < core.index("call GetLeadAbility")
    assert core.index("cp SUPER_LUCK") < core.index("cp COMPOUND_EYES") < core.index("ld a, [wPartyMon1Item]")
    assert core.count("call BattleRandom") == core.count("jr c, .UpdateItem") == 4
    ability = pinned["home/pokemon.asm"].split("GetLeadAbility::", 1)[1].split("\n\n", 1)[0]
    assert "wPartyMon1IsEgg" in ability and "wPartyMon1Species" in ability and "HP" not in ability
    damage = pinned["engine/battle/effect_commands.asm"].split(".type_boost\n", 1)[1]
    assert "ln a, 6, 5 ; x1.2" in damage
    fixtures = sources()
    assert "DEF ALWAYS_ITEM_2  EQU $ff" in fixtures["constants/item_constants.asm"].decode()
    expected_colors = {"PINK_BOW": [[31, 18, 23], [28, 12, 17]],
                       "SILVER_LEAF": [[19, 24, 22], [12, 16, 15]],
                       "GOLD_LEAF": [[31, 25, 8], [27, 18, 1]]}
    for new in manifest["new_items"]:
        token = new["source_item"]
        assert new["source_plural_name"] == {"PINK_BOW": "Pink Bows", "SILVER_LEAF": "Silver Leaves", "GOLD_LEAF": "Gold Leaves"}[token]
        assert new["source_rgb555"] == expected_colors[token]
        assert new["source_id"] == {"PINK_BOW": 0x8a, "SILVER_LEAF": 0xe1, "GOLD_LEAF": 0xe2}[token]
        assert new["source_attributes"] == (["1000", "HELD_TYPE_BOOST", "FAIRY", "ITEM", "ITEMMENU_NOUSE", "ITEMMENU_NOUSE"] if token == "PINK_BOW"
                                             else ["500", "0", "0", "ITEM", "ITEMMENU_NOUSE", "ITEMMENU_NOUSE"])
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "source.png"
            src.write_bytes(fixtures[new["source_icon"]])
            w, h, pixels = _read_png_2bit_grayscale(src)
        assert (w, h) == (24, 24)
        actual = indexed_pixels((ROOT / f"graphics/items/icons/polished_{token.lower()}.png").read_bytes())
        assert actual == [[3 - v for v in row] for row in pixels]
        pal = (ROOT / f"graphics/items/icon_palettes/polished_{token.lower()}.pal").read_text().splitlines()
        colors = [list(map(int, line.split())) for line in pal[3:]]
        assert colors[:4] == [[255] * 3] + [[(v << 3) | (v >> 2) for v in rgb] for rgb in expected_colors[token]] + [[0] * 3]
        constants = (ROOT / "include/constants/items.h").read_text()
        assert f'{new["target_item"]} = {new["target_id"]},' in constants
    print("PASS: all 391 identities, 70 actual item mappings, source branches, 131072 byte pairs and original icon pixels/colors")


if __name__ == "__main__":
    main()
