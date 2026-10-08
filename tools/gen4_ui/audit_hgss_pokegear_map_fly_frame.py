#!/usr/bin/env python3
from pathlib import Path
import json

REGION_MAP = Path("src/region_map.c")
MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/map/map_fly_frame.json")
MAIN1 = Path("graphics/gen4_ui/hgss_pokegear/verified/map/map_main1.json")

LEGACY_TOKENS = (
    'graphics/pokenav/region_map/frame.png',
    'graphics/pokenav/region_map/frame.bin',
    'sRegionMapFramePal',
    'sRegionMapFrameGfxLZ',
    'sRegionMapFrameTilemapLZ',
)

PRESERVE_TOKENS = (
    'gRegionMapInfos',
    'InitRegionMap(&sFlyMap->regionMap, FALSE)',
    'CreateRegionMapCursor',
    'CreateRegionMapPlayerIcon',
)

def main():
    src = REGION_MAP.read_text()
    for token in LEGACY_TOKENS:
        if token not in src:
            raise ValueError(f"{REGION_MAP}: expected legacy Fly Map frame token missing: {token}")
    for token in PRESERVE_TOKENS:
        if token not in src:
            raise ValueError(f"{REGION_MAP}: required geography/runtime token missing: {token}")

    main1 = json.loads(MAIN1.read_text())
    p1 = main1.get("pipeline", {})
    if (p1.get("authentic_hgss_asset_ready"), p1.get("wired_live"), p1.get("legacy_equivalent_removed")) != (True, True, True):
        raise ValueError(f"{MAIN1}: authentic MAIN_1 prerequisite is not complete")

    data = json.loads(MANIFEST.read_text())
    if data.get("phase") != "legacy_fly_map_frame_audited":
        raise ValueError(f"{MANIFEST}: unexpected phase")
    p = data.get("pipeline", {})
    state = (
        p.get("authentic_hgss_asset_ready"),
        p.get("wired_live"),
        p.get("legacy_equivalent_removed"),
    )
    if state != (True, False, False):
        raise ValueError(f"{MANIFEST}: audit must be asset-ready but unwired/unremoved")

if __name__ == "__main__":
    main()
