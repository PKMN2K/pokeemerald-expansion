#!/usr/bin/env python3
from pathlib import Path
import hashlib
import json


APP_SWITCH_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/app_switch_skin0.json")
SCREEN_SHELL_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/screen_shell_skin0.json")
LEFT_HEADERS_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/left_headers.json")

AUTHENTIC_GIT_BLOBS = {
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_tiles.png"): "0b33041050709233198495bf43f4969738c4921e",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_palette.NCLR"): "69f91d3bf7a72e5868d5dc6cf04add710bd7138a",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_tilemap.NSCR"): "c9103c3dba3d4f1ec88025987ad0f2ddbc63c0b1",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_tiles.png"): "163a4fa068fbffae472d1672947fad86647d110b",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_palette.NCLR"): "f293e5f8760201fd109d77bb9cefcdcbb4682f67",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_tilemap.NSCR"): "f0d5863b63fb82a6bc36c4f49c17859fea8d05aa",
}

LEGACY_ASSETS = (
    Path("graphics/pokenav/left_headers/beauty.png"),
    Path("graphics/pokenav/left_headers/condition.png"),
    Path("graphics/pokenav/left_headers/cool.png"),
    Path("graphics/pokenav/left_headers/cute.png"),
    Path("graphics/pokenav/left_headers/hoenn_map.png"),
    Path("graphics/pokenav/left_headers/main_menu.png"),
    Path("graphics/pokenav/left_headers/match_call.png"),
    Path("graphics/pokenav/left_headers/palette.pal"),
    Path("graphics/pokenav/left_headers/party.png"),
    Path("graphics/pokenav/left_headers/ribbons.png"),
    Path("graphics/pokenav/left_headers/search.png"),
    Path("graphics/pokenav/left_headers/smart.png"),
    Path("graphics/pokenav/left_headers/tough.png"),
)

RUNTIME_FILES = (
    Path("src/pokenav_main_menu.c"),
    Path("src/pokenav_conditions_gfx.c"),
    Path("src/pokenav_conditions_search_results.c"),
    Path("src/pokenav_match_call_gfx.c"),
    Path("src/pokenav_menu_handler_gfx.c"),
    Path("src/pokenav_region_map.c"),
    Path("src/pokenav_ribbons_list.c"),
    Path("src/graphics.c"),
    Path("include/graphics.h"),
    Path("include/pokenav.h"),
)

FORBIDDEN_RUNTIME_TOKENS = (
    "LoadLeftHeaderGfxForIndex",
    "UpdateRegionMapRightHeaderTiles",
    "ShowLeftHeaderGfx",
    "HideMainOrSubMenuLeftHeader",
    "SetLeftHeaderSpritesInvisibility",
    "AreLeftHeaderSpritesMoving",
    "gPokenavLeftHeader",
    "leftHeaderSprites",
    "submenuLeftHeaderSprites",
    "sMenuLeftHeader",
    "sPokenavSubMenuLeftHeader",
    "sUseLegacyLeftHeadersForRollback",
    "MoveLeftHeader",
    "SpriteCB_MoveLeftHeader",
    "graphics/pokenav/left_headers",
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
        raise ValueError(f"{path}: authentic HGSS prerequisite is no longer complete")


def verify_legacy_removed():
    for path in LEGACY_ASSETS:
        if path.exists():
            raise ValueError(f"{path}: obsolete Emerald left-header asset still exists")

    for path in RUNTIME_FILES:
        text = path.read_text()
        for token in FORBIDDEN_RUNTIME_TOKENS:
            if token in text:
                raise ValueError(f"{path}: obsolete left-header token remains: {token!r}")


def verify_phase3_manifest():
    manifest = json.loads(LEFT_HEADERS_MANIFEST.read_text())
    if manifest.get("phase") != "legacy_left_headers_removed":
        raise ValueError(f"{LEFT_HEADERS_MANIFEST}: unexpected phase")
    pipeline = manifest.get("pipeline", {})
    state = (
        pipeline.get("authentic_hgss_asset_ready"),
        pipeline.get("wired_live"),
        pipeline.get("legacy_equivalent_removed"),
    )
    if state != (True, True, True):
        raise ValueError(f"{LEFT_HEADERS_MANIFEST}: unexpected Phase-3 pipeline state")


def main():
    for path, expected in AUTHENTIC_GIT_BLOBS.items():
        verify_blob(path, expected)
    verify_completed_manifest(APP_SWITCH_MANIFEST)
    verify_completed_manifest(SCREEN_SHELL_MANIFEST)
    verify_legacy_removed()
    verify_phase3_manifest()


if __name__ == "__main__":
    main()
