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

EXPECTED_GIT_BLOBS = {
    LEGACY_NAV_ICON: "83624135bafa7dbdd84c461f10a15fe68249ea4f",
    MAIN_UI_PALETTE: "bbdc310859698dcce8dc7e0f9aa45e8ec2d94087",
    MAIN_UI_SPRITES: "c6dfd8b3946d4875a154bd4de359b1ee244dcd18",
    MAIN_UI_CELLS: "aac8033e326bfadbb8d7328f62bb3246582e4d6c",
    MAIN_UI_ANIMS: "8bbd3faad47fad0187eae6441de4c6ddc56a15d6",
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


def verify_completed_hgss_main_ui_source():
    manifest = json.loads(MAIN_UI_MANIFEST.read_text())
    pipeline = manifest.get("pipeline", {})
    required = (
        pipeline.get("authentic_hgss_asset_ready"),
        pipeline.get("wired_live"),
        pipeline.get("legacy_equivalent_removed"),
    )
    if required != (True, True, True):
        raise ValueError(f"{MAIN_UI_MANIFEST}: common HGSS UI source is no longer complete")


def verify_phase2_spinner_suppression():
    manifest = json.loads(NAV_ICON_MANIFEST.read_text())
    if manifest.get("phase") != "legacy_spinner_suppressed_ci_pending":
        raise ValueError(f"{NAV_ICON_MANIFEST}: unexpected phase")

    main = MAIN_MENU_C.read_text()

    # Legacy asset/code stays byte-locked for the one-cycle rollback path.
    rollback_tokens = (
        'INCGFX_U16("graphics/pokenav/nav_icon.png", ".gbapal")',
        'INCGFX_U32("graphics/pokenav/nav_icon.png", ".4bpp.smol")',
        "static const struct CompressedSpriteSheet sSpinningPokenavSpriteSheet[]",
        "static const struct SpriteTemplate sSpinningPokenavSpriteTemplate",
        "CreateSprite(&sSpinningPokenavSpriteTemplate, 220, 12, 0)",
        "SpriteCB_SpinningPokenav",
    )
    for token in rollback_tokens:
        if token not in main:
            raise ValueError(f"{MAIN_MENU_C}: rollback spinner binding missing: {token!r}")

    # The normal/authentic path must instantiate no legacy spinner.
    required = (
        "static bool8 sUseLegacySpinningNavIconForRollback = FALSE;",
        "menu->spinningPokenav = NULL;",
        "if (sUseLegacySpinningNavIconForRollback)",
        "if (menu->spinningPokenav != NULL)",
        "if (menu->spinningPokenav == NULL)",
    )
    for token in required:
        if token not in main:
            raise ValueError(f"{MAIN_MENU_C}: Phase-2 suppression token missing: {token!r}")

    create_pos = main.index("CreateSprite(&sSpinningPokenavSpriteTemplate, 220, 12, 0)")
    gate_pos = main.rfind("if (sUseLegacySpinningNavIconForRollback)", 0, create_pos)
    if gate_pos < 0 or create_pos - gate_pos > 300:
        raise ValueError(f"{MAIN_MENU_C}: legacy spinner creation is not rollback-gated")

    # Existing Match Call call sites are intentionally retained; with no
    # spinner instantiated their helper safely becomes a no-op.
    match_call = MATCH_CALL_C.read_text()
    if "HideSpinningPokenavSprite();" not in match_call:
        raise ValueError(f"{MATCH_CALL_C}: Match Call compatibility call-sites changed unexpectedly")


def main():
    for path in EXPECTED_GIT_BLOBS:
        verify_blob(path)
    verify_completed_hgss_main_ui_source()
    verify_phase2_spinner_suppression()


if __name__ == "__main__":
    main()
