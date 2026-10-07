#!/usr/bin/env python3
from pathlib import Path
import json

REGION_MAP_C = Path("src/pokenav_region_map.c")
MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/map/map_city_zoom_obj_palette.json")
PREREQUISITES = (
    Path("graphics/gen4_ui/hgss_pokegear/verified/map/map_sub2.json"),
    Path("graphics/gen4_ui/hgss_pokegear/verified/map/map_window_palette.json"),
    Path("graphics/gen4_ui/hgss_pokegear/verified/map/map_city_zoom_text.json"),
)

LEGACY_TOKENS = (
    "PALTAG_CITY_ZOOM",
    "sCityZoomTilesSpritePalette",
    "Pokenav_AllocAndLoadPalettes(sCityZoomTilesSpritePalette)",
    "FreeSpritePaletteByTag(PALTAG_CITY_ZOOM)",
    "LoadCityZoomViewGfx",
    "FreeCityZoomViewGfx",
)

PRESERVE_TOKENS = (
    'graphics/pokenav/region_map/zoom_tiles.png',
    "gRegionMapCityZoomTiles_Pal",
    "CopyPaletteIntoBufferUnfaded(gRegionMapCityZoomTiles_Pal, BG_PLTT_ID(3), PLTT_SIZE_4BPP)",
    'data/region_map/city_map_tilemaps.h',
    'data/region_map/city_map_entries.h',
)

def verify_complete(path):
    data = json.loads(path.read_text())
    p = data.get("pipeline", {})
    state = (
        p.get("authentic_hgss_asset_ready"),
        p.get("wired_live"),
        p.get("legacy_equivalent_removed"),
    )
    if state != (True, True, True):
        raise ValueError(f"{path}: prerequisite migration is not complete")

def main():
    runtime = REGION_MAP_C.read_text()
    for token in LEGACY_TOKENS:
        if token in runtime:
            raise ValueError(f"{REGION_MAP_C}: removed legacy token still present: {token!r}")
    for token in PRESERVE_TOKENS:
        if token not in runtime:
            raise ValueError(f"{REGION_MAP_C}: required geography/BG palette token missing: {token!r}")

    for path in PREREQUISITES:
        verify_complete(path)

    data = json.loads(MANIFEST.read_text())
    if data.get("phase") != "legacy_obj_palette_lifecycle_removed":
        raise ValueError(f"{MANIFEST}: unexpected phase")
    p = data.get("pipeline", {})
    state = (
        p.get("authentic_hgss_asset_ready"),
        p.get("wired_live"),
        p.get("legacy_equivalent_removed"),
    )
    if state != (True, True, True):
        raise ValueError(f"{MANIFEST}: unexpected completed pipeline state")

if __name__ == "__main__":
    main()
