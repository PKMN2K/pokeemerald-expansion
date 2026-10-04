#!/usr/bin/env python3
from pathlib import Path
import hashlib


LEGACY_HEADER_GFX = Path("graphics/pokenav/header.png")
LEGACY_HEADER_MAP = Path("graphics/pokenav/header.bin")
APP_SWITCH_TILES = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_tiles.png")
APP_SWITCH_PALETTE = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_palette.NCLR")
APP_SWITCH_TILEMAP = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_app_switch_tilemap.NSCR")
MAIN_MENU_C = Path("src/pokenav_main_menu.c")
MENU_GFX_C = Path("src/pokenav_menu_handler_gfx.c")
GRAPHICS_C = Path("src/graphics.c")
GRAPHICS_H = Path("include/graphics.h")
GRAPHICS_RULES = Path("graphics_file_rules.mk")
HELP_BAR_GENERATOR = Path("tools/gen4_ui/make_hgss_pokegear_help_bar.py")

EXPECTED_GIT_BLOBS = {
    APP_SWITCH_TILES: "0b33041050709233198495bf43f4969738c4921e",
    APP_SWITCH_PALETTE: "69f91d3bf7a72e5868d5dc6cf04add710bd7138a",
    APP_SWITCH_TILEMAP: "c9103c3dba3d4f1ec88025987ad0f2ddbc63c0b1",
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
    return data


def verify_legacy_removed():
    for path in (LEGACY_HEADER_GFX, LEGACY_HEADER_MAP):
        if path.exists():
            raise ValueError(f"{path}: obsolete Emerald top-header asset still exists")

    main = MAIN_MENU_C.read_text()
    graphics = GRAPHICS_C.read_text()
    graphics_h = GRAPHICS_H.read_text()
    rules = GRAPHICS_RULES.read_text()
    helper = HELP_BAR_GENERATOR.read_text()

    forbidden_main = (
        "sUseLegacyTopHeaderForRollback",
        "ExposeHgssPokegearTopStrip",
        "gPokenavHeader_Gfx",
        "gPokenavHeader_Tilemap",
        "gPokenavHeader_Pal",
    )
    for token in forbidden_main:
        if token in main:
            raise ValueError(f"{MAIN_MENU_C}: legacy top-header token remains: {token}")

    if "gPokenavHeader_" in graphics:
        raise ValueError(f"{GRAPHICS_C}: legacy top-header graphics symbol remains")
    if "gPokenavHeader_" in graphics_h:
        raise ValueError(f"{GRAPHICS_H}: legacy top-header extern remains")
    if "graphics/pokenav/header.png" in rules or "graphics/pokenav/header.bin" in rules:
        raise ValueError(f"{GRAPHICS_RULES}: legacy top-header build dependency remains")
    if "graphics/pokenav/header.png" in helper or "EXPECTED_HEADER_GIT_BLOB" in helper:
        raise ValueError(f"{HELP_BAR_GENERATOR}: legacy palette-slot dependency remains")


def verify_live_hgss_binding():
    main = MAIN_MENU_C.read_text()
    gfx = MENU_GFX_C.read_text()

    required_main = (
        "static const u32 sTransparentBgTile[8] = {0};",
        "LoadBgTiles(0, sTransparentBgTile, sizeof(sTransparentBgTile), 0);",
        "FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, 32, 32);",
        "HGSS_HELP_BAR_STRIP_TOP = 20",
        "HGSS_HELP_BAR_STRIP_HEIGHT = 4",
    )
    for token in required_main:
        if token not in main:
            raise ValueError(f"{MAIN_MENU_C}: missing final HGSS binding {token!r}")

    required_gfx = (
        "#define HGSS_POKEGEAR_APP_SWITCH_WIDTH_TILES        30",
        "#define HGSS_POKEGEAR_APP_SWITCH_HEIGHT_TILES       4",
        "static const u32 sTransparentMessageBoxBgTile[8] = {0};",
        "SetBgTilemapBuffer(1, gfx->bg1TilemapBuffer);",
        "LoadBgTiles(1, sTransparentMessageBoxBgTile, sizeof(sTransparentMessageBoxBgTile), 0);",
        "FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 32, 32);",
        "LoadBgTilemap(",
        "gfx->hgssAppSwitchTilemap",
    )
    for token in required_gfx:
        if token not in gfx:
            raise ValueError(f"{MENU_GFX_C}: missing validated app-switch binding {token!r}")


def main():
    verify_blob(APP_SWITCH_TILES)
    verify_blob(APP_SWITCH_PALETTE)
    verify_blob(APP_SWITCH_TILEMAP)
    verify_legacy_removed()
    verify_live_hgss_binding()


if __name__ == "__main__":
    main()
