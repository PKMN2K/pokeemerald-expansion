#!/usr/bin/env python3
from pathlib import Path
import hashlib
import json


LEGACY_NAV_ICON = Path("graphics/pokenav/nav_icon.png")
MAIN_UI_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/main_ui_sprites_skin0.json")
MAIN_UI_PALETTE = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_ui_palette.NCLR")
MAIN_UI_SPRITES = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_ui_sprites.png")
MAIN_UI_CELLS = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_ui_cells.NCER")
MAIN_UI_ANIMS = Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_ui_anims.NANR")
NAV_ICON_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/spinning_nav_icon.json")
MAIN_MENU_C = Path("src/pokenav_main_menu.c")
MATCH_CALL_C = Path("src/pokenav_match_call_gfx.c")
POKENAV_H = Path("include/pokenav.h")

EXPECTED_HGSS_GIT_BLOBS = {
    MAIN_UI_PALETTE: "bbdc310859698dcce8dc7e0f9aa45e8ec2d94087",
    MAIN_UI_SPRITES: "c6dfd8b3946d4875a154bd4de359b1ee244dcd18",
    MAIN_UI_CELLS: "aac8033e326bfadbb8d7328f62bb3246582e4d6c",
    MAIN_UI_ANIMS: "8bbd3faad47fad0187eae6441de4c6ddc56a15d6",
}

REMOVED_TOKENS = (
    "spinningPokenav",
    "SpinningPokenav",
    "sUseLegacySpinningNavIconForRollback",
    "sSpinningNavgearPalettes",
    "nav_icon.png",
    "GetSpinningPokenavSprite",
    "HideSpinningPokenavSprite",
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


def verify_completed_hgss_main_ui_source():
    manifest = json.loads(MAIN_UI_MANIFEST.read_text())
    pipeline = manifest.get("pipeline", {})
    if (
        pipeline.get("authentic_hgss_asset_ready"),
        pipeline.get("wired_live"),
        pipeline.get("legacy_equivalent_removed"),
    ) != (True, True, True):
        raise ValueError(f"{MAIN_UI_MANIFEST}: common HGSS UI source is no longer complete")


def verify_phase3_spinner_removal():
    manifest = json.loads(NAV_ICON_MANIFEST.read_text())
    if manifest.get("phase") != "legacy_spinner_removed_ci_pending":
        raise ValueError(f"{NAV_ICON_MANIFEST}: unexpected phase")
    if manifest.get("pipeline", {}).get("legacy_equivalent_removed") is not True:
        raise ValueError(f"{NAV_ICON_MANIFEST}: removal flag is not final")

    if LEGACY_NAV_ICON.exists():
        raise ValueError(f"{LEGACY_NAV_ICON}: obsolete legacy spinner asset still exists")

    for path in (MAIN_MENU_C, MATCH_CALL_C, POKENAV_H):
        text = path.read_text()
        for token in REMOVED_TOKENS:
            if token in text:
                raise ValueError(f"{path}: removed spinner token still present: {token!r}")

    main = MAIN_MENU_C.read_text()
    if "menu->palettes = ~1;" not in main:
        raise ValueError(
            f"{MAIN_MENU_C}: palette fade mask was not normalized after spinner palette removal"
        )


def main():
    for path in EXPECTED_HGSS_GIT_BLOBS:
        verify_hgss_blob(path)
    verify_completed_hgss_main_ui_source()
    verify_phase3_spinner_removal()


if __name__ == "__main__":
    main()
