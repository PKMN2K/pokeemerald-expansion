#!/usr/bin/env python3
from pathlib import Path
import hashlib
import json


APP_SWITCH_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/app_switch_skin0.json")
SCREEN_SHELL_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/screen_shell_skin0.json")
LEFT_HEADERS_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/left_headers.json")
MAIN_MENU_C = Path("src/pokenav_main_menu.c")
MENU_GFX_C = Path("src/pokenav_menu_handler_gfx.c")
GRAPHICS_C = Path("src/graphics.c")

EXPECTED_GIT_BLOBS = {
    Path("graphics/pokenav/left_headers/beauty.png"): "96eb88ca621cb6038e16581aa38472e489374f9f",
    Path("graphics/pokenav/left_headers/condition.png"): "6d80db5e3f7685965a41c6028a6e4bc7a0ea7dc4",
    Path("graphics/pokenav/left_headers/cool.png"): "a96291b836c9d947026cdc523b5ab07dcabb8a0e",
    Path("graphics/pokenav/left_headers/cute.png"): "63c3f9557eda5caf69b26aa16dfa3b2f5038b112",
    Path("graphics/pokenav/left_headers/hoenn_map.png"): "b347c01ee947855358f5a64e4612329a3f87e750",
    Path("graphics/pokenav/left_headers/main_menu.png"): "c0371f5678fb57ea42c70e96890d4c6009e774fa",
    Path("graphics/pokenav/left_headers/match_call.png"): "66455af78bbf8fe14a7a393c45120f38fbf8fa4e",
    Path("graphics/pokenav/left_headers/palette.pal"): "eb13b1a87259d4393c6cda3261bcacddde8b26e7",
    Path("graphics/pokenav/left_headers/party.png"): "b66ef3b17fc2c310c67110ed72bf86896438f4e6",
    Path("graphics/pokenav/left_headers/ribbons.png"): "6753dc9233c4beced965967f1349f5176f636028",
    Path("graphics/pokenav/left_headers/search.png"): "f4484accf8209d48a0281f9e444f76d6bfc38d20",
    Path("graphics/pokenav/left_headers/smart.png"): "c53dcdf4c00e2ce63448b2908e57daa259a1be23",
    Path("graphics/pokenav/left_headers/tough.png"): "f78d47524b89aebaa7fcc6a114a9ab5bfe3a166f",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_tiles.png"): "0b33041050709233198495bf43f4969738c4921e",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_palette.NCLR"): "69f91d3bf7a72e5868d5dc6cf04add710bd7138a",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_tilemap.NSCR"): "c9103c3dba3d4f1ec88025987ad0f2ddbc63c0b1",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_tiles.png"): "163a4fa068fbffae472d1672947fad86647d110b",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_palette.NCLR"): "f293e5f8760201fd109d77bb9cefcdcbb4682f67",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_tilemap.NSCR"): "f0d5863b63fb82a6bc36c4f49c17859fea8d05aa",
}


def git_blob_sha(data):
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def verify_blob(path):
    data = path.read_bytes()
    actual = git_blob_sha(data)
    expected = EXPECTED_GIT_BLOBS[path]
    if actual != expected:
        raise ValueError(f"{path}: Git blob {actual} != verified {expected}")


def verify_completed_manifest(path):
    manifest = json.loads(path.read_text())
    pipeline = manifest.get("pipeline", {})
    required = (
        pipeline.get("authentic_hgss_asset_ready"),
        pipeline.get("wired_live"),
        pipeline.get("legacy_equivalent_removed"),
    )
    if required != (True, True, True):
        raise ValueError(f"{path}: authentic HGSS prerequisite is no longer complete")


def verify_authentic_shared_surfaces_are_live():
    menu = MENU_GFX_C.read_text()
    for token in (
        "LoadHgssPokegearScreenShell();",
        "LoadHgssPokegearAppSwitchChrome();",
        "sHgssPokegearScreenShellTiles",
        "sHgssPokegearAppSwitchTiles",
    ):
        if token not in menu:
            raise ValueError(f"{MENU_GFX_C}: missing live authentic HGSS token {token!r}")


def verify_legacy_left_headers_are_still_live():
    main = MAIN_MENU_C.read_text()
    graphics = GRAPHICS_C.read_text()

    required_main = (
        "CreateLeftHeaderSprites();",
        "sMenuLeftHeaderSpriteSheet",
        "sMenuLeftHeaderSpriteSheets[]",
        "sPokenavSubMenuLeftHeaderSpriteSheets[]",
        "LoadLeftHeaderGfxForIndex",
        "ShowLeftHeaderGfx",
        "HideMainOrSubMenuLeftHeader",
        "MoveLeftHeader",
        "SpriteCB_MoveLeftHeader",
    )
    for token in required_main:
        if token not in main:
            raise ValueError(f"{MAIN_MENU_C}: legacy left-header binding changed: {token!r}")

    required_graphics = (
        'INCGFX_U16("graphics/pokenav/left_headers/palette.pal", ".gbapal")',
        'INCGFX_U32("graphics/pokenav/left_headers/main_menu.png", ".4bpp.smol")',
        'INCGFX_U32("graphics/pokenav/left_headers/hoenn_map.png", ".4bpp.smol")',
        'INCGFX_U32("graphics/pokenav/left_headers/match_call.png", ".4bpp.smol")',
        'INCGFX_U32("graphics/pokenav/left_headers/party.png", ".4bpp.smol")',
        'INCGFX_U32("graphics/pokenav/left_headers/search.png", ".4bpp.smol")',
    )
    for token in required_graphics:
        if token not in graphics:
            raise ValueError(f"{GRAPHICS_C}: legacy left-header graphics binding changed: {token!r}")


def verify_phase1_manifest():
    manifest = json.loads(LEFT_HEADERS_MANIFEST.read_text())
    if manifest.get("phase") != "authentic_behavior_audited":
        raise ValueError(f"{LEFT_HEADERS_MANIFEST}: unexpected phase")
    pipeline = manifest.get("pipeline", {})
    if (
        pipeline.get("authentic_hgss_asset_ready"),
        pipeline.get("wired_live"),
        pipeline.get("legacy_equivalent_removed"),
    ) != (True, True, False):
        raise ValueError(f"{LEFT_HEADERS_MANIFEST}: unexpected Phase-1 pipeline state")


def main():
    for path in EXPECTED_GIT_BLOBS:
        verify_blob(path)
    verify_completed_manifest(APP_SWITCH_MANIFEST)
    verify_completed_manifest(SCREEN_SHELL_MANIFEST)
    verify_authentic_shared_surfaces_are_live()
    verify_legacy_left_headers_are_still_live()
    verify_phase1_manifest()


if __name__ == "__main__":
    main()
