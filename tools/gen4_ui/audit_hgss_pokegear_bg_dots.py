#!/usr/bin/env python3
from pathlib import Path
import hashlib
import json


LEGACY_DOTS_GFX = Path("graphics/pokenav/bg_dots.png")
LEGACY_DOTS_MAP = Path("graphics/pokenav/bg_dots.bin")
HGSS_SHELL_TILES = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_tiles.png")
HGSS_SHELL_PALETTE = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_palette.NCLR")
HGSS_SHELL_TILEMAP = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_tilemap.NSCR")
HGSS_SHELL_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/screen_shell_skin0.json")
BG_DOTS_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/shared_bg_dots.json")
MENU_GFX_C = Path("src/pokenav_menu_handler_gfx.c")

EXPECTED_GIT_BLOBS = {
    LEGACY_DOTS_GFX: "056e2400b84fa02d02a24b43eb24904eefbf2bdd",
    LEGACY_DOTS_MAP: "7161609bb086de4cc03b0fc52f283649e7f9761d",
    HGSS_SHELL_TILES: "163a4fa068fbffae472d1672947fad86647d110b",
    HGSS_SHELL_PALETTE: "f293e5f8760201fd109d77bb9cefcdcbb4682f67",
    HGSS_SHELL_TILEMAP: "f0d5863b63fb82a6bc36c4f49c17859fea8d05aa",
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


def verify_completed_hgss_shell():
    manifest = json.loads(HGSS_SHELL_MANIFEST.read_text())
    pipeline = manifest.get("pipeline", {})
    required = (
        pipeline.get("authentic_hgss_asset_ready"),
        pipeline.get("wired_live"),
        pipeline.get("legacy_equivalent_removed"),
    )
    if required != (True, True, True):
        raise ValueError(
            f"{HGSS_SHELL_MANIFEST}: fixed-shell pipeline is no longer complete"
        )

    menu = MENU_GFX_C.read_text()
    for token in (
        "LoadHgssPokegearScreenShell();",
        "sHgssPokegearScreenShellTiles",
        "sHgssPokegearScreenShellPal",
        "sHgssPokegearScreenShellTilemap",
    ):
        if token not in menu:
            raise ValueError(f"{MENU_GFX_C}: missing live HGSS shell token {token!r}")


def verify_phase2_suppression():
    manifest = json.loads(BG_DOTS_MANIFEST.read_text())
    if manifest.get("phase") != "legacy_bg3_suppressed_ci_pending":
        raise ValueError(f"{BG_DOTS_MANIFEST}: unexpected phase")

    menu = MENU_GFX_C.read_text()

    # Rollback resources remain byte-locked for this one validation cycle.
    for token in (
        'INCGFX_U16("graphics/pokenav/bg_dots.png", ".gbapal")',
        'INCGFX_U32("graphics/pokenav/bg_dots.png", ".4bpp.smol")',
        'INCGFX_U32("graphics/pokenav/bg_dots.bin", ".smolTM")',
        "DecompressAndCopyTileDataToVram(3, sPokenavBgDotsTiles",
        "DecompressAndCopyTileDataToVram(3, sPokenavBgDotsTilemap",
        "CopyPaletteIntoBufferUnfaded(sPokenavBgDotsPal, BG_PLTT_ID(3)",
        "ChangeBgX(3, 0x80, BG_COORD_ADD);",
    ):
        if token not in menu:
            raise ValueError(f"{MENU_GFX_C}: rollback implementation missing {token!r}")

    # The normal path must suppress every shared-dot behavior.
    required = (
        "static bool8 sUseLegacyMovingBgDotsForRollback = FALSE;",
        "gfx->bg3ScrollTaskId = TASK_NONE;",
        "if (sUseLegacyMovingBgDotsForRollback)",
        "if (!sUseLegacyMovingBgDotsForRollback)",
        "HideBg(3);",
        "if (gfx->bg3ScrollTaskId != TASK_NONE)",
    )
    for token in required:
        if token not in menu:
            raise ValueError(f"{MENU_GFX_C}: missing Phase-2 suppression token {token!r}")

    if menu.count("if (sUseLegacyMovingBgDotsForRollback)") < 2:
        raise ValueError(f"{MENU_GFX_C}: BG3 load/show are not both rollback-gated")
    if menu.count("if (!sUseLegacyMovingBgDotsForRollback)") < 4:
        raise ValueError(f"{MENU_GFX_C}: scroll/palette helpers are not fully suppressed")


def main():
    for path in EXPECTED_GIT_BLOBS:
        verify_blob(path)
    verify_completed_hgss_shell()
    verify_phase2_suppression()


if __name__ == "__main__":
    main()
