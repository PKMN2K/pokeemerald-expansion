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

EXPECTED_HGSS_GIT_BLOBS = {
    HGSS_SHELL_TILES: "163a4fa068fbffae472d1672947fad86647d110b",
    HGSS_SHELL_PALETTE: "f293e5f8760201fd109d77bb9cefcdcbb4682f67",
    HGSS_SHELL_TILEMAP: "f0d5863b63fb82a6bc36c4f49c17859fea8d05aa",
}

REMOVED_SOURCE_TOKENS = (
    "sUseLegacyMovingBgDotsForRollback",
    "sPokenavBgDotsPal",
    "sPokenavBgDotsTiles",
    "sPokenavBgDotsTilemap",
    "CreateMovingBgDotsTask",
    "DestroyMovingDotsBgTask",
    "Task_MoveBgDots",
    "CreateBgDotPurplePalTask",
    "ChangeBgDotsColorToPurple",
    "CreateBgDotLightBluePalTask",
    "IsTaskActive_UpdateBgDotsPalette",
    "Task_UpdateBgDotsPalette",
    "bg3ScrollTaskId",
    "graphics/pokenav/bg_dots",
)


def git_blob_sha(data):
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def verify_hgss_blob(path):
    data = path.read_bytes()
    actual = git_blob_sha(data)
    expected = EXPECTED_HGSS_GIT_BLOBS[path]
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


def verify_phase3_removal():
    manifest = json.loads(BG_DOTS_MANIFEST.read_text())
    if manifest.get("phase") != "legacy_bg3_removed_ci_pending":
        raise ValueError(f"{BG_DOTS_MANIFEST}: unexpected phase")

    pipeline = manifest.get("pipeline", {})
    if pipeline.get("legacy_equivalent_removed") is not True:
        raise ValueError(f"{BG_DOTS_MANIFEST}: removal flag is not final")

    for path in (LEGACY_DOTS_GFX, LEGACY_DOTS_MAP):
        if path.exists():
            raise ValueError(f"{path}: obsolete legacy moving-dot asset still exists")

    menu = MENU_GFX_C.read_text()
    for token in REMOVED_SOURCE_TOKENS:
        if token in menu:
            raise ValueError(f"{MENU_GFX_C}: removed legacy token still present: {token!r}")

    # The shared/core launcher keeps BG3 blank. Feature modules may still use
    # BG3 independently for their own app-specific surfaces.
    if menu.count("HideBg(3);") < 2:
        raise ValueError(f"{MENU_GFX_C}: core PokéGear no longer keeps BG3 hidden")


def main():
    for path in EXPECTED_HGSS_GIT_BLOBS:
        verify_hgss_blob(path)
    verify_completed_hgss_shell()
    verify_phase3_removal()


if __name__ == "__main__":
    main()
