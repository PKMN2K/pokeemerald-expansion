#!/usr/bin/env python3
from pathlib import Path
import json

REGION_MAP = Path("src/region_map.c")
MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/map/map_player_marker.json")

LEGACY_ASSETS = (
    "graphics/pokenav/region_map/brendan_icon.png",
    "graphics/pokenav/region_map/may_icon.png",
    "graphics/pokenav/region_map/red_icon.png",
    "graphics/pokenav/region_map/leaf_icon.png",
)

PRESERVE_TOKENS = (
    "gRegionMapInfos",
    "CreateRegionMapPlayerIcon",
    "playerIconSpritePosX",
    "playerIconSpritePosY",
)

def main():
    src = REGION_MAP.read_text()
    for token in LEGACY_ASSETS:
        if token not in src:
            raise ValueError(f"{REGION_MAP}: expected legacy player-marker asset missing: {token}")
    for token in PRESERVE_TOKENS:
        if token not in src:
            raise ValueError(f"{REGION_MAP}: required map behavior token missing: {token}")

    data = json.loads(MANIFEST.read_text())
    if data.get("phase") != "authentic_player_marker_asset_extracted":
        raise ValueError(f"{MANIFEST}: unexpected phase")

    retail = data.get("retail_hgss_binding", {})
    creation = retail.get("sprite_creation_source", {})
    if (creation.get("template_index"), creation.get("resource_set"), creation.get("animation")) != (3, 1, 1):
        raise ValueError(f"{MANIFEST}: retail HGSS player marker binding changed")

    p = data.get("pipeline", {})
    state = (
        p.get("authentic_hgss_asset_ready"),
        p.get("wired_live"),
        p.get("legacy_equivalent_removed"),
    )
    if state != (True, False, False):
        raise ValueError(f"{MANIFEST}: extraction gate must be ready but remain unwired/unremoved")

if __name__ == "__main__":
    main()
