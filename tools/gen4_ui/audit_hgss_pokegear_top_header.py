#!/usr/bin/env python3
from pathlib import Path
import binascii
import hashlib
import struct
import zlib


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
MESSAGE_GFX = Path("graphics/pokenav/message.png")
MESSAGE_MAP = Path("graphics/pokenav/message.bin")

EXPECTED_GIT_BLOBS = {
    APP_SWITCH_TILES: "0b33041050709233198495bf43f4969738c4921e",
    APP_SWITCH_PALETTE: "69f91d3bf7a72e5868d5dc6cf04add710bd7138a",
    APP_SWITCH_TILEMAP: "c9103c3dba3d4f1ec88025987ad0f2ddbc63c0b1",
    MESSAGE_GFX: "7df840d3a90d11a8c42568df113a1003dc898c24",
    MESSAGE_MAP: "5f9930568acd30668bdc3fb03db6431412f2ba7d",
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


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def paeth_predictor(a, b, c):
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def load_locked_indexed_png4(path):
    data = verify_blob(path)
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError(f"{path}: not a PNG")

    pos = len(PNG_SIGNATURE)
    ihdr = None
    idat = bytearray()
    while pos < len(data):
        if pos + 12 > len(data):
            raise ValueError(f"{path}: truncated PNG chunk")
        length = struct.unpack_from(">I", data, pos)[0]
        chunk_type = data[pos + 4:pos + 8]
        payload_start = pos + 8
        payload_end = payload_start + length
        crc_end = payload_end + 4
        if crc_end > len(data):
            raise ValueError(f"{path}: truncated {chunk_type!r} chunk")

        payload = data[payload_start:payload_end]
        expected_crc = struct.unpack_from(">I", data, payload_end)[0]
        actual_crc = binascii.crc32(chunk_type + payload) & 0xFFFFFFFF
        if expected_crc != actual_crc:
            raise ValueError(f"{path}: invalid {chunk_type!r} CRC")

        if chunk_type == b"IHDR":
            ihdr = struct.unpack(">IIBBBBB", payload)
        elif chunk_type == b"IDAT":
            idat.extend(payload)
        elif chunk_type == b"IEND":
            break
        pos = crc_end

    if ihdr is None:
        raise ValueError(f"{path}: missing IHDR")
    width, height, bit_depth, color_type, compression, filter_method, interlace = ihdr
    if bit_depth != 4 or color_type != 3:
        raise ValueError(f"{path}: expected 4-bit indexed PNG")
    if width % 2:
        raise ValueError(f"{path}: odd-width 4-bit PNG is unsupported")
    if compression != 0 or filter_method != 0 or interlace != 0:
        raise ValueError(f"{path}: unsupported PNG encoding")

    row_bytes = width // 2
    raw = zlib.decompress(bytes(idat))
    if len(raw) != height * (row_bytes + 1):
        raise ValueError(f"{path}: unexpected decompressed size")

    rows = []
    prev = bytearray(row_bytes)
    offset = 0
    for _ in range(height):
        filter_type = raw[offset]
        scan = bytearray(raw[offset + 1:offset + 1 + row_bytes])
        offset += row_bytes + 1
        recon = bytearray(row_bytes)

        for x, value in enumerate(scan):
            left = recon[x - 1] if x else 0
            up = prev[x]
            up_left = prev[x - 1] if x else 0
            if filter_type == 0:
                recon[x] = value
            elif filter_type == 1:
                recon[x] = (value + left) & 0xFF
            elif filter_type == 2:
                recon[x] = (value + up) & 0xFF
            elif filter_type == 3:
                recon[x] = (value + ((left + up) // 2)) & 0xFF
            elif filter_type == 4:
                recon[x] = (value + paeth_predictor(left, up, up_left)) & 0xFF
            else:
                raise ValueError(f"{path}: unsupported PNG filter {filter_type}")

        pixels = []
        for value in recon:
            pixels.extend((value >> 4, value & 0x0F))
        rows.append(pixels)
        prev = recon

    return rows


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


def verify_message_transparency():
    message_map = verify_blob(MESSAGE_MAP)
    if len(message_map) != 32 * 20 * 2:
        raise ValueError(f"{MESSAGE_MAP}: expected 1280 bytes")
    message_entries = struct.unpack("<640H", message_map)
    for y in range(4):
        row = message_entries[y * 32:(y + 1) * 32]
        if [entry & 0x03FF for entry in row[:30]] != [3] * 30:
            raise ValueError(f"{MESSAGE_MAP}: row {y} no longer uses transparent tile 3")

    message_pixels = load_locked_indexed_png4(MESSAGE_GFX)
    tile3 = [
        message_pixels[y][x]
        for y in range(8)
        for x in range(24, 32)
    ]
    if set(tile3) != {0}:
        raise ValueError(f"{MESSAGE_GFX}: tile 3 is no longer transparent")


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
    verify_blob(MESSAGE_GFX)
    verify_blob(MESSAGE_MAP)
    verify_legacy_removed()
    verify_message_transparency()
    verify_live_hgss_binding()


if __name__ == "__main__":
    main()
