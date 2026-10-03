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
MESSAGE_GFX = Path("graphics/pokenav/message.png")
MESSAGE_MAP = Path("graphics/pokenav/message.bin")

EXPECTED_GIT_BLOBS = {
    LEGACY_HEADER_GFX: "7fe891e22f0fd440d3ee7d41ae0a2b35d008ac5d",
    LEGACY_HEADER_MAP: "f405a6dfcfbbacf7f2864d071d019e9c9a0a0084",
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
    # Decode only after this audit's own blob lock passes. Do not reuse an
    # asset-specific verifier from another generator: its allow-list is
    # intentionally limited to that generator's own retail source files.
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
        raise ValueError(f"{path}: odd-width 4-bit PNG is unsupported by this audit")
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


def verify_legacy_header_geometry():
    raw = verify_blob(LEGACY_HEADER_MAP)
    if len(raw) != 32 * 32 * 2:
        raise ValueError(f"{LEGACY_HEADER_MAP}: expected 2048 bytes")

    entries = struct.unpack("<1024H", raw)

    for y in range(32):
        row = entries[y * 32:(y + 1) * 32]
        occupied = [x for x, entry in enumerate(row) if (entry & 0x03FF) != 0]

        if y in (0, 1, 2, 3, 22, 23):
            if occupied != list(range(30)):
                raise ValueError(
                    f"{LEGACY_HEADER_MAP}: row {y} occupied columns changed: "
                    f"{occupied}"
                )
        elif occupied:
            raise ValueError(
                f"{LEGACY_HEADER_MAP}: unexpected occupied row {y}: {occupied}"
            )

    # The only currently visible main-menu legacy header region is rows 0..3.
    # Rows 22..23 are covered by the authentic help-tooltip strip when BG0 is
    # scrolled 32 px for feature screens.
    return entries


def verify_transparent_exposure_path():
    # The legacy header tilemap's canonical blank entry is tile 0. Verify the
    # locked header sheet still keeps that exact tile fully transparent.
    header_pixels = load_locked_indexed_png4(LEGACY_HEADER_GFX)
    header_tile0 = [
        header_pixels[y][x]
        for y in range(8)
        for x in range(8)
    ]
    if set(header_tile0) != {0}:
        raise ValueError(f"{LEGACY_HEADER_GFX}: tile 0 is no longer transparent")

    # BG1 sits between BG0 and the authentic BG2 app-switch. Its main-menu
    # message tilemap uses tile 3 across rows 0..15; verify rows 0..3 still do
    # so and that tile 3 itself is entirely transparent.
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


def verify_live_overlap():
    main = MAIN_MENU_C.read_text()
    gfx = MENU_GFX_C.read_text()

    required_main = (
        "HGSS_HELP_BAR_STRIP_TOP = 20",
        "HGSS_HELP_BAR_STRIP_HEIGHT = 4",
        "sUseLegacyTopHeaderForRollback = FALSE",
        "HGSS_TOP_STRIP_WIDTH = 30",
        "HGSS_TOP_STRIP_HEIGHT = 4",
        "ExposeHgssPokegearTopStrip();",
        "FillBgTilemapBufferRect_Palette0(",
    )
    for token in required_main:
        if token not in main:
            raise ValueError(f"{MAIN_MENU_C}: missing validated tooltip binding {token!r}")

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
    verify_blob(LEGACY_HEADER_GFX)
    verify_blob(APP_SWITCH_TILES)
    verify_blob(APP_SWITCH_PALETTE)
    verify_blob(APP_SWITCH_TILEMAP)
    verify_blob(MESSAGE_GFX)
    verify_blob(MESSAGE_MAP)
    verify_legacy_header_geometry()
    verify_transparent_exposure_path()
    verify_live_overlap()


if __name__ == "__main__":
    main()
