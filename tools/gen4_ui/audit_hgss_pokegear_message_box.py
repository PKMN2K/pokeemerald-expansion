#!/usr/bin/env python3
from pathlib import Path
import hashlib
import json


HANDLER = Path("src/pokenav_menu_handler_gfx.c")
GRAPHICS = Path("src/graphics.c")
GRAPHICS_H = Path("include/graphics.h")
SHELL_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/screen_shell_skin0.json")
TEXT_PALETTE_MANIFEST = Path("graphics/gen4_ui/hgss_storage/verified/text_windows.json")
MESSAGE_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/message_box.json")
TEXT_PALETTE = Path("graphics/gen4_ui/hgss_storage/verified/text_windows_hgss.gbapal")

LEGACY_ASSETS = (
    Path("graphics/pokenav/message.png"),
    Path("graphics/pokenav/message.bin"),
)

AUTHENTIC_GIT_BLOBS = {
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_tiles.png"): "163a4fa068fbffae472d1672947fad86647d110b",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_palette.NCLR"): "f293e5f8760201fd109d77bb9cefcdcbb4682f67",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_tilemap.NSCR"): "f0d5863b63fb82a6bc36c4f49c17859fea8d05aa",
    TEXT_PALETTE: "975495973ba86ed1e1d53104f6e2a4ef2fd6471d",
}

FORBIDDEN_RUNTIME_TOKENS = (
    "sUseLegacyMessageBoxForRollback",
    "gPokenavMessageBox_Pal",
    "gPokenavMessageBox_Gfx",
    "gPokenavMessageBox_Tilemap",
    "graphics/pokenav/message.png",
    "graphics/pokenav/message.bin",
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
        raise ValueError(f"{path}: authentic prerequisite is no longer complete")


def verify_legacy_removed():
    for path in LEGACY_ASSETS:
        if path.exists():
            raise ValueError(f"{path}: obsolete Emerald message-box asset still exists")

    for path in (HANDLER, GRAPHICS, GRAPHICS_H):
        text = path.read_text()
        for token in FORBIDDEN_RUNTIME_TOKENS:
            if token in text:
                raise ValueError(f"{path}: obsolete message-box token remains: {token!r}")


def verify_hgss_text_rehome():
    handler = HANDLER.read_text()
    required = (
        'static const u16 sHgssConditionSearchFontPal[] = INCBIN_U16("graphics/gen4_ui/hgss_storage/verified/text_windows_hgss.gbapal");',
        ".paletteNum = HGSS_CONDITION_SEARCH_FONT_PAL_BANK,",
        "CopyPaletteIntoBufferUnfaded(sHgssConditionSearchFontPal, BG_PLTT_ID(HGSS_CONDITION_SEARCH_FONT_PAL_BANK), sizeof(sHgssConditionSearchFontPal));",
        "static const u8 sOptionDescTextColors[]  = {0, 1, 2};",
        "static const u8 sOptionDescTextColors2[] = {0, 1, 2};",
        "LoadBgTiles(1, sTransparentMessageBoxBgTile, sizeof(sTransparentMessageBoxBgTile), 0);",
        "FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 32, 32);",
        "LoadHgssPokegearScreenShell();",
        "ShowBg(1);",
    )
    for token in required:
        if token not in handler:
            raise ValueError(f"{HANDLER}: Phase-3 HGSS text/shell token changed: {token!r}")


def verify_phase3_manifest():
    manifest = json.loads(MESSAGE_MANIFEST.read_text())
    if manifest.get("phase") != "legacy_message_box_removed":
        raise ValueError(f"{MESSAGE_MANIFEST}: unexpected phase")
    pipeline = manifest.get("pipeline", {})
    state = (
        pipeline.get("authentic_hgss_asset_ready"),
        pipeline.get("wired_live"),
        pipeline.get("legacy_equivalent_removed"),
    )
    if state != (True, True, True):
        raise ValueError(f"{MESSAGE_MANIFEST}: unexpected Phase-3 pipeline state")


def main():
    for path, expected in AUTHENTIC_GIT_BLOBS.items():
        verify_blob(path, expected)
    verify_completed_manifest(SHELL_MANIFEST)
    verify_completed_manifest(TEXT_PALETTE_MANIFEST)
    verify_legacy_removed()
    verify_hgss_text_rehome()
    verify_phase3_manifest()


if __name__ == "__main__":
    main()
