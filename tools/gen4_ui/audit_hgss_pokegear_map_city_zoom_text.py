#!/usr/bin/env python3
from pathlib import Path
import hashlib
import json


LEGACY_CITY_ZOOM_TEXT = Path("graphics/pokenav/region_map/city_zoom_text.png")
MAP_SUB2_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/map/map_sub2.json")
MAP_WINDOW_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/map/map_window_palette.json")
CITY_ZOOM_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/map/map_city_zoom_text.json")
REGION_MAP_C = Path("src/pokenav_region_map.c")
GRAPHICS_C = Path("src/graphics.c")

EXPECTED_BLOBS = {
    LEGACY_CITY_ZOOM_TEXT: "4e2b41be6dde2bf5450901a5e611992bf89ec3d3",
    Path("graphics/gen4_ui/hgss_pokegear/verified/map/pgmap_gra_00000062.NCLR"):
        "907fcbae7aef50adf87c6eb1323ab6f2b83c0e77",
    Path("graphics/gen4_ui/hgss_pokegear/verified/map/pgmap_gra_00000068.png"):
        "890771f6c06680c5cc75af1df0ecf90d8a6d318c",
    Path("graphics/gen4_ui/hgss_pokegear/verified/map/pgmap_gra_00000069.NSCR"):
        "09a8868e7ebb01f86554feda5464ab9ebb16f466",
}

LEGACY_RUNTIME_TOKENS = (
    "gRegionMapCityZoomText_Gfx",
    "sCityZoomTextSpriteSheet",
    "CreateCityZoomTextSprites",
    "SpriteCB_CityZoomText",
    "UpdateCityZoomTextPosition",
    "SetCityZoomTextInvisibility",
)


def git_blob_sha(data):
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def verify_blob(path, expected):
    actual = git_blob_sha(path.read_bytes())
    if actual != expected:
        raise ValueError(f"{path}: Git blob {actual} != verified {expected}")


def verify_completed_manifest(path):
    manifest = json.loads(path.read_text())
    pipeline = manifest.get("pipeline", {})
    state = (
        pipeline.get("authentic_hgss_asset_ready"),
        pipeline.get("wired_live"),
        pipeline.get("legacy_equivalent_removed"),
    )
    if state != (True, True, True):
        raise ValueError(f"{path}: prerequisite authentic HGSS Map surface is no longer complete")


def verify_phase1_legacy_binding():
    if not LEGACY_CITY_ZOOM_TEXT.exists():
        raise ValueError(f"{LEGACY_CITY_ZOOM_TEXT}: Phase-1 legacy source unexpectedly missing")

    graphics = GRAPHICS_C.read_text()
    if 'graphics/pokenav/region_map/city_zoom_text.png' not in graphics:
        raise ValueError(f"{GRAPHICS_C}: legacy city-zoom text graphics binding missing")

    runtime = REGION_MAP_C.read_text()
    for token in LEGACY_RUNTIME_TOKENS:
        if token not in runtime and token != "gRegionMapCityZoomText_Gfx":
            raise ValueError(f"{REGION_MAP_C}: expected live Phase-1 token missing: {token!r}")
    if "gRegionMapCityZoomText_Gfx" not in runtime:
        raise ValueError(f"{REGION_MAP_C}: city-zoom text resource is no longer referenced")


def verify_phase1_manifest():
    manifest = json.loads(CITY_ZOOM_MANIFEST.read_text())
    if manifest.get("phase") != "legacy_city_zoom_text_audited":
        raise ValueError(f"{CITY_ZOOM_MANIFEST}: unexpected phase")
    pipeline = manifest.get("pipeline", {})
    state = (
        pipeline.get("authentic_hgss_asset_ready"),
        pipeline.get("wired_live"),
        pipeline.get("legacy_equivalent_removed"),
    )
    if state != (True, False, False):
        raise ValueError(f"{CITY_ZOOM_MANIFEST}: unexpected Phase-1 pipeline state")


def main():
    for path, expected in EXPECTED_BLOBS.items():
        verify_blob(path, expected)
    verify_completed_manifest(MAP_SUB2_MANIFEST)
    verify_completed_manifest(MAP_WINDOW_MANIFEST)
    verify_phase1_legacy_binding()
    verify_phase1_manifest()


if __name__ == "__main__":
    main()
