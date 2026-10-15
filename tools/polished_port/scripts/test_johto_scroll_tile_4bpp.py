#!/usr/bin/env python3
"""Lossless GBA 4bpp equivalent of Polished Crystal's ScrollTileDown.

Source: Rangi42/polishedcrystal 3fa43192379df5c3e7b09a08e4d5d79af4f02f42
engine/tilesets/tileset_anims.asm, ScrollTileDown. The 2bpp routine moves
the final two-byte pixel row to the first row. On GBA 4bpp that means moving
the final four-byte row to the first. No new artwork or palette is introduced.

Staging helper only. A runtime callback, its 22-step dispatch integration,
and actual tile-$35 VRAM updates remain unregistered.
"""


def scroll_down_4bpp(tile):
    """Rotate one 8x8 GBA 4bpp tile down by one pixel (8 rows x 4 bytes)."""
    if not isinstance(tile, (bytes, bytearray)) or len(tile) != 32:
        raise ValueError("expected exactly one 32-byte GBA 4bpp tile")
    return bytes(tile[-4:] + tile[:-4])


def source_scroll_down_2bpp(tile):
    """Reference translation of original ScrollTileDown row-pair rotation."""
    if not isinstance(tile, (bytes, bytearray)) or len(tile) != 16:
        raise ValueError("expected exactly one 16-byte GBC 2bpp tile")
    return bytes(tile[-2:] + tile[:-2])


def unpack_gbc_2bpp(tile):
    if len(tile) != 16:
        raise ValueError("invalid 2bpp tile size")
    rows = []
    for y in range(8):
        low, high = tile[2 * y:2 * y + 2]
        rows.append([((low >> (7 - x)) & 1) | (((high >> (7 - x)) & 1) << 1)
                     for x in range(8)])
    return rows


def to_gba_4bpp(tile):
    """Map opaque GBC indices 0..3 to the existing staged GBA indices 4,1,2,3."""
    rows = unpack_gbc_2bpp(tile)
    pixels = [4 if p == 0 else p for row in rows for p in row]
    return bytes(pixels[i] | (pixels[i + 1] << 4) for i in range(0, 64, 2))


def test():
    # Distinct row/pixel patterns test bitplane decoding, row direction,
    # color-0 opacity, and commutation with the original GBC routine.
    for seed in range(16):
        original = bytes(((i * 29 + seed * 17) ^ (i >> 1)) & 255
                         for i in range(16))
        gba = to_gba_4bpp(original)
        gbc = original
        for _ in range(8):
            assert scroll_down_4bpp(to_gba_4bpp(gbc)) == to_gba_4bpp(
                source_scroll_down_2bpp(gbc))
            gba = scroll_down_4bpp(gba)
            gbc = source_scroll_down_2bpp(gbc)
            assert gba == to_gba_4bpp(gbc)
        assert gba == to_gba_4bpp(original)
    assert scroll_down_4bpp(bytes(range(32))) == bytes(
        list(range(28, 32)) + list(range(28)))
    for bad in (b"", bytes(16), bytes(33), "not bytes"):
        try:
            scroll_down_4bpp(bad)
        except ValueError:
            pass
        else:
            raise AssertionError("invalid GBA tile was accepted")
    print("PASS: 4bpp scroll direction and all eight source row rotations")
    print("PASS: source GBC color 0 remains opaque GBA palette index 4")
    print("STAGED ONLY: no Johto layout or Emerald tile-animation callback installed")


if __name__ == "__main__":
    test()
