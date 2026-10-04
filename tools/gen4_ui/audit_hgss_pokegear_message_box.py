#!/usr/bin/env python3
from pathlib import Path
import hashlib
import json


HANDLER = Path("src/pokenav_menu_handler_gfx.c")
GRAPHICS = Path("src/graphics.c")
GRAPHICS_H = Path("include/graphics.h")
SHELL_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/screen_shell_skin0.json")
MESSAGE_MANIFEST = Path("graphics/gen4_ui/hgss_pokegear/verified/message_box.json")

LEGACY_GIT_BLOBS = {
    Path("graphics/pokenav/message.png"): "7df840d3a90d11a8c42568df113a1003dc898c24",
    Path("graphics/pokenav/message.bin"): "5f9930568acd30668bdc3fb03db6431412f2ba7d",
}

AUTHENTIC_GIT_BLOBS = {
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_tiles.png"): "163a4fa068fbffae472d1672947fad86647d110b",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_palette.NCLR"): "f293e5f8760201fd109d77bb9cefcdcbb4682f67",
    Path("graphics/gen4_ui/hgss_pokegear/verified/pgear_skin0_screen_shell_tilemap.NSCR"): "f0d5863b63fb82a6bc36c4f49c17859fea8d05aa",
}


def git_blob_sha(data):
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def verify_blob(path, expected):
    actual = git_blob_sha(path.read_bytes())
    if actual != expected:
        raise ValueError(f"{path}: Git blob {actual} != verified {expected}")


def verify_shell_complete():
    manifest = json.loads(SHELL_MANIFEST.read_text())
    pipeline = manifest.get("pipeline", {})
    state = (
        pipeline.get("authentic_hgss_asset_ready"),
        pipeline.get("wired_live"),
        pipeline.get("legacy_equivalent_removed"),
    )
    if state != (True, True, True):
        raise ValueError(f"{SHELL_MANIFEST}: authentic HGSS fixed shell is no longer complete")


def verify_phase2_runtime():
    handler = HANDLER.read_text()
    graphics = GRAPHICS.read_text()
    graphics_h = GRAPHICS_H.read_text()

    required = (
        "static bool8 sUseLegacyMessageBoxForRollback = FALSE;",
        "static const u32 sTransparentMessageBoxBgTile[8] = {0};",
        "SetBgTilemapBuffer(1, gfx->bg1TilemapBuffer);",
        "LoadBgTiles(1, sTransparentMessageBoxBgTile, sizeof(sTransparentMessageBoxBgTile), 0);",
        "FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 32, 32);",
        "if (sUseLegacyMessageBoxForRollback)",
        "DecompressAndCopyTileDataToVram(1, gPokenavMessageBox_Gfx, 0, 0, 0);",
        "CopyToBgTilemapBuffer(1, gPokenavMessageBox_Tilemap, 0, 0);",
        "CopyPaletteIntoBufferUnfaded(gPokenavMessageBox_Pal, BG_PLTT_ID(1), PLTT_SIZE_4BPP);",
        "ShowBg(1);",
        "LoadHgssPokegearScreenShell();",
    )
    for token in required:
        if token not in handler:
            raise ValueError(f"{HANDLER}: Phase-2 runtime token changed: {token!r}")

    gate = handler.index("if (sUseLegacyMessageBoxForRollback)")
    gfx_load = handler.index("DecompressAndCopyTileDataToVram(1, gPokenavMessageBox_Gfx", gate)
    tilemap_load = handler.index("CopyToBgTilemapBuffer(1, gPokenavMessageBox_Tilemap", gate)
    palette_load = handler.index("CopyPaletteIntoBufferUnfaded(gPokenavMessageBox_Pal")
    if gfx_load - gate > 300 or tilemap_load - gate > 400:
        raise ValueError(f"{HANDLER}: legacy message-box pixels are not rollback-gated")
    if palette_load < gate:
        raise ValueError(f"{HANDLER}: Phase-2 text palette dependency moved unexpectedly")

    graphics_tokens = (
        'gPokenavMessageBox_Pal[] = INCGFX_U16("graphics/pokenav/message.png", ".gbapal")',
        'gPokenavMessageBox_Gfx[] = INCGFX_U32("graphics/pokenav/message.png", ".4bpp.smol")',
        'gPokenavMessageBox_Tilemap[] = INCGFX_U32("graphics/pokenav/message.bin", ".smolTM")',
    )
    for token in graphics_tokens:
        if token not in graphics:
            raise ValueError(f"{GRAPHICS}: rollback graphics binding changed: {token!r}")

    for token in (
        "gPokenavMessageBox_Pal",
        "gPokenavMessageBox_Gfx",
        "gPokenavMessageBox_Tilemap",
    ):
        if token not in graphics_h:
            raise ValueError(f"{GRAPHICS_H}: rollback declaration changed: {token!r}")

def verify_phase2_manifest():
    manifest = json.loads(MESSAGE_MANIFEST.read_text())
    if manifest.get("phase") != "legacy_message_box_pixels_suppressed_ci_pending":
        raise ValueError(f"{MESSAGE_MANIFEST}: unexpected phase")
    pipeline = manifest.get("pipeline", {})
    state = (
        pipeline.get("authentic_hgss_asset_ready"),
        pipeline.get("wired_live"),
        pipeline.get("legacy_equivalent_removed"),
    )
    if state != (True, True, False):
        raise ValueError(f"{MESSAGE_MANIFEST}: unexpected Phase-2 pipeline state")


def main():
    for path, expected in LEGACY_GIT_BLOBS.items():
        verify_blob(path, expected)
    for path, expected in AUTHENTIC_GIT_BLOBS.items():
        verify_blob(path, expected)
    verify_shell_complete()
    verify_phase2_runtime()
    verify_phase2_manifest()


if __name__ == "__main__":
    main()
