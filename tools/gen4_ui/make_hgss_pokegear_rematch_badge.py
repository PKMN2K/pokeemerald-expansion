#!/usr/bin/env python3
"""Prepare genuine HGSS phone-status frames for the Match Call rematch badge.

Tiles 0..3 are the retail active 16x16 frame; 4..7 are the retail disabled
frame. Tile 8 is transparent padding for clearing absent badges. Retail phone
availability states are reused for Emerald rematch semantics, not recolored.
"""

import argparse
from pathlib import Path

import make_hgss_pokegear_cursor as common
from make_hgss_pokegear_phone_status import (
    PHONE_STATUS_PALETTE_BANK,
    PHONE_STATUS_TILES,
    validate_phone_status_cells,
    validate_phone_status_nanr,
)


def build_tiles():
    validate_phone_status_nanr(common.ANIMS)
    validate_phone_status_cells(common.CELLS)
    rows = common.load_indexed_png4(common.SPRITES)
    frames = b"".join(common.extract_base_cursor_tiles(rows, tile)
                      for tile in PHONE_STATUS_TILES)
    return frames + bytes(32)


def build_palette():
    return common.make_palette(PHONE_STATUS_PALETTE_BANK)


def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--tiles', action='store_true')
    group.add_argument('--palette', action='store_true')
    parser.add_argument('output')
    args = parser.parse_args()
    data = build_tiles() if args.tiles else build_palette()
    Path(args.output).write_bytes(data)
    print(f'{args.output}: authentic HGSS rematch badge source, {len(data)} bytes')


if __name__ == '__main__':
    main()
